static uint64 AnnNow() {return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
// Included inside namespace GMTransport. Announcement edits are published at restart.
struct Announcement {
    uint32 id=0,entry=0,uid=0,phase=0,delay=0,map=0,mode=1,revision=0;
    uint64 spawn=0;
    uint32 script=0,type=0,action=0;
    bool enabled=true;
    std::string text,name;
};
static std::vector<Announcement> announcements;
static std::map<uint32,std::vector<uint32>> announcementNodes;
static std::mutex announcementMutex;
struct PendingSpeech {uint64 due;Announcement a;};
static std::vector<PendingSpeech> pendingSpeech;
static std::string Hex(std::string const& s) {static char const* digits="0123456789abcdef";std::string r;for(unsigned char c:s){r+=digits[c>>4];r+=digits[c&15];}return r;}
static bool Unhex(std::string const& s,std::string& r) {
    if(s.empty() || s.size()%2 || s.size()>160) return false;
    auto digit=[](char c)->int {if(c>='0'&&c<='9') return c-'0';if(c>='a'&&c<='f') return c-'a'+10;return -1;};
    for(size_t i=0;i<s.size();i+=2){int a=digit(s[i]),b=digit(s[i+1]);if(a<0||b<0) return false;char c=char(a*16+b);if(uint8(c)<32 || c=='|') return false;r+=c;}return true;
}
static std::vector<GMRoute::Node> EditableNodes(uint32 entry) {
    auto r=records.find(entry);if(r!=records.end()) return r->second->route.nodes;
    std::vector<TaxiPathNodeEntry> nodes;uint32 speed=0;
    return ReadNative(entry,nodes,speed)?NativeEditable(entry,nodes):std::vector<GMRoute::Node>{};
}
static bool AnnNode(uint32 entry,uint32 uid,uint32& map) {
    auto nodes=EditableNodes(entry);for(size_t i=0;i<nodes.size();++i) if((nodes[i].uid?nodes[i].uid:uint32(i+1))==uid) {
        // Native spline control/sentinel nodes never receive server waypoint callbacks.
        if(entry<9000000) {
            bool skip=false;std::vector<size_t> frames;
            for(size_t j=0;j<nodes.size();++j) {if(skip){skip=false;continue;}if(j+1<nodes.size() && ((nodes[j].flags&1)||nodes[j].map!=nodes[j+1].map)){skip=true;continue;}frames.push_back(j);}
            if(!frames.empty() && nodes[frames.front()].flags!=2 && !nodes[frames.front()].arrival && !nodes[frames.front()].departure) frames.erase(frames.begin());
            if(!frames.empty() && nodes[frames.back()].flags!=2 && !nodes[frames.back()].arrival && !nodes[frames.back()].departure) frames.pop_back();
            if(std::find(frames.begin(),frames.end(),i)==frames.end()) return false;
        }
        map=nodes[i].map;return true;
    }return false;
}
static std::vector<Announcement> StoredAnnouncements(uint32 entry=0) {
    std::vector<Announcement> result;
    auto q=WorldDatabase.PQuery("SELECT id,entry,node_uid,phase,delay_ms,spawn_id,map,mode,text,legacy_script,legacy_type,legacy_id,enabled,revision FROM gm_transport_announcement WHERE ({}=0 OR entry={}) ORDER BY id",entry,entry);
    if(q) do {auto f=q->Fetch();Announcement a;a.id=f[0].GetUInt32();a.entry=f[1].GetUInt32();a.uid=f[2].GetUInt32();a.phase=f[3].GetUInt32();a.delay=f[4].GetUInt32();a.spawn=f[5].GetUInt64();a.map=f[6].GetUInt32();a.mode=f[7].GetUInt32();a.text=f[8].GetString();a.script=f[9].GetUInt32();a.type=f[10].GetUInt32();a.action=f[11].GetUInt32();a.enabled=f[12].GetBool();a.revision=f[13].GetUInt32();result.push_back(a);} while(q->NextRow());return result;
}
static void OriginalTalk(std::vector<Announcement>& result,uint32 entry,GMRoute::Node const& node,uint32 phase,uint32 script,uint32 type,uint32 action,uint32 group,uint32 creature,uint32 delay) {
    auto text=WorldDatabase.PQuery("SELECT Text,Type FROM creature_text WHERE CreatureID={} AND GroupID={} ORDER BY ID LIMIT 1",creature,group);
    auto npc=WorldDatabase.PQuery("SELECT c.guid,c.map,t.name FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.id={} AND c.map={} ORDER BY POW(c.position_x-{},2)+POW(c.position_y-{},2)+POW(c.position_z-{},2),c.guid LIMIT 1",creature,node.map,node.p.x,node.p.y,node.p.z);
    if(!text||!npc) return;
    auto f=npc->Fetch();Announcement a;a.entry=entry;a.uid=node.uid;a.phase=phase;a.delay=delay;a.spawn=f[0].GetUInt64();a.map=f[1].GetUInt32();a.name=f[2].GetString();a.script=script;a.type=type;a.action=action;a.text=text->Fetch()[0].GetString();a.mode=text->Fetch()[1].GetUInt32()==14?1:0;result.push_back(a);
}
static std::vector<Announcement> OriginalAnnouncements(uint32 entry) {
    std::vector<Announcement> result;auto base=OriginalPath(entry);if(!base) return result;
    // Resolve original events against DBC, then retain their stable identity after edits.
    std::vector<TaxiPathNodeEntry> raw;for(auto n:*base) raw.push_back(*n);
    std::vector<GMRoute::Node> nodes;
    for(size_t i=0;i<raw.size();++i) {auto n=raw[i];nodes.push_back({{n.Loc.X,n.Loc.Y,n.Loc.Z},double(n.Delay),n.ContinentID,n.Flags,n.ArrivalEventID,n.DepartureEventID,uint32(i+1)});}
    auto edited=EditableNodes(entry);
    for(auto node:nodes) for(uint32 phase=0;phase<2;++phase) {
        uint32 event=phase?node.departure:node.arrival;if(!event) continue;
        for(auto const& n:edited) if((phase?n.departure:n.arrival)==event) {node=n;break;}
        auto q=WorldDatabase.PQuery("SELECT id,action_type,action_param1,target_type,target_param1 FROM smart_scripts WHERE source_type=1 AND entryorguid={} AND event_type=71 AND event_param1={} ORDER BY id",entry,event);
        if(!q) continue;
        do {auto f=q->Fetch();uint32 id=f[0].GetUInt32(),kind=f[1].GetUInt32(),param=f[2].GetUInt32();
            if(kind==1 && f[3].GetUInt32()==19) OriginalTalk(result,entry,node,phase,entry,1,id,param,f[4].GetUInt32(),0);
            if(kind==80) {
                auto timed=WorldDatabase.PQuery("SELECT id,event_param1,event_param2,action_type,action_param1,target_type,target_param1 FROM smart_scripts WHERE source_type=9 AND entryorguid={} ORDER BY id",param);
                uint32 delay=0;
                if(timed) do {auto t=timed->Fetch();if(t[1].GetUInt32()!=t[2].GetUInt32()) break;delay+=t[1].GetUInt32();if(t[3].GetUInt32()==1 && t[5].GetUInt32()==19) OriginalTalk(result,entry,node,phase,param,9,t[0].GetUInt32(),t[4].GetUInt32(),t[6].GetUInt32(),delay);} while(timed->NextRow());
            }
        } while(q->NextRow());
    }return result;
}
static bool SameLegacy(Announcement const& a,Announcement const& b) {return a.script && a.script==b.script && a.type==b.type && a.action==b.action;}
static std::vector<Announcement> AllAnnouncements(uint32 entry) {
    auto stored=StoredAnnouncements(entry);auto originals=OriginalAnnouncements(entry);
    for(auto const& a:originals) if(std::none_of(stored.begin(),stored.end(),[&](auto const& s){return SameLegacy(a,s);})) stored.push_back(a);
    for(auto& a:stored) if(a.name.empty()) if(auto q=WorldDatabase.PQuery("SELECT t.name FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.guid={}",a.spawn)) a.name=q->Fetch()[0].GetString();
    return stored;
}
static void ListAnnouncements(ChatHandler* h,uint32 entry) {
    h->PSendSysMessage("[TR:ALIST:%u]",entry);uint32 key=0;
    for(auto const& a:AllAnnouncements(entry)) {
        ++key;h->PSendSysMessage("[TR:ANN:%u:%u:%u:%u:%u:%u:%u:%u:%llu:%u:%u:%u:%u:%u:%u]",entry,key,a.id,a.revision,a.uid,a.phase,a.delay,a.mode,static_cast<unsigned long long>(a.spawn),a.map,a.script,a.type,a.action,a.enabled?1:0,a.id?0:1);
        auto text=Hex(a.text);for(size_t i=0;i<text.size();i+=160) h->PSendSysMessage("[TR:ATEXT:%u:%u:%s]",entry,key,text.substr(i,160).c_str());
        h->PSendSysMessage("[TR:ANAME:%u:%u:%s]",entry,key,Hex(a.name).c_str());
    }h->PSendSysMessage("[TR:ALISTEND:%u]",entry);
}
void LoadAnnouncements() {
    std::lock_guard<std::recursive_mutex> lock(guard);
    announcements=StoredAnnouncements();announcementNodes.clear();
    for(auto const& a:announcements) if(!announcementNodes.count(a.entry)) {auto nodes=EditableNodes(a.entry);auto& ids=announcementNodes[a.entry];for(size_t i=0;i<nodes.size();++i) ids.push_back(nodes[i].uid?nodes[i].uid:uint32(i+1));}
    TC_LOG_INFO("entities.transport","Loaded {} GM transport announcement overrides",announcements.size());
}
bool SuppressTalk(uint32 entry,int32 script,uint32 type,uint32 action) {
    for(auto const& a:announcements) if(a.entry==entry && a.script==uint32(script) && a.type==type && a.action==action && a.script) return true;
    return false;
}
static void QueueAnnouncements(uint32 entry,uint32 uid,bool departure,uint64 lateness=0) {
    std::lock_guard<std::mutex> lock(announcementMutex);
    for(auto const& a:announcements) if(a.enabled && a.entry==entry && a.uid==uid && a.phase==uint32(departure)) {
        if(pendingSpeech.size()<4096) pendingSpeech.push_back({AnnNow()+(a.delay>lateness?a.delay-lateness:0),a});
    }
}
void NativeAnnouncement(uint32 entry,uint32 node,bool departure) {
    auto i=announcementNodes.find(entry);if(i!=announcementNodes.end() && node<i->second.size()) QueueAnnouncements(entry,i->second[node],departure);
}
void CustomAnnouncements(uint32 entry,uint32 oldTime,uint32 diff) {
    std::lock_guard<std::recursive_mutex> lock(guard);auto i=records.find(entry);if(i==records.end()) return;
    auto const& r=i->second->route;
    if(!r.period) return;
    // Include the left boundary, exclude the right: no duplicate at exact node times or wraps.
    for(size_t n=0;n<r.nodes.size();++n) for(uint32 phase=0;phase<2;++phase) {
        double at=phase?r.departures[n]:r.starts[n];double delta=std::fmod(at-oldTime+r.period,double(r.period));
        if(delta<diff) QueueAnnouncements(entry,r.nodes[n].uid?r.nodes[n].uid:uint32(n+1),phase!=0,uint64(diff-delta));
    }
}
void TickAnnouncements() {
    std::vector<Announcement> ready;
    {std::lock_guard<std::mutex> lock(announcementMutex);uint64 now=AnnNow();for(auto i=pendingSpeech.begin();i!=pendingSpeech.end();) if(i->due<=now){ready.push_back(i->a);i=pendingSpeech.erase(i);}else ++i;}
    for(auto const& a:ready) if(auto map=sMapMgr->FindBaseNonInstanceMap(a.map)) if(auto npc=map->GetCreatureBySpawnId(a.spawn)) if(npc->IsAlive()) {
        if(a.mode) npc->Yell(a.text,LANG_UNIVERSAL);else npc->Say(a.text,LANG_UNIVERSAL);
    }
}
struct AnnDraft {uint32 token=0,routeRevision=0;uint64 started=0;Announcement a;};
static std::map<uint32,AnnDraft> annDrafts;
bool AnnouncementCommand(ChatHandler* h,char const* args) {
    std::lock_guard<std::recursive_mutex> lock(guard);
    auto error=[&](char const* text) {h->PSendSysMessage("[TR:AERROR:%s]",text);return true;};
    std::istringstream in(args?args:"");std::string command;in>>command;
    if(command=="audit") {uint32 entry;if(!(in>>entry)) return error("Entry requis / Entry required");auto all=AllAnnouncements(entry);
        TC_LOG_INFO("entities.transport","GM announcement audit {}: {} entries",entry,all.size());
        for(auto const& a:all) TC_LOG_INFO("entities.transport","GM announcement audit route {} uid {} NPC {} delay {} phase {} original {}",entry,a.uid,a.spawn,a.delay,a.phase,a.id==0);
        ListAnnouncements(h,entry);return true;
    }
    if(!h->GetSession()) return error("Utiliser audit depuis la console / Use audit from console");
    uint32 account=h->GetSession()->GetAccountId();
    if(command=="list") {uint32 entry;if(!(in>>entry)||EditableNodes(entry).empty()) return error("Transport introuvable / Transport not found");ListAnnouncements(h,entry);return true;}
    if(command=="begin") {
        AnnDraft d;uint32 enabled=1,selected=0;
        auto& a=d.a;
        if(!(in>>d.token>>a.entry>>d.routeRevision>>a.id>>a.revision>>a.uid>>a.phase>>a.delay>>a.mode>>a.script>>a.type>>a.action>>enabled>>selected)) return error("Requete incomplete / Incomplete request");
        if(Revision(a.entry)!=d.routeRevision || a.phase>1 || a.delay>600000 || a.mode>1 || enabled>1 || selected>1) return error("Valeurs ou revision invalides / Invalid values or revision");
        a.enabled=enabled!=0;
        auto all=AllAnnouncements(a.entry);auto old=std::find_if(all.begin(),all.end(),[&](auto const& v){return a.id?v.id==a.id:SameLegacy(a,v);});
        if(a.id||a.script) {if(old==all.end()||old->revision!=a.revision) return error("Recharge les annonces / Reload announcements");a.spawn=old->spawn;a.map=old->map;a.script=old->script;a.type=old->type;a.action=old->action;}
        else if(a.revision || a.type || a.action) return error("Identite invalide / Invalid identity");
        uint32 map=0;
        if(a.enabled && !AnnNode(a.entry,a.uid,map)) return error("Choisis un point de passage, pas un point technique / Select a travel node, not a spline control point");
        if(selected) {auto npc=h->getSelectedCreature();if(!npc || !npc->GetSpawnId()) return error("Selectionne un PNJ permanent / Select a permanent NPC");a.spawn=npc->GetSpawnId();a.map=npc->GetMapId();}
        if(a.enabled && sMapStore.LookupEntry(map) && sMapStore.LookupEntry(map)->Instanceable()) return error("Annonces limitees aux cartes exterieures / Announcements require an outdoor map");
        if(!a.spawn || (a.enabled && map!=a.map)) return error("PNJ et point doivent etre sur la meme carte / NPC and node must share a map");
        if(annDrafts.size()>128) annDrafts.clear();d.started=AnnNow();annDrafts[account]=d;h->PSendSysMessage("[TR:AACK:%u:0]",d.token);return true;
    }
    uint32 token;if(!(in>>token)) return error("Commande invalide / Invalid command");
    auto i=annDrafts.find(account);if(i==annDrafts.end()||i->second.token!=token||AnnNow()-i->second.started>120000) return error("Envoi expire / Upload expired");
    auto& d=i->second;
    if(command=="text") {std::string hex;uint32 offset;if(!(in>>offset>>hex)||offset!=d.a.text.size()||!Unhex(hex,d.a.text)||d.a.text.size()>1000){annDrafts.erase(i);return error("Texte invalide (1000 octets max., sans codes couleur) / Invalid text");}h->PSendSysMessage("[TR:AACK:%u:%u]",token,uint32(d.a.text.size()));return true;}
    if(command!="commit") return error("Commande inconnue / Unknown command");
    Announcement a=d.a;std::wstring unicode;
    if(!Utf8toWStr(a.text,unicode)) {annDrafts.erase(i);return error("Texte UTF-8 invalide / Invalid UTF-8 text");}
    if(Revision(a.entry)!=d.routeRevision || a.text.empty()) {annDrafts.erase(i);return error("Recharge le trajet ou renseigne le texte / Reload route or enter text");}
    if(a.id) {auto q=WorldDatabase.PQuery("SELECT revision FROM gm_transport_announcement WHERE id={}",a.id);if(!q||q->Fetch()[0].GetUInt32()!=a.revision) return error("Annonce modifiee entre-temps / Announcement changed");}
    else if(a.script) {auto all=StoredAnnouncements(a.entry);for(auto const& v:all) if(SameLegacy(a,v)) return error("Recharge les annonces / Reload announcements");}
    auto escaped=a.text;WorldDatabase.EscapeString(escaped);
    if(a.id) WorldDatabase.DirectPExecute("UPDATE gm_transport_announcement SET node_uid={},phase={},delay_ms={},spawn_id={},map={},mode={},text='{}',enabled={},revision=revision+1 WHERE id={} AND revision={}",a.uid,a.phase,a.delay,a.spawn,a.map,a.mode,escaped,a.enabled?1:0,a.id,a.revision);
    else WorldDatabase.DirectPExecute("INSERT INTO gm_transport_announcement(entry,node_uid,phase,delay_ms,spawn_id,map,mode,text,legacy_script,legacy_type,legacy_id,enabled) VALUES ({},{},{},{},{},{},{},'{}',{},{},{},{})",a.entry,a.uid,a.phase,a.delay,a.spawn,a.map,a.mode,escaped,a.script,a.type,a.action,a.enabled?1:0);
    auto verify=StoredAnnouncements(a.entry);bool saved=std::any_of(verify.begin(),verify.end(),[&](auto const& v){return (a.id?v.id==a.id:(a.script?SameLegacy(a,v):v.uid==a.uid && v.spawn==a.spawn)) && v.text==a.text && v.revision==a.revision+1;});
    annDrafts.erase(i);if(!saved) return error("Echec base de donnees / Database save failed");
    h->PSendSysMessage("[TR:ASAVED:%u]",a.entry);ListAnnouncements(h,a.entry);return true;
}
