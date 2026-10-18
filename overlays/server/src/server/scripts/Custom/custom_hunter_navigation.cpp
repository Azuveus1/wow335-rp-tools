#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Chat.h"
#include "Player.h"
#include "WorldSession.h"
#include "RBAC.h"
#include "DatabaseEnv.h"
#include "Map.h"
#include "MapManager.h"
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "PathGenerator.h"
#include "DisableMgr.h"
#include "TemporarySummon.h"
#include "Log.h"
#include <mutex>
#include <cmath>
using namespace Trinity::ChatCommands;
namespace HunterProto {
constexpr uint32 Entry=9100100;
struct Point {Position p;UnitStandStateType seat=UNIT_STAND_STATE_SIT;};
Point inn{{1631.2f,-4438.2f,15.7436f,2.75762f}},flight{{1677.8f,-4316.0f,61.9445f,5.25344f},UNIT_STAND_STATE_STAND};
std::mutex configMutex;
ObjectGuid actor;
Point GetPoint(bool fly) {std::lock_guard<std::mutex> lock(configMutex);return fly?flight:inn;}
char const* StateName(uint32 state) {static char const* names[]={"resting","to_flightmaster","waiting_at_flightmaster","to_inn","blocked","stopped"};return state<6?names[state]:"unknown";}
struct AI : ScriptedAI {
    using ScriptedAI::ScriptedAI;
    uint32 state=0,tick=0,elapsed=0,legs=0;
    Point goal;
    bool walking=true,testing=false,returnScheduled=false;
    uint32 returnTimer=0;
    std::string reason;
    void Reset() override {
        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UnitFlags(UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC));
        me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        me->SetWalk(walking);me->SetStandState(GetPoint(false).seat);state=0;
    }
    void MoveInLineOfSight(Unit*) override {} // No target tracking or combat in this prototype.
    void Report(char const* phase) {
        TC_LOG_INFO("scripts","Hunter prototype {}: state={}, legs={}, x={:.3f} y={:.3f} z={:.3f} reason={}",phase,StateName(state),legs,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),reason);
    }
    bool Busy() const {return state==1||state==3;}
    void Block(std::string text) {
        reason=std::move(text);state=4;me->StopMoving();me->GetMotionMaster()->Clear();me->GetMotionMaster()->MoveIdle();Report("blocked");
        me->Say("Je ne trouve pas de passage praticable. Je m'arrete ici.",LANG_UNIVERSAL);
    }
    bool Leg() {
        if(!DisableMgr::IsPathfindingEnabled(me->GetMapId())) {Block("pathfinding_disabled");return false;}
        if(++legs>64) {Block("too_many_partial_paths");return false;}
        PathGenerator path(me);
        bool built=path.CalculatePath(goal.p.GetPositionX(),goal.p.GetPositionY(),goal.p.GetPositionZ(),false);
        // Funnel corners stay inside the navmesh corridor and avoid the smooth-path point limit.
        if(!built || (path.GetPathType()&PATHFIND_SHORT)) {
            path.SetUseStraightPath(true);built=path.CalculatePath(goal.p.GetPositionX(),goal.p.GetPositionY(),goal.p.GetPositionZ(),false);
        }
        uint32 flags=uint32(path.GetPathType());
        if(!built || (flags&(PATHFIND_SHORTCUT|PATHFIND_NOPATH|PATHFIND_NOT_USING_PATH|PATHFIND_SHORT)) || path.GetPath().size()<2) {Block("navmesh_refused_path_"+std::to_string(flags));return false;}
        auto const& end=path.GetPath().back();
        if(me->GetExactDist(end.x,end.y,end.z)<0.5f) {Block("partial_path_without_progress");return false;}
        Movement::MoveSplineInit move(me);move.MovebyPath(path.GetPath());move.SetWalk(walking);move.Launch();elapsed=0;
        TC_LOG_INFO("scripts","Hunter prototype leg {}: points={} flags={} end=({:.3f},{:.3f},{:.3f})",legs,path.GetPath().size(),flags,end.x,end.y,end.z);
        return true;
    }
    bool Start(bool fly,bool test=false) {
        if(testing && !test) return false;
        Point next=GetPoint(fly);
        if(me->GetMapId()!=1 || !next.p.IsPositionValid()) return false;
        me->StopMoving();me->GetMotionMaster()->Clear();me->GetMotionMaster()->MoveIdle();me->Dismount();me->SetStandState(UNIT_STAND_STATE_STAND);
        goal=next;state=fly?1:3;reason.clear();legs=0;tick=0;elapsed=0;returnScheduled=false;
        me->Say(fly?"Une piste ! Je rejoins le maitre de vol.":"Je retourne me reposer a l'auberge.",LANG_UNIVERSAL);
        Report("start");
        if(me->GetExactDist(&goal.p)<=1.0f) {Arrive();return true;}
        return Leg();
    }
    void Arrive() {
        bool fly=state==1;
        state=fly?2:0;reason.clear();me->SetFacingTo(goal.p.GetOrientation());
        if(!fly) {
            // Sitting down may align by a few centimetres, never bypass a travel segment.
            if(me->GetExactDist(&goal.p)<=1.5f) me->GetMap()->CreatureRelocation(me,goal.p.GetPositionX(),goal.p.GetPositionY(),goal.p.GetPositionZ(),goal.p.GetOrientation());
            me->SetStandState(goal.seat);
        }
        Report("arrived");
        me->Say(fly?"Je suis arrive. J'attends un nouveau renseignement.":"Je me repose en attendant une piste.",LANG_UNIVERSAL);
        if(testing && fly) {returnScheduled=true;returnTimer=1000;}
        else if(testing) {testing=false;Report("selftest_pass");}
    }
    void UpdateAI(uint32 diff) override {
        if(returnScheduled) {if(returnTimer>diff){returnTimer-=diff;return;}returnScheduled=false;Start(false,true);return;}
        if(!Busy()) return;
        elapsed+=diff;tick+=diff;
        if(elapsed>300000) {Block("travel_timeout");return;}
        if(tick<300) return;tick=0;
        if(!me->movespline->Finalized()) return;
        float planar=me->GetExactDist2d(&goal.p),vertical=std::abs(me->GetPositionZ()-goal.p.GetPositionZ());
        if(planar<=1.1f && vertical<=1.5f) Arrive();else Leg();
    }
    bool Allowed(Player* p) const {return p && p->GetSession()->HasPermission(rbac::RBAC_PERM_COMMAND_GOBJECT_ADD);}
    bool OnGossipHello(Player* p) override {
        if(!Allowed(p)) return false;
        ClearGossipMenuFor(p);
        AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Recevoir une piste : rejoindre le maitre de vol",GOSSIP_SENDER_MAIN,1);
        AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Retourner a l'auberge",GOSSIP_SENDER_MAIN,2);
        AddGossipItemFor(p,GOSSIP_ICON_CHAT,walking?"Se deplacer en courant":"Se deplacer en marchant",GOSSIP_SENDER_MAIN,3);
        AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Afficher mon etat",GOSSIP_SENDER_MAIN,4);
        SendGossipMenuFor(p,1,me);return true;
    }
    bool OnGossipSelect(Player* p,uint32 menu,uint32 index) override {
        if(!Allowed(p)) return false;
        uint32 action=GetGossipActionFor(p,index);ClearGossipMenuFor(p);CloseGossipMenuFor(p);
        if(action==1||action==2) Start(action==1);
        else if(action==3) {if(!Busy()){walking=!walking;me->SetWalk(walking);}else ChatHandler(p->GetSession()).SendSysMessage("Attends l'arrivee pour changer l'allure.");}
        else if(action==4) ChatHandler(p->GetSession()).PSendSysMessage("Mercenaire : %s. %s",StateName(state),reason.c_str());
        return true;
    }
};
Creature* Current() {
    auto map=sMapMgr->FindBaseNonInstanceMap(1);return map && !actor.IsEmpty()?map->GetCreature(actor):nullptr;
}
AI* CurrentAI() {auto c=Current();return c?dynamic_cast<AI*>(c->AI()):nullptr;}
Creature* Spawn() {
    if(auto c=Current()) return c;
    auto map=sMapMgr->CreateBaseMap(1);auto point=GetPoint(false);
    map->LoadGrid(point.p.GetPositionX(),point.p.GetPositionY());
    auto c=map->SummonCreature(Entry,point.p);if(!c)return nullptr;
    c->setActive(true);c->SetHomePosition(point.p);actor=c->GetGUID();
    TC_LOG_INFO("scripts","Hunter prototype spawned at inn");return c;
}
class Commands : public CommandScript {
public:
 Commands():CommandScript("hunter_navigation_prototype_commands"){}
 ChatCommandTable GetCommands() const override {
    static ChatCommandTable sub={
      {"spawn",SpawnCommand,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
      {"alert",Alert,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
      {"rest",Rest,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
      {"remove",Remove,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
      {"status",Status,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
      {"setrest",SetRest,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::No},
      {"setfly",SetFly,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::No},
      {"selftest",SelfTest,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes}};
    static ChatCommandTable root={{"hunterproto",sub}};return root;
 }
 static bool SpawnCommand(ChatHandler* h) {if(!Spawn()){h->SendSysMessage("Impossible de creer le prototype. Verifie son template.");return false;}h->SendSysMessage("Mercenaire present a l'auberge d'Orgrimmar. .hunterproto alert / rest / status / remove");return true;}
 static bool Alert(ChatHandler* h) {auto a=CurrentAI();if(!a){h->SendSysMessage("Utilise d'abord .hunterproto spawn.");return false;}return a->Start(true);}
 static bool Rest(ChatHandler* h) {auto a=CurrentAI();return a && a->Start(false);}
 static bool Remove(ChatHandler* h) {if(auto c=Current())c->DespawnOrUnsummon();actor.Clear();h->SendSysMessage("Prototype retire.");return true;}
 static bool Status(ChatHandler* h) {auto a=CurrentAI();if(!a){h->SendSysMessage("Aucun prototype present.");return true;}h->PSendSysMessage("Mercenaire : %s ; portions calculees : %u ; %s",StateName(a->state),a->legs,a->reason.c_str());a->Report("status");return true;}
 static bool SetPoint(ChatHandler* h,bool fly) {
    auto p=h->GetPlayer();if(!p||p->GetMapId()!=1||p->GetZoneId()!=1637||p->GetTransport()||!p->IsAlive()) {h->SendSysMessage("Place-toi a Orgrimmar, hors transport.");return false;}
    if(Current()){h->SendSysMessage("Retire d'abord le prototype avec .hunterproto remove.");return false;}
    Point point{p->GetPosition(),fly?UNIT_STAND_STATE_STAND:p->GetStandState()};
    if(point.seat!=UNIT_STAND_STATE_SIT_CHAIR && point.seat!=UNIT_STAND_STATE_SIT_LOW_CHAIR && point.seat!=UNIT_STAND_STATE_SIT_MEDIUM_CHAIR && point.seat!=UNIT_STAND_STATE_SIT_HIGH_CHAIR)point.seat=UNIT_STAND_STATE_SIT;
    WorldDatabase.DirectPExecute("REPLACE INTO hunter_proto_point(name,map,x,y,z,o,seat) VALUES ('{}',1,{},{},{},{},{})",fly?"flight":"inn",p->GetPositionX(),p->GetPositionY(),p->GetPositionZ(),p->GetOrientation(),uint32(point.seat));
    {std::lock_guard<std::mutex> lock(configMutex);(fly?flight:inn)=point;}
    h->SendSysMessage(fly?"Point du maitre de vol enregistre.":"Place de repos enregistree (position, orientation et assise).");return true;
 }
 static bool SetRest(ChatHandler* h) {return SetPoint(h,false);}
 static bool SetFly(ChatHandler* h) {return SetPoint(h,true);}
 static bool SelfTest(ChatHandler* h) {if(Current()){h->SendSysMessage("Un prototype existe deja ; test refuse.");return false;}auto c=Spawn();if(!c)return false;auto a=dynamic_cast<AI*>(c->AI());if(!a)return false;a->testing=true;a->walking=false;return a->Start(true,true);}
};
class ActorScript : public CreatureScript {public:ActorScript():CreatureScript("npc_hunter_navigation_proto"){} CreatureAI* GetAI(Creature* c) const override {return new AI(c);} };
class World : public WorldScript {public:World():WorldScript("hunter_navigation_prototype_world"){} void OnStartup() override {
    auto q=WorldDatabase.Query("SELECT name,x,y,z,o,seat FROM hunter_proto_point WHERE map=1");
    if(q)do {auto f=q->Fetch();Point p{{float(f[1].GetDouble()),float(f[2].GetDouble()),float(f[3].GetDouble()),float(f[4].GetDouble())},UnitStandStateType(f[5].GetUInt8())};if(!p.p.IsPositionValid()||p.seat>UNIT_STAND_STATE_SUBMERGED)continue;std::lock_guard<std::mutex> lock(configMutex);if(f[0].GetString()=="inn")inn=p;else if(f[0].GetString()=="flight")flight=p;}while(q->NextRow());
}};
}
#include "HunterContracts.inc"
void AddSC_hunter_navigation_prototype() {HunterContracts::Register();new HunterProto::Commands();new HunterProto::ActorScript();new HunterProto::World();}

