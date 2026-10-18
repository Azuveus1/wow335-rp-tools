#include "Tools/GMTransport/shared/NativeRoute.h"
#include "GMTransport.h"
#include "Engine/Source/Event/Event.h"
#include "Tools/GMTransport/shared/RouteMath.h"
#include "Tools/GMTransport/shared/RouteProjection.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapDoodadDef.h"
#include "WoW/Source/WorldClient/MapObjDef.h"
#include <unordered_map>
#include <cstring>
#include "WoW/Source/Game/ShipPath.h"
#include "WoW/Source/DB/Rec/TaxiPathNodeRec.h"
namespace {
std::unordered_map<uint32_t,GMRoute::Route> routes;
GMRoute::Route draft;
struct Native { uint32_t path=0,period=0,revision=0; float speed=10,accel=1; std::vector<TaxiPathNodeRec> nodes; };
std::unordered_map<uint32_t,Native> nativeRoutes;
uint32_t nativeRevision=0;
bool draftNative=false;
Native nativeDraft;
ShipPath nativePreview;
CMapBaseObj* preview=nullptr;
int32_t previewDisplay=0;
bool previewWmo=false;
float previewScale=1;
double previewTime=0;
struct Marker { CMapBaseObj* object=nullptr; GMRoute::Vec p; };
std::vector<Marker> flags;
void ClearShip() { if(preview) CWorld::ObjectDestroy(preview); preview=nullptr; }
void UpdateFlags() {
    while(flags.size()>draft.nodes.size()) {
        if(flags.back().object) CWorld::ObjectDestroy(flags.back().object);
        flags.pop_back();
    }
    flags.resize(draft.nodes.size());
    const char* name="World\\Generic\\PVP\\BattlefieldBanners\\BattlefieldBannerAllianceLargeBW.mdx";
    for(size_t i=0;i<flags.size();++i) {
        auto& flag=flags[i]; auto p=draft.nodes[i].p;
        if((draftNative ? nativeDraft.nodes[i].m_continentID : int(draft.NodeMap(i)))!=World::QueryMapId()) {
            if(flag.object) CWorld::ObjectDestroy(flag.object);flag.object=nullptr;continue;
        }
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)) continue;
        if(!flag.object) {
            const int16_t set=0; float radius=0;
            flag.object=CWorld::ObjectCreate(name,C3Vector(float(p.x),float(p.y),float(p.z)),0,0,0,0,1,&set,&radius,0);
        }
        if(flag.object) {
            C44Matrix m; m.Identity(); m.Scale(0.6f);
            m.d0=float(p.x);m.d1=float(p.y);m.d2=float(p.z);
            auto doodad=static_cast<CMapDoodadDef*>(flag.object);
            CMap::UpdateDoodadDefTransform(doodad,m);
            World::ObjectEnableCollision(flag.object,0);
            flag.p=p;
        }
    }
}
bool ReadRoute(lua_State* L,int table,GMRoute::Route& r) {
    luaL_checktype(L,table,LUA_TTABLE);
    size_t count=lua_objlen(L,table); if(count>128) return false;
    r.nodes.clear();
    for(size_t i=1;i<=count;++i) {
        lua_rawgeti(L,table,int(i)); if(!lua_istable(L,-1)) { lua_pop(L,1); return false; }
        double v[4]={};
        for(int j=1;j<=4;++j) { lua_rawgeti(L,-1,j); v[j-1]=lua_tonumber(L,-1); lua_pop(L,1); }
        lua_rawgeti(L,-1,5);uint32_t map=lua_isnumber(L,-1)?uint32_t(lua_tointeger(L,-1)):r.map;lua_pop(L,1);
        lua_pop(L,1); r.nodes.push_back({{v[0],v[1],v[2]},v[3],map});
    }
    return r.Build();
}
int Define(lua_State* L) {
    uint32_t entry=uint32_t(luaL_checkinteger(L,1));
    GMRoute::Route r; r.map=uint32_t(luaL_checkinteger(L,2)); r.speed=luaL_checknumber(L,3);
    if(entry<9000000 || entry>9999999 || (routes.size()>=128 && !routes.count(entry)) || !ReadRoute(L,4,r)) { lua_pushboolean(L,0); return 1; }
    routes[entry]=r; lua_pushboolean(L,1); return 1;
}
int PlayerPosition(lua_State* L) {
    if(!CGGameUI::s_inWorld) return 0;
    auto p=static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(),TYPEMASK_PLAYER,__FILE__,__LINE__));
    if(!p) return 0;
    auto v=p->GetPosition(); lua_pushinteger(L,World::QueryMapId()); lua_pushnumber(L,v.x); lua_pushnumber(L,v.y); lua_pushnumber(L,v.z); return 4;
}
int Cursor(lua_State* L) {
    double z=luaL_checknumber(L,1);
    auto frame=CGWorldFrame::s_currentWorldFrame; auto device=g_theGxDevicePtr;
    if(!CGGameUI::s_inWorld || !frame || !device || !std::isfinite(z)) return 0;
    float x,y; EventInputGetMousePosition(&x,&y);
    C44Matrix savedP=device->m_projection,savedV=device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];
    frame->m_camera->SetupWorldProjection(frame->m_viewportDDC);
    C3Vector a,b; bool ray=frame->GetLineSegment(x,y,&a,&b)!=0;
    device->XformSetProjection(savedP); device->XformSetView(savedV);
    if(!ray || std::abs(b.z-a.z)<0.00001) return 0;
    double t=(z-a.z)/(b.z-a.z); if(t<0 || t>1) return 0;
    double px=a.x+(b.x-a.x)*t,py=a.y+(b.y-a.y)*t;
    if(!std::isfinite(px)||!std::isfinite(py)||std::abs(px)>17000||std::abs(py)>17000) return 0;
    lua_pushnumber(L,px); lua_pushnumber(L,py); return 2;
}
int SetDraft(lua_State* L) {
    ClearShip(); draftNative=false;
    draft.map=uint32_t(luaL_checkinteger(L,1)); draft.speed=luaL_checknumber(L,2);
    bool valid=ReadRoute(L,3,draft);
    previewDisplay=int32_t(luaL_checkinteger(L,4)); previewScale=float(luaL_checknumber(L,5)); previewTime=0;
    lua_pushboolean(L,valid); lua_pushnumber(L,draft.period/1000.0); return 2;
}
bool ReadNative(lua_State* L,int table,Native& r) {
    luaL_checktype(L,table,LUA_TTABLE);size_t count=lua_objlen(L,table);
    if(count<4 || count>1024 || !std::isfinite(r.speed)||r.speed<1||r.speed>40||!std::isfinite(r.accel)||r.accel<=0) return false;
    r.nodes.clear();r.nodes.resize(count);
    for(size_t i=0;i<count;++i) {
        lua_rawgeti(L,table,int(i+1));if(!lua_istable(L,-1)){lua_pop(L,1);return false;}
        double v[8];for(int j=0;j<8;++j){lua_rawgeti(L,-1,j+1);v[j]=lua_tonumber(L,-1);lua_pop(L,1);if(!std::isfinite(v[j])){lua_pop(L,1);return false;}}
        lua_pop(L,1);
        if(std::abs(v[0])>17000||std::abs(v[1])>17000||std::abs(v[2])>10000||v[3]<0||v[3]>600||v[4]<0||v[4]>10000) return false;
        auto& n=r.nodes[i];n.m_pathID=int(r.path);n.m_nodeIndex=int(i);n.m_loc[0]=float(v[0]);n.m_loc[1]=float(v[1]);n.m_loc[2]=float(v[2]);n.m_delay=int(v[3]);n.m_continentID=int(v[4]);n.m_flags=int(v[5]);n.m_arrivalEventID=int(v[6]);n.m_departureEventID=int(v[7]);
    }
    std::vector<GMRoute::Node> check;
    for(auto const& n:r.nodes) check.push_back({{n.m_loc[0],n.m_loc[1],n.m_loc[2]},double(n.m_delay),uint32_t(n.m_continentID),uint32_t(n.m_flags)});
    return GMRoute::ValidNative(check);
}
int DefineNative(lua_State* L) {
    uint32_t entry=uint32_t(luaL_checkinteger(L,1));Native r;
    r.path=uint32_t(luaL_checkinteger(L,2));r.speed=float(luaL_checknumber(L,3));r.accel=float(luaL_checknumber(L,4));r.period=uint32_t(luaL_checkinteger(L,5));
    bool valid=entry>0 && entry<9000000 && nativeRoutes.size()<1024 && ReadNative(L,6,r);
    if(valid){r.revision=++nativeRevision;nativeRoutes[entry]=r;}lua_pushboolean(L,valid);return 1;
}
int SetNativeDraft(lua_State* L) {
    Native r;r.path=uint32_t(luaL_checkinteger(L,1));r.speed=float(luaL_checknumber(L,2));r.accel=float(luaL_checknumber(L,3));
    bool valid=ReadNative(L,4,r);ClearShip();draftNative=valid;draft=GMRoute::Route();
    if(valid){nativeDraft=r;nativePreview.Init(r.path,r.speed,r.accel,nullptr,&nativeDraft.nodes);draft.period=nativePreview.m_totalPathTime;
        for(auto const& n:r.nodes) draft.nodes.push_back({{n.m_loc[0],n.m_loc[1],n.m_loc[2]},double(n.m_delay)});
    }
    previewDisplay=int32_t(luaL_checkinteger(L,5));previewScale=float(luaL_checknumber(L,6));previewTime=0;
    lua_pushboolean(L,valid);lua_pushnumber(L,draft.period/1000.0);return 2;
}
GMRoute::Pose PreviewPose() {
    if(!draftNative) return draft.At(previewTime);
    int32_t map;uint32_t sequence,leg;C3Vector p;float facing,roll,pitch;
    nativePreview.GetPosition(uint32_t(previewTime),0,&map,&sequence,&p,&facing,&leg,&roll,&pitch,1);
    return {{p.x,p.y,p.z},facing,false};
}
void Project(lua_State* L,C44Matrix const& matrix,GMRoute::Vec camera,GMRoute::Vec p,int index) {
    double x,y;
    if(GMRoute::Project(matrix,p,camera,x,y)) {
            lua_createtable(L,2,0); lua_pushnumber(L,x); lua_rawseti(L,-2,1); lua_pushnumber(L,y); lua_rawseti(L,-2,2); lua_rawseti(L,-2,index); return;
    }
    lua_pushboolean(L,0); lua_rawseti(L,-2,index);
}
int Tick(lua_State* L) {
    double elapsed=luaL_checknumber(L,1); bool play=lua_toboolean(L,2)!=0;
    if(!CGGameUI::s_inWorld || !CGWorldFrame::s_currentWorldFrame || !g_theGxDevicePtr) { GMTransportClearPreview(); return 0; }
    auto frame=CGWorldFrame::s_currentWorldFrame; auto device=g_theGxDevicePtr;
    const auto& origin=frame->m_camera->m_position;
    GMRoute::Vec camera{origin.x,origin.y,origin.z};
    UpdateFlags();
    C44Matrix savedP=device->m_projection, savedV=device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];
    frame->m_camera->SetupWorldProjection(frame->m_viewportDDC);
    C44Matrix matrix=device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level]*device->m_projection;
    device->XformSetProjection(savedP); device->XformSetView(savedV);
    lua_createtable(L,int(draft.nodes.size()),0);
    for(size_t i=0;i<draft.nodes.size();++i) {
        if((draftNative ? nativeDraft.nodes[i].m_continentID : int(draft.NodeMap(i)))!=World::QueryMapId()) {lua_pushboolean(L,0);lua_rawseti(L,-2,int(i+1));}
        else Project(L,matrix,camera,draft.nodes[i].p,int(i+1));
    }
    lua_newtable(L);
    if(draft.period) {
        int index=1;
        if(draftNative) {
            for(uint32_t i=0;i<nativePreview.m_legs.Count();++i) {
                auto& leg=nativePreview.m_legs[i];
                if(leg.m_mapId==World::QueryMapId()) for(int j=0;j<=256;++j) {
                    C3Vector p;leg.m_spline.Pos(j/256.0f,p,C3Spline::EVAL_ARCLENGTH);Project(L,matrix,camera,{p.x,p.y,p.z},index++);
                }
                lua_pushboolean(L,0);lua_rawseti(L,-2,index++);
            }
        } else {
            for(size_t i=0;i<draft.nodes.size();++i) {
                if(int(draft.NodeMap(i))==World::QueryMapId() && !draft.Transition(i)) {
                    for(int j=0;j<=32;++j) Project(L,matrix,camera,draft.Curve(i,j/32.0),index++);
                } else {lua_pushboolean(L,0);lua_rawseti(L,-2,index++);}
            }
        }
        if(play && std::isfinite(elapsed)) previewTime+=std::max(0.0,std::min(1.0,elapsed))*1000;
        if(play && !draftNative && int(draft.At(previewTime).map)!=World::QueryMapId()) play=false;
        if(play && draftNative) {
            int32_t map;uint32_t seq,leg;C3Vector p;float f,r,t;
            nativePreview.GetPosition(uint32_t(previewTime),0,&map,&seq,&p,&f,&leg,&r,&t,1);
            if(map!=World::QueryMapId()) play=false;
        }
        if(play && !preview) {
            auto rec=g_gameObjectDisplayInfoDB.GetRecord(previewDisplay);
            if(rec && rec->m_modelName && rec->m_modelName[0]) {
                const char* ext=std::strrchr(rec->m_modelName,'.'); previewWmo=ext&&_stricmp(ext,".wmo")==0;
                const int16_t set=0; float radius=0; auto pose=PreviewPose();
                preview=CWorld::ObjectCreate(rec->m_modelName,C3Vector(float(pose.p.x),float(pose.p.y),float(pose.p.z)),float(pose.facing),0,0,0,1,&set,&radius,0);
            }
        }
        if(!play) ClearShip();
        if(preview) {
            auto pose=PreviewPose(); C44Matrix m; m.Identity(); m.RotateAroundZ(float(pose.facing)); m.Scale(previewScale);
            m.d0=float(pose.p.x); m.d1=float(pose.p.y); m.d2=float(pose.p.z);
            if(previewWmo) CMapObjDef::UpdateMatrix(static_cast<CMapObjDef*>(preview),m);
            else CMap::UpdateDoodadDefTransform(static_cast<CMapDoodadDef*>(preview),m);
            World::ObjectEnableCollision(preview,0);
        }
    }
    return 2;
}
int Clear(lua_State*) { GMTransportClearPreview(); return 0; }
}
bool GMTransportPosition(uint32_t entry,uint32_t time,int32_t& map,C3Vector& position,float& facing) {
    auto i=routes.find(entry); if(i==routes.end()) return false;
    auto p=i->second.At(time); map=int32_t(p.map); position=C3Vector(float(p.p.x),float(p.p.y),float(p.p.z)); facing=float(p.facing); return true;
}
void GMTransportPrepareShip(uint32_t entry,ShipPath& path) {
    auto i=nativeRoutes.find(entry);if(i==nativeRoutes.end() || path.m_editorRevision==i->second.revision) return;
    auto const& r=i->second;path.Init(r.path,r.speed,r.accel,path.m_physics,&r.nodes);
    if(r.period) path.SetTotalPathTime(r.period);
    path.m_editorRevision=r.revision;
}
void GMTransportClearPreview() {
    ClearShip();
    for(auto& flag:flags) if(flag.object) CWorld::ObjectDestroy(flag.object);
    flags.clear();
}
void GMTransportRegister() {
    FrameScript_RegisterFunction("GMRouteNativeDefine",DefineNative);
    FrameScript_RegisterFunction("GMRouteNativeDraft",SetNativeDraft);
    FrameScript_RegisterFunction("GMRouteCursor",Cursor);
    FrameScript_RegisterFunction("GMRouteDefine",Define); FrameScript_RegisterFunction("GMRoutePosition",PlayerPosition);
    FrameScript_RegisterFunction("GMRouteDraft",SetDraft); FrameScript_RegisterFunction("GMRouteTick",Tick); FrameScript_RegisterFunction("GMRouteClear",Clear);
}
void GMTransportUnregister() {
    FrameScript_UnregisterFunction("GMRouteNativeDefine");FrameScript_UnregisterFunction("GMRouteNativeDraft");
    nativeRoutes.clear();draftNative=false;
    FrameScript_UnregisterFunction("GMRouteCursor");
    GMTransportClearPreview(); routes.clear(); draft=GMRoute::Route();
    FrameScript_UnregisterFunction("GMRouteDefine"); FrameScript_UnregisterFunction("GMRoutePosition"); FrameScript_UnregisterFunction("GMRouteDraft"); FrameScript_UnregisterFunction("GMRouteTick"); FrameScript_UnregisterFunction("GMRouteClear");
}
