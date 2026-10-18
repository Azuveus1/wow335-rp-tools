#include "Creature.h"
#include "MapManager.h"
#include "Timer.h"
#include "Util.h"
#include <sstream>
#include <chrono>
#include "GMTransportRoute.h"
#include "TransportMgr.h"
#include "Transport.h"
#include "ObjectMgr.h"
#include "GameObjectData.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Chat.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "Map.h"
#include "Log.h"
#include <map>
#include <mutex>
#include <memory>
#include "GMNativeRoute.h"

namespace GMTransport {
struct Record {
    GMRoute::Route route;
    GameObjectTemplate object;
    TransportTemplate path;
    std::vector<TaxiPathNodeEntry> nodes;
    bool active=false;
    Transport* live=nullptr;
};
static std::map<uint32,std::unique_ptr<Record>> records;
static std::recursive_mutex guard;
struct NativeRecord {
    GameObjectTemplate object;
    std::vector<TaxiPathNodeEntry> owned;
    TaxiPathNodeList path;
};
static std::map<uint32,std::unique_ptr<NativeRecord>> nativeRecords;
static bool ValidMaps(GMRoute::Route const& r) {
    std::set<uint32> ids;for(size_t i=0;i<r.nodes.size();++i) if(!ids.insert(r.nodes[i].uid?r.nodes[i].uid:uint32(i+1)).second) return false;
    for(size_t i=0;i<r.nodes.size();++i) { auto m=sMapStore.LookupEntry(r.NodeMap(i));if(!m || m->Instanceable()) return false; }
    return true;
}
static bool Register(uint32 entry,uint32 source,GMRoute::Route route) {
    auto original=sObjectMgr->GetGameObjectTemplate(source);
    auto map=sMapStore.LookupEntry(route.map);
    if(!original || original->type!=GAMEOBJECT_TYPE_MAP_OBJ_TRANSPORT || !map || map->Instanceable() || !route.Build() || !ValidMaps(route)) return false;
    auto display=sGameObjectDisplayInfoStore.LookupEntry(original->displayId);
    if(!display || !display->ModelName) return false;
    std::lock_guard<std::recursive_mutex> lock(guard);
    if(records.count(entry)) return false;
    auto r=std::make_unique<Record>(); r->route=route; r->object=*original;
    r->object.entry=entry; r->object.name="GM Route " + std::to_string(entry);
    r->object.moTransport.taxiPathId=0; r->object.moTransport.mapID=0;
    r->object.moTransport.canBeStopped=0; r->object.ScriptId=0; r->object.AIName.clear();
    r->object.InitializeQueryData();
    r->path.entry=entry; r->path.inInstance=false; r->path.pathTime=route.period; for(size_t i=0;i<route.nodes.size();++i) r->path.mapsUsed.insert(route.NodeMap(i));
    r->nodes.resize(route.nodes.size());
    for(size_t i=0;i<route.nodes.size();++i) {
        auto& n=r->nodes[i]; n={}; n.NodeIndex=uint32(i); n.ContinentID=route.NodeMap(i);
        n.Loc.X=float(route.nodes[i].p.x); n.Loc.Y=float(route.nodes[i].p.y); n.Loc.Z=float(route.nodes[i].p.z);
        r->path.keyFrames.emplace_back(&n); r->path.keyFrames.back().InitialOrientation=float(route.Facing(i,0));
    }
    records.emplace(entry,std::move(r)); return true;
}
GameObjectTemplate const* Template(uint32 entry) {
    std::lock_guard<std::recursive_mutex> lock(guard); auto i=records.find(entry); if(i!=records.end()) return &i->second->object; auto n=nativeRecords.find(entry); return n==nativeRecords.end()?nullptr:&n->second->object;
}
TransportTemplate const* Path(uint32 entry) {
    std::lock_guard<std::recursive_mutex> lock(guard); auto i=records.find(entry); return i==records.end()?nullptr:&i->second->path;
}
bool Position(uint32 entry,uint32 time,GMRoute::Pose& pose) {
    std::lock_guard<std::recursive_mutex> lock(guard); auto i=records.find(entry); if(i==records.end()) return false;
    pose=i->second->route.At(time); return true;
}
static void Send(Player* player,uint32 entry,Record const& r) {
    ChatHandler h(player->GetSession());
    h.PSendSysMessage("[TR:BEGIN:%u:%u:%u:%.6f:%.6f:%u]",entry,r.object.displayId,r.route.map,r.route.speed,r.object.size,uint32(r.route.nodes.size()));
    for(size_t i=0;i<r.route.nodes.size();++i) {
        auto const& n=r.route.nodes[i];
        h.PSendSysMessage("[TR:NODE:%u:%u:%.6f:%.6f:%.6f:%.3f:%u]",entry,uint32(i+1),n.p.x,n.p.y,n.p.z,n.wait,r.route.NodeMap(i));
    }
    h.PSendSysMessage("[TR:END:%u]",entry);
}
void Sync(Player* player) {
    std::lock_guard<std::recursive_mutex> lock(guard);
    for(auto const& item:records) Send(player,item.first,*item.second);
    ChatHandler h(player->GetSession());
    for(auto const& item:nativeRecords) {
        auto const& r=*item.second; auto t=sTransportMgr->GetTransportTemplate(item.first);
        h.PSendSysMessage("[TR:NATIVE:%u:%u:%u:%u:%u:%u]",item.first,r.object.moTransport.taxiPathId,r.object.moTransport.moveSpeed,r.object.moTransport.accelRate,t?t->pathTime:0,uint32(r.path.size()));
        for(size_t i=0;i<r.path.size();++i) { auto n=r.path[i];
            h.PSendSysMessage("[TR:NNODE:%u:%u:%.6f:%.6f:%.6f:%u:%u:%u:%u:%u]",item.first,uint32(i+1),n->Loc.X,n->Loc.Y,n->Loc.Z,n->Delay,n->ContinentID,n->Flags,n->ArrivalEventID,n->DepartureEventID);
        }
        h.PSendSysMessage("[TR:NEND:%u]",item.first);
    }
}
static void SaveMeta(WorldDatabaseTransaction const&,uint32,std::vector<GMRoute::Node> const&);
uint32 Save(uint32 source,GMRoute::Route const& route) {
    // Commands run on the world session thread. Published records are immutable.
    std::lock_guard<std::recursive_mutex> lock(guard);
    if(records.size()>=128) return 0;
    uint32 entry=9000000;
    if(auto last=WorldDatabase.Query("SELECT MAX(entry) FROM gm_transport_route")) entry=std::max(entry,last->Fetch()[0].GetUInt32()+1);
    if(entry>9999999) return 0;
    while(sObjectMgr->GetGameObjectTemplate(entry)) ++entry;
    if(!Register(entry,source,route)) return 0;
    SQLTransaction trans=WorldDatabase.BeginTransaction();
    trans->PAppend("INSERT INTO gm_transport_route (entry,source,map,speed,enabled) VALUES ({},{},{},{},0)",entry,source,route.map,route.speed);
    for(size_t i=0;i<route.nodes.size();++i) {
        auto const& n=route.nodes[i];
        trans->PAppend("INSERT INTO gm_transport_route_node (entry,node,x,y,z,wait_seconds) VALUES ({},{},{},{},{},{})",entry,i,n.p.x,n.p.y,n.p.z,n.wait);
        trans->PAppend("INSERT INTO gm_transport_route_node_map VALUES ({},{},{})",entry,i,route.NodeMap(i));
    }
    SaveMeta(trans,entry,route.nodes);
    WorldDatabase.DirectCommitTransaction(trans);
    auto check=WorldDatabase.PQuery("SELECT COUNT(*) FROM gm_transport_route_node WHERE entry={}",entry);
    if(!check || check->Fetch()[0].GetUInt64()!=route.nodes.size()) { records.erase(entry); return 0; }
    return entry;
}
bool Activate(uint32 entry,Player* player) {
    std::lock_guard<std::recursive_mutex> lock(guard);
    auto i=records.find(entry); if(i==records.end()) return false;
    auto& r=*i->second;
    if(r.active) return true; // Idempotent activation, never duplicate a transport.
    if(player && (!r.path.mapsUsed.count(player->GetMapId()) || player->GetInstanceId()!=0)) return false;
    // Deliver path before the object's create packet to every currently connected client.
    for(auto const& session:sWorld->GetAllSessions()) if(auto p=session.second->GetPlayer()) Send(p,entry,r);
    r.live=sTransportMgr->CreateTransport(entry);
    if(!r.live) return false;
    r.active=true;
    TC_LOG_INFO("entities.transport", "GM route {} activated: {} nodes, map {}, period {} ms",entry,r.route.nodes.size(),r.route.map,r.route.period);
    WorldDatabase.DirectPExecute("UPDATE gm_transport_route SET enabled=1 WHERE entry={}",entry);
    return true;
}
bool Deactivate(uint32 entry,Player* player) {
    std::lock_guard<std::recursive_mutex> lock(guard);
    auto i=records.find(entry); if(i==records.end()) return false;
    auto& r=*i->second;
    if(r.live) {
        if(player->GetMap()!=r.live->GetMap() || !r.live->GetPassengers().empty()) return false;
        r.live->GetMap()->RemoveFromMap<Transport>(r.live,true); r.live=nullptr;
    }
    r.active=false;
    WorldDatabase.DirectPExecute("UPDATE gm_transport_route SET enabled=0 WHERE entry={}",entry);
    return true;
}
void Load() {
    auto query=WorldDatabase.Query("SELECT entry,source,map,speed,enabled FROM gm_transport_route ORDER BY entry LIMIT 128");
    if(!query) return;
    do {
        auto f=query->Fetch(); uint32 entry=f[0].GetUInt32(),source=f[1].GetUInt32();
        GMRoute::Route r; r.map=f[2].GetUInt32(); r.speed=f[3].GetDouble();
        auto nodes=WorldDatabase.PQuery("SELECT n.x,n.y,n.z,n.wait_seconds,COALESCE(m.map,{}),COALESCE(meta.uid,n.node+1) FROM gm_transport_route_node n LEFT JOIN gm_transport_route_node_map m ON m.entry=n.entry AND m.node=n.node LEFT JOIN gm_transport_node_meta meta ON meta.entry=n.entry AND meta.node=n.node WHERE n.entry={} ORDER BY n.node",r.map,entry);
        if(nodes) do { auto n=nodes->Fetch(); r.nodes.push_back({{n[0].GetDouble(),n[1].GetDouble(),n[2].GetDouble()},n[3].GetDouble(),uint32(n[4].GetUInt64()),0,0,0,uint32(n[5].GetUInt64())}); } while(nodes->NextRow());
        if(!sObjectMgr->GetGameObjectTemplate(entry) && Register(entry,source,r) && f[4].GetBool()) Activate(entry,nullptr);
    } while(query->NextRow());
    TC_LOG_INFO("entities.transport", "Loaded {} GM editor routes",records.size());
}
uint32 Revision(uint32 entry) {
    auto q=WorldDatabase.PQuery("SELECT revision FROM gm_transport_revision WHERE entry={}",entry);
    return q?q->Fetch()[0].GetUInt32():0;
}
static void AddRevision(WorldDatabaseTransaction const& tx,uint32 entry,uint32 revision) {
    tx->PAppend("INSERT INTO gm_transport_revision VALUES ({},{}) ON DUPLICATE KEY UPDATE revision=VALUES(revision)",entry,revision+1);
}
static void Archive(WorldDatabaseTransaction const& tx,uint32 entry,uint32 revision,bool native) {
    if(native)
        tx->PAppend("INSERT IGNORE INTO gm_transport_history(entry,revision,payload) SELECT entry,{},JSON_OBJECT('speed',speed,'meta',(SELECT JSON_ARRAYAGG(JSON_ARRAY(node,uid,map,flags,arrival,departure)) FROM gm_transport_node_meta WHERE entry={}), 'nodes',(SELECT JSON_ARRAYAGG(JSON_ARRAY(node,x,y,z,wait_seconds)) FROM gm_transport_native_node WHERE entry={})) FROM gm_transport_native WHERE entry={}",revision,entry,entry,entry);
    else
        tx->PAppend("INSERT IGNORE INTO gm_transport_history(entry,revision,payload) SELECT entry,{},JSON_OBJECT('source',source,'map',map,'speed',speed,'enabled',enabled,'nodes',(SELECT JSON_ARRAYAGG(JSON_ARRAY(node,x,y,z,wait_seconds,(SELECT map FROM gm_transport_route_node_map m WHERE m.entry=gm_transport_route_node.entry AND m.node=gm_transport_route_node.node))) FROM gm_transport_route_node WHERE entry={})) FROM gm_transport_route WHERE entry={}",revision,entry,entry);
}
static TaxiPathNodeList const* OriginalPath(uint32 entry) {
    auto object=sObjectMgr->GetGameObjectTemplate(entry);
    if(!object || object->type!=GAMEOBJECT_TYPE_MAP_OBJ_TRANSPORT) return nullptr;
    auto id=object->moTransport.taxiPathId;
    if(!id || id>=sTaxiPathNodesByPath.size() || sTaxiPathNodesByPath[id].size()<4 || sTaxiPathNodesByPath[id].size()>1024) return nullptr;
    return &sTaxiPathNodesByPath[id];
}
TaxiPathNodeList const* NativePath(uint32 entry) {
    auto i=nativeRecords.find(entry); return i==nativeRecords.end()?nullptr:&i->second->path;
}
static std::vector<GMRoute::Node> NativeEditable(uint32 entry, std::vector<TaxiPathNodeEntry> const& nodes) {
    std::vector<GMRoute::Node> r;
    for(size_t i=0;i<nodes.size();++i) {auto const& n=nodes[i];r.push_back({{n.Loc.X,n.Loc.Y,n.Loc.Z},double(n.Delay),n.ContinentID,n.Flags,n.ArrivalEventID,n.DepartureEventID,uint32(i+1)});}
    if(auto q=WorldDatabase.PQuery("SELECT node,uid FROM gm_transport_node_meta WHERE entry={} ORDER BY node",entry)) do {auto f=q->Fetch();if(f[0].GetUInt32()<r.size()) r[f[0].GetUInt32()].uid=f[1].GetUInt32();} while(q->NextRow());
    return r;
}
static bool NativeMaps(std::vector<GMRoute::Node> const& nodes) {
    std::set<uint32> maps;bool instance=false;
    for(auto const& n:nodes) {auto m=sMapStore.LookupEntry(n.map);if(!m) return false;maps.insert(n.map);instance|=m->Instanceable();}
    return !instance || maps.size()==1;
}
static bool ReadNative(uint32 entry,std::vector<TaxiPathNodeEntry>& nodes,uint32& speed) {
    auto base=OriginalPath(entry); if(!base) return false;
    nodes.clear(); for(auto n:*base) nodes.push_back(*n);
    speed=sObjectMgr->GetGameObjectTemplate(entry)->moTransport.moveSpeed;
    auto saved=WorldDatabase.PQuery("SELECT speed FROM gm_transport_native WHERE entry={}",entry);
    if(saved) {
        speed=saved->Fetch()[0].GetUInt32();
        auto q=WorldDatabase.PQuery("SELECT n.node,n.x,n.y,n.z,n.wait_seconds,m.map,m.flags,m.arrival,m.departure FROM gm_transport_native_node n LEFT JOIN gm_transport_node_meta m ON m.entry=n.entry AND m.node=n.node WHERE n.entry={} ORDER BY n.node",entry);
        if(!q || q->GetRowCount()<4 || q->GetRowCount()>1024 || speed<1 || speed>40) return false;
        std::vector<TaxiPathNodeEntry> edited;size_t i=0;
        do {auto f=q->Fetch();if(f[0].GetUInt32()!=i) return false;
            TaxiPathNodeEntry n={};
            if(f[5].IsNull()) {if(q->GetRowCount()!=nodes.size()) return false;n=nodes[i];}
            else {n.ContinentID=f[5].GetUInt32();n.Flags=f[6].GetUInt32();n.ArrivalEventID=f[7].GetUInt32();n.DepartureEventID=f[8].GetUInt32();}
            n.NodeIndex=uint32(i++);n.PathID=base->front()->PathID;
            n.Loc.X=float(f[1].GetDouble());n.Loc.Y=float(f[2].GetDouble());n.Loc.Z=float(f[3].GetDouble());n.Delay=f[4].GetUInt32();edited.push_back(n);
        } while(q->NextRow());nodes=std::move(edited);
    }
    auto check=NativeEditable(entry,nodes);
    if(!GMRoute::ValidNative(check)) return false;
    return NativeMaps(check);
}
static void SaveMeta(WorldDatabaseTransaction const& tx,uint32 entry,std::vector<GMRoute::Node> const& nodes) {
    tx->PAppend("DELETE FROM gm_transport_node_meta WHERE entry={}",entry);
    std::string ids;
    for(size_t i=0;i<nodes.size();++i) {auto n=nodes[i];if(!n.uid) n.uid=uint32(i+1);
        tx->PAppend("INSERT INTO gm_transport_node_meta VALUES ({},{},{},{},{},{},{})",entry,i,n.uid,n.map,n.flags,n.arrival,n.departure);
        if(!ids.empty()) ids+=",";ids+=std::to_string(n.uid);
    }
    if(!ids.empty()) tx->PAppend("UPDATE gm_transport_announcement SET enabled=0,revision=revision+1 WHERE entry={} AND node_uid NOT IN ({})",entry,ids);
}
void LoadNative() {
    auto q=WorldDatabase.Query("SELECT entry FROM gm_transport_native ORDER BY entry"); if(!q) return;
    do {
        uint32 entry=q->Fetch()[0].GetUInt32(),speed=0;
        auto r=std::make_unique<NativeRecord>();
        if(!ReadNative(entry,r->owned,speed)) { TC_LOG_ERROR("entities.transport","Invalid native editor override {} ignored",entry); continue; }
        r->object=*sObjectMgr->GetGameObjectTemplate(entry); r->object.moTransport.moveSpeed=speed; r->object.InitializeQueryData();
        for(auto const& n:r->owned) r->path.push_back(&n);
        nativeRecords[entry]=std::move(r);
        TC_LOG_INFO("entities.transport","Loaded native editor override {}",entry);
    } while(q->NextRow());
}
void Catalogue(Player* player) {
    ChatHandler h(player->GetSession()); h.SendSysMessage("[TR:LISTBEGIN]");
    auto q=WorldDatabase.Query("SELECT entry,name,type FROM gameobject_template WHERE type IN (11,15) ORDER BY name,entry");
    if(q) do { auto f=q->Fetch(); uint32 entry=f[0].GetUInt32(); std::string name=f[1].GetString();
        for(char& c:name) if(c==':' || c=='|' || c=='[' || c==']' || c=='\n' || c=='\r') c=' ';
        if(name.size()>100) name.resize(100);
        uint32 kind=f[2].GetUInt32()==15 && OriginalPath(entry)?1:2;
        h.PSendSysMessage("[TR:ITEM:%u:%u:%u:%s]",entry,kind,Revision(entry),name.c_str());
    } while(q->NextRow());
    std::lock_guard<std::recursive_mutex> lock(guard);
    for(auto const& item:records) h.PSendSysMessage("[TR:ITEM:%u:0:%u:%s]",item.first,Revision(item.first),item.second->object.name.c_str());
    h.SendSysMessage("[TR:LISTEND]");
}
bool Edit(Player* player,uint32 entry) {
    std::lock_guard<std::recursive_mutex> lock(guard); ChatHandler h(player->GetSession());
    auto i=records.find(entry);
    if(i!=records.end()) {
        auto& r=*i->second; uint32 source=0;
        if(auto q=WorldDatabase.PQuery("SELECT source FROM gm_transport_route WHERE entry={}",entry)) source=q->Fetch()[0].GetUInt32();
        h.PSendSysMessage("[TR:EDIT:%u:%u:0:%u:%u:%.6f:%u:%.6f:%u:%u:0:0]",entry,source,Revision(entry),r.route.map,r.route.speed,r.object.displayId,r.object.size,uint32(r.route.nodes.size()),r.active?1:0);
        for(size_t j=0;j<r.route.nodes.size();++j) { auto n=r.route.nodes[j];
            h.PSendSysMessage("[TR:ENODE:%u:%u:%.6f:%.6f:%.6f:%.3f:%u:0:0:0:%u]",entry,uint32(j+1),n.p.x,n.p.y,n.p.z,n.wait,r.route.NodeMap(j),n.uid?n.uid:uint32(j+1));
        }
    } else {
        std::vector<TaxiPathNodeEntry> nodes; uint32 speed;
        if(!ReadNative(entry,nodes,speed)) return false;
        auto o=sObjectMgr->GetGameObjectTemplate(entry);
        h.PSendSysMessage("[TR:EDIT:%u:%u:1:%u:%u:%u:%u:%.6f:%u:0:%u:%u]",entry,entry,Revision(entry),nodes.front().ContinentID,speed,o->displayId,o->size,uint32(nodes.size()),o->moTransport.accelRate,o->moTransport.taxiPathId);
        auto editable=NativeEditable(entry,nodes);
        for(size_t j=0;j<nodes.size();++j) { auto n=nodes[j];
            h.PSendSysMessage("[TR:ENODE:%u:%u:%.6f:%.6f:%.6f:%u:%u:%u:%u:%u:%u]",entry,uint32(j+1),n.Loc.X,n.Loc.Y,n.Loc.Z,n.Delay,n.ContinentID,n.Flags,n.ArrivalEventID,n.DepartureEventID,editable[j].uid);
        }
    }
    uint32 high=0;
    if(auto q=WorldDatabase.PQuery("SELECT GREATEST(COALESCE((SELECT MAX(uid) FROM gm_transport_node_meta WHERE entry={}),0),COALESCE((SELECT MAX(node_uid) FROM gm_transport_announcement WHERE entry={}),0))",entry,entry)) high=uint32(q->Fetch()[0].GetUInt64());
    h.PSendSysMessage("[TR:EEND:%u:%u]",entry,high); return true;
}
bool Update(uint32 entry,uint32 revision,GMRoute::Route const& route,Player* player) {
    std::lock_guard<std::recursive_mutex> lock(guard);
    auto i=records.find(entry); if(i==records.end() || Revision(entry)!=revision) return false;
    // Live instances hold pointers into the record. Remove an empty transport before replacement.
    auto& old=*i->second; bool active=old.active;
    if(old.live && (old.live->GetMap()!=player->GetMap() || !old.live->GetPassengers().empty())) return false;
    auto checked=route; if(!checked.Build() || !ValidMaps(checked)) return false;
    auto tx=WorldDatabase.BeginTransaction();
    Archive(tx,entry,revision,false);
    tx->PAppend("UPDATE gm_transport_route SET speed={},map={} WHERE entry={}",route.speed,checked.map,entry);
    tx->PAppend("DELETE FROM gm_transport_route_node WHERE entry={}",entry);
    for(size_t j=0;j<route.nodes.size();++j) { auto n=route.nodes[j]; tx->PAppend("INSERT INTO gm_transport_route_node VALUES ({},{},{},{},{},{})",entry,j,n.p.x,n.p.y,n.p.z,n.wait); tx->PAppend("INSERT INTO gm_transport_route_node_map VALUES ({},{},{})",entry,j,checked.NodeMap(j)); }
    SaveMeta(tx,entry,checked.nodes);
    AddRevision(tx,entry,revision); WorldDatabase.DirectCommitTransaction(tx);
    if(Revision(entry)!=revision+1) return false;
    uint32 source=0; if(auto q=WorldDatabase.PQuery("SELECT source FROM gm_transport_route WHERE entry={}",entry)) source=q->Fetch()[0].GetUInt32();
    if(old.live) { old.live->GetMap()->RemoveFromMap<Transport>(old.live,true);old.live=nullptr; }
    records.erase(i);
    if(!Register(entry,source,checked)) return false;
    if(active && !Activate(entry,player)) return false;
    for(auto const& session:sWorld->GetAllSessions()) if(auto p=session.second->GetPlayer()) Send(p,entry,*records.at(entry));
    return true;
}
bool UpdateNative(uint32 entry,uint32 revision,double speed,std::vector<GMRoute::Node> const& nodes) {
    std::lock_guard<std::recursive_mutex> lock(guard);
    auto original=OriginalPath(entry);
    if(!original || Revision(entry)!=revision || !GMRoute::ValidNative(nodes) || !NativeMaps(nodes) || !std::isfinite(speed) || speed<1 || speed>40 || speed!=std::floor(speed)) return false;
    for(size_t i=0;i<nodes.size();++i) { auto n=nodes[i];
        if(!std::isfinite(n.p.x)||!std::isfinite(n.p.y)||!std::isfinite(n.p.z)||std::abs(n.p.x)>17000||std::abs(n.p.y)>17000||std::abs(n.p.z)>10000||!std::isfinite(n.wait)||n.wait<0||n.wait>600||n.wait!=std::floor(n.wait)) return false;

        // Stops retain native acceleration and deceleration.
        if(n.flags!=2 && n.wait!=0) return false;
    }
    auto tx=WorldDatabase.BeginTransaction();
    Archive(tx,entry,revision,true);
    tx->PAppend("INSERT INTO gm_transport_native VALUES ({},{}) ON DUPLICATE KEY UPDATE speed=VALUES(speed)",entry,uint32(speed));
    tx->PAppend("DELETE FROM gm_transport_native_node WHERE entry={}",entry);
    for(size_t i=0;i<nodes.size();++i) { auto n=nodes[i];tx->PAppend("INSERT INTO gm_transport_native_node VALUES ({},{},{},{},{},{})",entry,i,n.p.x,n.p.y,n.p.z,uint32(n.wait)); }
    SaveMeta(tx,entry,nodes);
    AddRevision(tx,entry,revision);WorldDatabase.DirectCommitTransaction(tx);
    return Revision(entry)==revision+1;
}


#include "GMTransportAnnouncements.inc"
}
