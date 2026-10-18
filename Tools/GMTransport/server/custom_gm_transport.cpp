#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"
#include "WorldSession.h"
#include "RBAC.h"
#include "GMTransportRoute.h"
#include "GameTime.h"
#include <unordered_map>
using namespace Trinity::ChatCommands;
namespace {
struct Draft { uint32 token,source,count; time_t started; GMRoute::Route route; uint32 target=0,revision=0; bool native=false; };
std::unordered_map<uint32,Draft> drafts;
bool Error(ChatHandler* h) { h->SendSysMessage("[TR:ERROR] Invalid route/request, unsupported transport, wrong map, or database error."); h->SetSentErrorMessage(true); return false; }
class Commands : public CommandScript {
public:
 Commands():CommandScript("gm_transport_commands") {}
 ChatCommandTable GetCommands() const override {
    static ChatCommandTable sub={
      {"ann",GMTransport::AnnouncementCommand,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
      {"list",List,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::No},
      {"edit",Edit,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::No},
      {"beginupdate",BeginUpdate,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::No},
      {"begin",Begin,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::No},
      {"node",Node,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::No},
      {"commit",Commit,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::No},
      {"activate",Activate,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::No},
      {"deactivate",Deactivate,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::No},
      {"sync",Sync,rbac::RBAC_PERM_COMMAND_HELP,Console::No}};
    static ChatCommandTable root={{"gmroute",sub}}; return root;
 }
 static bool List(ChatHandler* h) { GMTransport::Catalogue(h->GetPlayer());return true; }
 static bool Edit(ChatHandler* h,uint32 entry) { return GMTransport::Edit(h->GetPlayer(),entry) || Error(h); }
 static bool BeginUpdate(ChatHandler* h,uint32 token,uint32 entry,uint32 revision,uint32 map,float speed,uint32 count) {
    bool native=entry<9000000;
    if(count<3 || count>(native?1024u:128u) || !std::isfinite(speed)||speed<1||speed>40 || GMTransport::Revision(entry)!=revision) return Error(h);
    if(drafts.size()>128) drafts.clear();
    Draft d{token,entry,count,GameTime::GetGameTime(),{}};
    d.target=entry;d.revision=revision;d.native=native;d.route.map=map;d.route.speed=speed;
    drafts[h->GetSession()->GetAccountId()]=d;h->PSendSysMessage("[TR:ACK:%u:0]",token);return true;
 }
 static bool Begin(ChatHandler* h,uint32 token,uint32 source,uint32 map,float speed,uint32 count) {
    auto p=h->GetPlayer();
    if(count<3 || count>128 || p->GetInstanceId() || !std::isfinite(speed) || speed<1 || speed>40) return Error(h);
    if(drafts.size()>128) drafts.clear();
    Draft d{token,source,count,GameTime::GetGameTime(),{}}; d.route.map=map; d.route.speed=speed;
    drafts[h->GetSession()->GetAccountId()]=d;
    h->PSendSysMessage("[TR:ACK:%u:0]",token); return true;
 }
 static bool Node(ChatHandler* h,uint32 token,uint32 index,float x,float y,float z,float wait,Optional<uint32> map,Optional<uint32> flags,Optional<uint32> arrival,Optional<uint32> departure,Optional<uint32> uid) {
    auto i=drafts.find(h->GetSession()->GetAccountId());
    if(i==drafts.end() || i->second.token!=token || GameTime::GetGameTime()-i->second.started>300) return Error(h);
    auto& d=i->second;
    if(index!=d.route.nodes.size()+1 || index>d.count || !std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||!std::isfinite(wait)) return Error(h);
    uint32 nodeMap=map.value_or(d.route.map);
    if(!d.native) { auto m=sMapStore.LookupEntry(nodeMap);if(!m || m->Instanceable()) return Error(h); }
    d.route.nodes.push_back({{x,y,z},wait,nodeMap,flags.value_or(0),arrival.value_or(0),departure.value_or(0),uid.value_or(index)}); h->PSendSysMessage("[TR:ACK:%u:%u]",token,index); return true;
 }
 static bool Commit(ChatHandler* h,uint32 token) {
    auto i=drafts.find(h->GetSession()->GetAccountId());
    if(i==drafts.end()||i->second.token!=token||i->second.route.nodes.size()!=i->second.count||GameTime::GetGameTime()-i->second.started>300) return Error(h);
    Draft d=i->second; drafts.erase(i);
    if(d.target) {
        bool ok=d.native?GMTransport::UpdateNative(d.target,d.revision,d.route.speed,d.route.nodes):GMTransport::Update(d.target,d.revision,d.route,h->GetPlayer());
        if(!ok) { h->SendSysMessage("[TR:ERROR] Update refused: reload latest revision, check values, and leave the transport (no passengers). Stay on its map for custom routes.");return false; }
        h->PSendSysMessage("[TR:UPDATED:%u:%u:%u:%u]",token,d.target,d.revision+1,d.native?1:0);return true;
    }
    if(!d.route.Build()) return Error(h);
    uint32 entry=GMTransport::Save(d.source,d.route); if(!entry) return Error(h);
    GMTransport::Sync(h->GetPlayer()); h->PSendSysMessage("[TR:SAVED:%u:%u]",token,entry); return true;
 }
 static bool Activate(ChatHandler* h,uint32 entry) {
    if(!GMTransport::Activate(entry,h->GetPlayer())) return Error(h);
    h->PSendSysMessage("[TR:ACTIVE:%u]",entry); return true;
 }
 static bool Sync(ChatHandler* h) { GMTransport::Sync(h->GetPlayer()); return true; }
 static bool Deactivate(ChatHandler* h,uint32 entry) {
    if(!GMTransport::Deactivate(entry,h->GetPlayer())) { h->SendSysMessage("[TR:ERROR] Cannot remove transport: leave it first and stay on the same map."); return false; }
    h->PSendSysMessage("[TR:INACTIVE:%u]",entry); return true;
 }
};
class Lifecycle : public WorldScript { public: Lifecycle():WorldScript("gm_transport_world"){} void OnStartup() override { GMTransport::Load(); GMTransport::LoadAnnouncements(); } void OnUpdate(uint32) override { GMTransport::TickAnnouncements(); } };
class Login : public PlayerScript { public: Login():PlayerScript("gm_transport_login"){} void OnLogin(Player* p,bool) override { GMTransport::Sync(p); } };
}
void AddSC_gm_transport() { new Commands(); new Lifecycle(); new Login(); }
