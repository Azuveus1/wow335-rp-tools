#include <cstdint>
#include <cassert>
#include <string>
#include <iostream>
struct WOWGUID{uint64_t m_guid=0;};
using NETMESSAGE=int;
constexpr int TYPEMASK_UNIT=1,TYPEMASK_PLAYER=2,SCRIPT_EVENT_UNIT_NAME_UPDATE=144,SCRIPT_EVENT_PLAYER_TOTEM_UPDATE=999;
int regenerated=0,portraits=0,tooltips=0,plates=0,consumers=0;
void PlayerNameTriggerNameRegenerate(void*){++regenerated;}
void Script_SendUnitSignal(WOWGUID,int event){assert(event==SCRIPT_EVENT_UNIT_NAME_UPDATE);++portraits;}
void FrameScript_SignalEvent(int,char const*,int){}
struct Plate {void UpdateNameDisplay(void*){++plates;}};
struct CGUnit_C {void* m_nameDesc=this;Plate* m_namePlateFrame=nullptr;void UpdateUnitNameText();void DestroyNamePlate(){m_namePlateFrame=nullptr;}const char* GetUnitName(void*,int){return "Alice Clairbois";}};
CGUnit_C unit;bool visible=true;
void* ClntObjMgrObjectPtr(WOWGUID,int,char const*,int){return visible?&unit:nullptr;}
struct TotemInfo {std::string m_name;int m_slot=0;};
struct CGGameUI {static void InvalidatePlayer(WOWGUID const&);static void UnitNameUpdate(WOWGUID const&);static void UnitTooltipUpdate(WOWGUID){++tooltips;}static TotemInfo* GetTotemInfo(WOWGUID){return nullptr;}};
struct CGGuildInfo{static void InvalidatePlayer(WOWGUID){++consumers;}};
struct CGCalendar{static void InvalidatePlayer(WOWGUID){++consumers;}};
struct CGAutoCompleteName{static void RefreshName(WOWGUID){++consumers;}};
void NameQueryCallback(int32_t,WOWGUID const&,void*,bool);
struct NameCache{};
struct Cache {
 using Callback=void(*)(int32_t,WOWGUID const&,void*,bool);
 bool loaded=false;int requests=0,invalidations=0;Callback callback=nullptr;WOWGUID requested;NameCache record;
 void Invalidate(uint64_t){loaded=false;++invalidations;}
 NameCache* GetRecord(uint64_t,WOWGUID g,Callback cb,void*,bool){if(loaded)return &record;++requests;requested=g;callback=cb;return nullptr;}
 void Complete(bool success=true){loaded=success;auto cb=callback;callback=nullptr;if(cb)cb(0,requested,nullptr,success);}
}g_nameDBCache;
struct {int invalidations=0;void Invalidate(int32_t){++invalidations;}}g_guildInfoCache;
struct CDataStore{uint64_t guid;void Get(uint64_t& value){value=guid;}};
#include "LiveNamesUnderTest.inc"
void reset(){g_nameDBCache={};regenerated=portraits=tooltips=plates=consumers=0;visible=true;}
int main(){
 reset();g_nameDBCache.loaded=true;CDataStore packet{5};ReceiveInvalidatePlayer(nullptr,0,0,&packet);
 assert(g_nameDBCache.invalidations==1&&g_nameDBCache.requests==1&&portraits==0&&regenerated==0&&consumers==3);
 g_nameDBCache.Complete();assert(regenerated==1&&portraits==1&&tooltips==1);
 reset();visible=false;ReceiveInvalidatePlayer(nullptr,0,0,&packet);assert(g_nameDBCache.requests==0&&portraits==0&&consumers==3);
 reset();ReceiveInvalidatePlayer(nullptr,0,0,&packet);visible=false;g_nameDBCache.Complete();assert(portraits==0&&regenerated==0);
 reset();ReceiveInvalidatePlayer(nullptr,0,0,&packet);g_nameDBCache.Complete(false);assert(portraits==0&&regenerated==0);
 reset();g_nameDBCache.loaded=true;CGGameUI::InvalidatePlayer(WOWGUID{5});assert(g_nameDBCache.requests==0&&portraits==1&&regenerated==1);
 reset();CDataStore guild{0x1FF0000000000001ULL};ReceiveInvalidatePlayer(nullptr,0,0,&guild);assert(g_guildInfoCache.invalidations==1&&g_nameDBCache.invalidations==0&&consumers==0);
 std::cout<<"PASS: actual invalidate packet handler, async name request/callback, overhead-name regeneration, tooltip and unit-frame events; absent/despawned players, failed queries, immediate-cache and guild-only paths.\n";
}
