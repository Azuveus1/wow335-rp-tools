#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "CharacterCache.h"
#include "HunterCombat.h"
#include "HunterTravelEvents.h"
#include "HunterTaxi.h"
#include "Mail.h"
#include "GameTime.h"
#include "WorldPacket.h"
#include <unordered_map>

namespace HunterContracts {
bool CheckBoard(ChatHandler* h);
constexpr uint32 Entry=HunterCombat::Entry;
constexpr size_t MaxActors=8;
struct Contract {
    uint32 target=0, buyer=0, map=1, level=1, outcome=0;
    Position position;
    float health=1.0f;
    ObjectGuid actor;
    uint64 payment=0;
    uint32 stake=0,paymentState=0,pending=0;
    HunterTaxi::State travel;
};
std::recursive_mutex mutex;
std::unordered_map<uint32,Contract> contracts;
ObjectGuid TargetGuid(uint32 low) {return ObjectGuid::Create<HighGuid::Player>(low);}
Creature* Actor(Contract const& c) {
    auto map=sMapMgr->FindBaseNonInstanceMap(c.map);
    return map && !c.actor.IsEmpty()?map->GetCreature(c.actor):nullptr;
}
// Wait for the transaction worker's explicit result; the void DirectCommit API
// cannot distinguish a rollback. No in-memory mail is published before commit.
bool Commit(CharacterDatabaseTransaction tx) {return CharacterDatabase.AsyncCommitTransaction(tx).m_future.get();}
struct Notice {uint32 id=0,receiver=0,money=0;time_t now=0;std::string subject,body;};
Notice AddNotice(CharacterDatabaseTransaction tx,uint32 receiver,std::string subject,std::string body,uint32 money=0) {
    Notice n;if(!receiver)return n;
    n.id=sObjectMgr->GenerateMailID();n.receiver=receiver;n.money=money;n.now=GameTime::GetGameTime();n.subject=std::move(subject);n.body=std::move(body);
    auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_INS_MAIL);
    stmt->setUInt32(0,n.id);stmt->setUInt8(1,MAIL_CREATURE);stmt->setInt8(2,MAIL_STATIONERY_DEFAULT);stmt->setUInt16(3,0);
    stmt->setUInt32(4,Entry);stmt->setUInt32(5,receiver);stmt->setString(6,n.subject);stmt->setString(7,n.body);stmt->setBool(8,false);
    stmt->setUInt64(9,uint64(n.now+30*DAY));stmt->setUInt64(10,uint64(n.now));stmt->setUInt32(11,money);stmt->setUInt32(12,0);stmt->setUInt8(13,MAIL_CHECK_MASK_HAS_BODY);tx->Append(stmt);return n;
}
void Publish(Notice const& n) {
    if(!n.id)return;
    if(auto p=ObjectAccessor::FindConnectedPlayer(TargetGuid(n.receiver))) {
        auto m=new Mail{};m->messageID=n.id;m->messageType=MAIL_CREATURE;m->stationery=MAIL_STATIONERY_DEFAULT;m->sender=Entry;m->receiver=n.receiver;
        m->subject=n.subject;m->body=n.body;m->money=n.money;m->deliver_time=n.now;m->expire_time=n.now+30*DAY;m->checked=MAIL_CHECK_MASK_HAS_BODY;m->state=MAIL_STATE_UNCHANGED;
        p->AddMail(m);p->AddNewMailDeliverTime(n.now);
    }
}
void AppendSave(CharacterDatabaseTransaction tx,Contract const& c) {
    tx->PAppend("UPDATE hunter_proto_contract SET map={},x={},y={},z={},o={},level={},health={},outcome={} WHERE target={}",c.map,c.position.GetPositionX(),c.position.GetPositionY(),c.position.GetPositionZ(),c.position.GetOrientation(),c.level,c.health,c.outcome,c.target);
}
void PrepareCorpse(Contract const& c,Creature* actor) {
    actor->loot.clear();actor->loot.gold=c.stake;actor->loot.loot_type=LOOT_CORPSE;actor->loot.lootOwnerGUID=TargetGuid(c.target);
    actor->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);actor->RemoveUnitFlag(UNIT_FLAG_SKINNABLE);
    actor->SetCorpseDelay(DAY,true);actor->ResetPlayerDamageReq();
    if(auto p=ObjectAccessor::FindConnectedPlayer(TargetGuid(c.target)))actor->SetLootRecipient(p,false);
    actor->SetDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
}
void Save(Contract const& c) {
    auto tx=CharacterDatabase.BeginTransaction();AppendSave(tx,c);
    tx->PAppend("REPLACE INTO hunter_proto_pursuit(target,payment_id,clue_source,destination,phase,source,path,next_node,flight_to) VALUES ({},{},{},{},{},{},{},{},{})",c.target,c.payment,c.travel.clueSource,c.travel.destination,c.travel.phase,c.travel.source,c.travel.path,c.travel.nextNode,c.travel.flightTo);
    Commit(tx);
}
HunterTaxi::State ReadTravel(Creature* actor);
void Snapshot(Contract& c,Creature* actor) {
    c.map=actor->GetMapId();c.position=actor->GetPosition();c.level=actor->GetLevel();c.health=actor->GetHealthPct()/100.0f;
    c.travel=ReadTravel(actor);
}
void Finish(uint32 target,uint32 outcome) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it=contracts.find(target);if(it==contracts.end() || it->second.outcome)return;
    if(it->second.stake && !it->second.payment) {
        auto q=CharacterDatabase.PQuery("SELECT payment_id FROM hunter_proto_contract WHERE target={}",target);
        if(!q || !q->Fetch()[0].GetUInt64()){it->second.pending=outcome;return;}
        it->second.payment=q->Fetch()[0].GetUInt64();
    }
    auto& c=it->second;Contract next=c;next.outcome=outcome;next.pending=0;
    if(auto a=Actor(c))Snapshot(next,a);
    next.paymentState=outcome==2?(c.stake?1:2):outcome==3?4:3;
    auto tx=CharacterDatabase.BeginTransaction();AppendSave(tx,next);
    if(c.payment)tx->PAppend("UPDATE hunter_proto_payment SET state={} WHERE id={} AND state=0",next.paymentState,c.payment);
    std::vector<Notice> notices;
    if(c.buyer)notices.push_back(AddNotice(tx,c.buyer,outcome==1?"Contrat accompli":outcome==2?"Votre mercenaire a ete vaincu":"Contrat annule",
        outcome==1?"Le mercenaire a elimine sa cible. La prime reservee a ete versee au mercenaire.":outcome==2?"La cible a vaincu le mercenaire. Elle peut recuperer la prime sur son corps.":"Le contrat a ete annule. La prime reservee vous est restituee dans ce courrier.",outcome==3?c.stake:0));
    if(outcome==2)notices.push_back(AddNotice(tx,target,"Vous avez vaincu le mercenaire","Vous avez mis fin a la traque. Recuperez la prime de "+std::to_string(c.stake/10000)+" pieces d'or sur son corps."));
    if(!Commit(tx)){c.pending=outcome;return;}
    c=next;for(auto const& n:notices)Publish(n);
    HunterTravelEvents::Unwatch(TargetGuid(target).GetRawValue());
    if(auto a=Actor(c)) {a->CombatStop(true);if(outcome==2 && c.stake)PrepareCorpse(c,a);else a->DespawnOrUnsummon(Milliseconds(1000));}
    TC_LOG_INFO("scripts","Hunter contract target={} outcome={} (1=hunter victory, 2=target victory, 3=cancelled)",target,outcome);
    if(auto p=ObjectAccessor::FindConnectedPlayer(TargetGuid(target)))ChatHandler(p->GetSession()).SendSysMessage(outcome==1?"Contrat de test termine : le mercenaire vous a vaincu.":outcome==2?"Contrat de test termine : vous avez vaincu le mercenaire.":"Contrat de test annule.");
}
// The fixture-only path tests the same monetary transaction with offline disposable
// characters. Live loot always supplies a Player and passes the separate corpse gate.
bool CreditReward(Contract& c,Player* p=nullptr) {
    if(c.outcome!=2 || c.paymentState!=1 || !c.stake || !c.payment || (!p && c.target!=9000101))return false;
    uint32 oldMoney=0;
    if(p)oldMoney=p->GetMoney();
    else {auto q=CharacterDatabase.PQuery("SELECT money FROM characters WHERE guid={}",c.target);if(!q)return false;oldMoney=q->Fetch()[0].GetUInt32();}
    if(c.stake>MAX_MONEY_AMOUNT-oldMoney)return false;
    auto tx=CharacterDatabase.BeginTransaction();
    if(p){p->SetMoney(oldMoney+c.stake);p->SaveGoldToDB(tx);}else tx->PAppend("UPDATE characters SET money={} WHERE guid={}",oldMoney+c.stake,c.target);
    tx->PAppend("UPDATE hunter_proto_payment SET state=2 WHERE id={} AND state=1",c.payment);
    if(!Commit(tx)){if(p)p->SetMoney(oldMoney);return false;}
    c.paymentState=2;return true;
}
void Claim(Creature* corpse,ObjectGuid playerGuid) {
    auto p=ObjectAccessor::FindConnectedPlayer(playerGuid);
    if(!p || !p->IsAlive() || corpse->IsAlive() || !HunterCombat::IsTarget(corpse,p) || !corpse->IsWithinDistInMap(p,INTERACTION_DISTANCE) || p->GetLootGUID()!=corpse->GetGUID())return;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it=contracts.find(uint32(playerGuid.GetCounter()));if(it==contracts.end())return;
    auto& c=it->second;
    if(c.actor!=corpse->GetGUID() || corpse->loot.gold!=c.stake)return;
    if(!CreditReward(c,p)){ChatHandler(p->GetSession()).SendSysMessage("Prime non recuperee : verifiez votre limite d'or ou reessayez.");return;}
    corpse->loot.NotifyMoneyRemoved();corpse->loot.gold=0;
    WorldPacket data(SMSG_LOOT_MONEY_NOTIFY,5);data<<uint32(c.stake)<<uint8(1);p->SendDirectMessage(&data);
    p->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY,c.stake);
    p->GetSession()->DoLootRelease(corpse->GetGUID());corpse->DespawnOrUnsummon(Milliseconds(100));c.actor.Clear();
}
struct AI : ScriptedAI {
    explicit AI(Creature* owner):ScriptedAI(owner),taxi(owner) {taxi.orgFlight=HunterProto::GetPoint(true).p;}
    HunterTaxi::Driver taxi;
    bool travelTest=false,testFlew=false,testResumed=false;
    uint32 testFlightTime=0;
    uint32 pulse=0;
    ObjectGuid targetGuid;
    void SetGUID(ObjectGuid const& guid,int32 id) override {if(id==HunterCombat::TargetSlot)targetGuid=guid;else if(id==HunterCombat::ClaimSlot)Claim(me,guid);}
    ObjectGuid GetGUID(int32 id) const override {return id==HunterCombat::TargetSlot?targetGuid:ObjectGuid::Empty;}
    uint32 Target() const {return uint32(HunterCombat::Target(me).GetCounter());}
    void Reset() override {me->SetReactState(REACT_DEFENSIVE);me->SetStandState(UNIT_STAND_STATE_STAND);}
    bool CanAIAttack(Unit const* who) const override {return !taxi.FlyingNow() && HunterCombat::IsTarget(me,who);}
    void AttackStart(Unit* who) override {
        if(who && CanAIAttack(who) && me->IsValidAttackTarget(who) && me->CanSeeOrDetect(who) && me->IsWithinLOSInMap(who)){me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);ScriptedAI::AttackStart(who);}
    }
    bool MayTalk(Player* player) const {
        return player && player->IsAlive() && me->IsAlive() && !me->IsInCombat() && !targetGuid.IsEmpty()
            && !HunterCombat::IsTarget(me,player) && me->IsWithinDistInMap(player,5.0f) && me->IsWithinLOSInMap(player);
    }
    bool OnGossipHello(Player* player) override {
        ClearGossipMenuFor(player);
        if(!MayTalk(player)){CloseGossipMenuFor(player);return true;}
        AddGossipItemFor(player,GOSSIP_ICON_CHAT,"Qui traques-tu ?",GOSSIP_SENDER_MAIN,1);
        SendGossipMenuFor(player,1,me);return true;
    }
    bool OnGossipSelect(Player* player,uint32 /*menu*/,uint32 index) override {
        uint32 action=GetGossipActionFor(player,index);ClearGossipMenuFor(player);CloseGossipMenuFor(player);
        if(action!=1 || !MayTalk(player))return true;
        std::string name;
        if(sCharacterCache->GetCharacterNameByGuid(targetGuid,name))me->Whisper("Je traque une personne nommee "+name+".",LANG_UNIVERSAL,player);
        else me->Whisper("Je suis une piste, mais je n'ai aucun nom a te donner pour le moment.",LANG_UNIVERSAL,player);
        return true;
    }
    void MoveInLineOfSight(Unit* who) override {
        if(!me->GetVictim() && who && who->GetGUID()==HunterCombat::Target(me) && me->IsWithinDistInMap(who,25.0f))AttackStart(who);
    }
    void DamageTaken(Unit* attacker,uint32& damage,DamageEffectType,SpellInfo const*) override {
        if(taxi.FlyingNow() || !HunterCombat::IsTarget(me,attacker))damage=0;
        else if(!me->GetVictim())AttackStart(attacker);
    }
    void JustDied(Unit* killer) override {if(HunterCombat::IsTarget(me,killer))Finish(Target(),2);}
    void KilledUnit(Unit* victim) override {if(victim && victim->GetGUID()==HunterCombat::Target(me))Finish(Target(),1);}
    void EnterEvadeMode(EvadeReason why) override {
        // No full heal on evade: the contract owns health across suspension/restarts.
        if (!_EnterEvadeMode(why)) return; me->GetThreatManager().ClearAllThreat();me->GetMotionMaster()->MoveTargetedHome();
    }
    void UpdateAI(uint32 diff) override {
        auto player=ObjectAccessor::FindConnectedPlayer(HunterCombat::Target(me));
        taxi.allowDeparture=travelTest || (player && player->IsInWorld());taxi.Tick(diff);
        if(travelTest) {
            if(taxi.FlyingNow()){testFlew=true;testFlightTime+=diff;if(!testResumed && testFlightTime>4000){auto saved=taxi.Export();taxi.Restore(saved);testResumed=true;TC_LOG_INFO("scripts","Hunter taxi selftest resumed: node={} position=({},{},{})",saved.nextNode,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ());}}
            if(taxi.Export().phase==HunterTaxi::Waiting){auto node=sTaxiNodesStore.LookupEntry(22);bool landed=node && me->GetExactDist(node->Pos.X,node->Pos.Y,node->Pos.Z)<2.f && !me->IsMounted() && !me->HasUnitFlag(UNIT_FLAG_ON_TAXI);TC_LOG_INFO("scripts","Hunter taxi selftest {}: walked to Doras, flew actual DBC paths through Crossroads to Thunder Bluff, resumed spline, landed at destination",testFlew && testResumed && landed?"PASS":"FAIL");travelTest=false;me->DespawnOrUnsummon(Milliseconds(500));return;}
        }
        pulse+=diff;
        if(pulse>=1000) {
            pulse=0;
            if(me->IsInCombat() || taxi.Travelling())me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);else me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
            auto p=ObjectAccessor::FindConnectedPlayer(HunterCombat::Target(me));
            if(!p || !p->IsInWorld()) {me->CombatStop(true);return;}
            if(!taxi.FlyingNow() && !me->GetVictim() && me->IsWithinDistInMap(p,25.0f))AttackStart(p);
            if(!me->IsInCombat() && me->GetLevel()!=p->GetLevel()) {
                float health=me->GetHealthPct()/100.0f;me->SetLevel(p->GetLevel());me->UpdateLevelDependantStats();me->SetHealth(std::max(uint32(1),uint32(me->GetMaxHealth()*health)));
            }
        }
        if(UpdateVictim())DoMeleeAttackIfReady();
    }
};
HunterTaxi::State ReadTravel(Creature* actor) {if(auto ai=dynamic_cast<AI*>(actor->AI()))return ai->taxi.Export();return {};}
class ActorScript : public CreatureScript {
public:ActorScript():CreatureScript("npc_hunter_contract_proto"){}
CreatureAI* GetAI(Creature* c) const override {return new AI(c);}
};
void Update(bool checkpoint) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    for(auto const& event:HunterTravelEvents::Drain()) {
        auto it=contracts.find(uint32(event.target));
        if(it==contracts.end() || it->second.outcome || it->second.pending || it->second.payment!=event.generation)continue;
        auto& c=it->second;c.travel.clueSource=event.source;c.travel.destination=event.destination;
        if(auto actor=Actor(c)){if(auto ai=dynamic_cast<AI*>(actor->AI()))ai->taxi.Clue(event.source,event.destination);Snapshot(c,actor);}
        else if(c.travel.phase!=HunterTaxi::Flying)c.travel.phase=HunterTaxi::Idle;
        Save(c);TC_LOG_INFO("scripts","Hunter taxi clue accepted: target={} playerDeparture={} playerDestination={} generation={}",c.target,event.source,event.destination,event.generation);
    }
    size_t present=0;for(auto const& pair:contracts)if(Actor(pair.second))++present;
    for(auto& pair:contracts) {
        auto& c=pair.second;
        if(c.pending){Finish(c.target,c.pending);continue;}
        if(c.stake && !c.payment){auto q=CharacterDatabase.PQuery("SELECT payment_id FROM hunter_proto_contract WHERE target={}",c.target);if(!q)continue;c.payment=q->Fetch()[0].GetUInt64();if(!c.payment)continue;}
        if(!c.outcome)HunterTravelEvents::Watch(TargetGuid(c.target).GetRawValue(),c.payment);
        bool rewardCorpse=c.outcome==2 && c.paymentState==1 && c.stake;
        if(c.outcome && !rewardCorpse)continue;
        auto a=Actor(c);auto p=ObjectAccessor::FindConnectedPlayer(TargetGuid(c.target));
        bool online=p && p->IsInWorld();
        if(a) {
            // A death callback completes the contract before this snapshot.
            if(!a->IsAlive()) {
                if(!c.outcome){Finish(c.target,2);continue;}
                if(!online){a->DespawnOrUnsummon();c.actor.Clear();--present;}
                continue;
            }
            Snapshot(c,a);bool airborne=c.travel.phase==HunterTaxi::Flying;
            if(checkpoint || (!online && !airborne))Save(c);
            if(!online && !airborne) {a->CombatStop(true);a->DespawnOrUnsummon();c.actor.Clear();--present;}
        } else if(online && present<MaxActors) {
            auto map=sMapMgr->CreateBaseMap(c.map);if(!map || map->Instanceable())continue;
            map->LoadGrid(c.position.GetPositionX(),c.position.GetPositionY());
            a=map->SummonCreature(Entry,c.position);if(!a)continue;
            HunterCombat::SetTarget(a,TargetGuid(c.target));a->SetFaction(35);a->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
            a->RemoveUnitFlag(UnitFlags(UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC));
            a->SetCorpseDelay(DAY,true);
            a->SetLevel(rewardCorpse?c.level:p->GetLevel());a->UpdateLevelDependantStats();a->SetHealth(std::max(uint32(1),uint32(a->GetMaxHealth()*c.health)));
            a->SetHomePosition(c.position);a->setActive(true);c.actor=a->GetGUID();++present;
            if(rewardCorpse){a->setDeathState(JUST_DIED);PrepareCorpse(c,a);}else {if(auto ai=dynamic_cast<AI*>(a->AI()))ai->taxi.Restore(c.travel);Snapshot(c,a);Save(c);}
        }
    }
}
class World : public WorldScript {
    uint32 timer=0,checkpointTimer=0;
public:World():WorldScript("hunter_contract_prototype_world"){}
void OnStartup() override {
    auto q=CharacterDatabase.Query("SELECT c.target,c.buyer,c.map,c.x,c.y,c.z,c.o,c.level,c.health,c.outcome,c.payment_id,COALESCE(p.amount,0),COALESCE(p.state,0) FROM hunter_proto_contract c LEFT JOIN hunter_proto_payment p ON p.id=c.payment_id");
    if(q)do {auto f=q->Fetch();Contract c;c.target=f[0].GetUInt32();c.buyer=f[1].GetUInt32();c.map=f[2].GetUInt32();c.position.Relocate(float(f[3].GetDouble()),float(f[4].GetDouble()),float(f[5].GetDouble()),float(f[6].GetDouble()));c.level=f[7].GetUInt32();c.health=float(f[8].GetDouble());c.outcome=f[9].GetUInt32();c.payment=f[10].GetUInt64();c.stake=uint32(f[11].GetUInt64());c.paymentState=uint32(f[12].GetUInt64());if(c.position.IsPositionValid() && std::isfinite(c.health) && c.health>=0 && c.health<=1 && (c.outcome || c.health>0)){contracts.emplace(c.target,c);TC_LOG_INFO("scripts","Hunter contract restored target={} outcome={} health={} stake={} paymentState={}",c.target,c.outcome,c.health,c.stake,c.paymentState);}}while(q->NextRow());
    auto progress=CharacterDatabase.Query("SELECT target,payment_id,clue_source,destination,phase,source,path,next_node,flight_to FROM hunter_proto_pursuit");
    if(progress)do {auto f=progress->Fetch();auto it=contracts.find(f[0].GetUInt32());if(it==contracts.end() || it->second.payment!=f[1].GetUInt64())continue;
        it->second.travel={f[2].GetUInt32(),f[3].GetUInt32(),f[4].GetUInt32(),f[5].GetUInt32(),f[6].GetUInt32(),f[7].GetUInt32(),f[8].GetUInt32()};
    }while(progress->NextRow());
    for(auto const& pair:contracts)if(!pair.second.outcome)HunterTravelEvents::Watch(TargetGuid(pair.first).GetRawValue(),pair.second.payment);
}
void OnUpdate(uint32 diff) override {timer+=diff;checkpointTimer+=diff;if(timer>=2000){timer=0;bool checkpoint=checkpointTimer>=10000;if(checkpoint)checkpointTimer=0;Update(checkpoint);}}
void OnShutdown() override {std::lock_guard<std::recursive_mutex> lock(mutex);for(auto& pair:contracts)if(!pair.second.outcome){if(auto actor=Actor(pair.second))Snapshot(pair.second,actor);Save(pair.second);}}
};
class Commands : public CommandScript {
public:Commands():CommandScript("hunter_contract_prototype_commands"){}
ChatCommandTable GetCommands() const override {
    static ChatCommandTable sub={
        {"boardcheck",CheckBoard,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
        {"taxitest",TaxiTest,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
        {"cluetest",ClueTest,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
        {"financialselftest",FinanceSelfTest,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
        {"selftest",SelfTest,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
        {"contracts",List,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes},
        {"cancel",Cancel,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes}};
    static ChatCommandTable root={{"huntercontract",sub}};return root;
}
static bool SelfTest(ChatHandler* h) {
    auto map=sMapMgr->CreateBaseMap(1);auto point=HunterProto::GetPoint(false);
    map->LoadGrid(point.p.GetPositionX(),point.p.GetPositionY());
    auto hunter=map->SummonCreature(Entry,point.p);auto outsider=map->SummonCreature(HunterProto::Entry,point.p);
    if(!hunter || !outsider){if(hunter)hunter->DespawnOrUnsummon();if(outsider)outsider->DespawnOrUnsummon();return false;}
    HunterCombat::SetTarget(hunter,TargetGuid(4294967000u));hunter->SetLevel(30);hunter->UpdateLevelDependantStats();
    auto health=hunter->GetHealth();
    bool attacksBlocked=!outsider->IsValidAttackTarget(hunter) && !hunter->IsValidAttackTarget(outsider);
    uint32 damage=Unit::DealDamage(outsider,hunter,health+1,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
    Unit::Kill(outsider,hunter,false);
    bool passed=attacksBlocked && !damage && hunter->IsAlive() && hunter->GetHealth()==health && hunter->GetLevel()==30 && hunter->GetCreatureTemplate()->rank==1
        && hunter->GetCreatorGUID().IsEmpty() && HunterCombat::Target(hunter)==TargetGuid(4294967000u)
        && hunter->GetReactionTo(outsider)==REP_FRIENDLY && HunterCombat::ViewerFaction(hunter,outsider)==35;
    hunter->SetCorpseDelay(DAY,true);hunter->setDeathState(JUST_DIED);
    Contract reward;reward.target=4294967000u;reward.stake=2500000;PrepareCorpse(reward,hunter);
    passed=passed && hunter->isDead() && hunter->loot.gold==2500000 && hunter->loot.loot_type==LOOT_CORPSE
        && hunter->loot.items.empty() && hunter->GetLootRecipientGroup()==nullptr && hunter->HasDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
    hunter->DespawnOrUnsummon();outsider->DespawnOrUnsummon();
    h->SendSysMessage(passed?"Hunter combat selftest PASS":"Hunter combat selftest FAIL");
    TC_LOG_INFO("scripts","Hunter combat selftest {}: private attack gate, direct damage, instant kill, elite level stats",passed?"PASS":"FAIL");
    return passed;
}
static bool TaxiTest(ChatHandler* h) {
    if(h->GetPlayer())return false;
    for(uint32 destination:{22u,30u,40u,80u}) {
        auto route=HunterTaxi::Route(23,destination,1);
        if(route.size()<2){TC_LOG_ERROR("scripts","Hunter taxi graph FAIL: missing Horde itinerary 23 -> {}",destination);return false;}
        TC_LOG_INFO("scripts","Hunter taxi graph PASS: 23 -> {} nodes={}",destination,route.size());
    }
    if(!HunterTaxi::Route(23,26,1).empty()){TC_LOG_ERROR("scripts","Hunter taxi graph FAIL: Alliance node accepted");return false;}
    auto map=sMapMgr->CreateBaseMap(1);auto point=HunterProto::GetPoint(false);map->LoadGrid(point.p.GetPositionX(),point.p.GetPositionY());
    auto actor=map->SummonCreature(Entry,point.p);if(!actor)return false;
    actor->setActive(true);actor->SetCorpseDelay(DAY,true);actor->SetHomePosition(point.p);HunterCombat::SetTarget(actor,TargetGuid(4294967000u));
    auto ai=dynamic_cast<AI*>(actor->AI());if(!ai){actor->DespawnOrUnsummon();return false;}
    ai->travelTest=true;ai->taxi.Clue(40,22);
    TC_LOG_INFO("scripts","Hunter taxi selftest started: inn -> Doras -> Crossroads -> Thunder Bluff");return true;
}
static bool ClueTest(ChatHandler* h,uint32 stage) {
    if(h->GetPlayer())return false;
    auto fixture=CharacterDatabase.Query("SELECT guid FROM characters WHERE guid=9000101 AND account=0 AND name='Hunttarget'");if(!fixture)return false;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if(stage==1){if(!CreateInternal(h,"Hunttarget",0))return false;return HunterTravelEvents::Publish(TargetGuid(9000101).GetRawValue(),22,40,HORDE);}
    auto it=contracts.find(9000101);bool ok=it!=contracts.end() && !it->second.outcome && it->second.travel.clueSource==22 && it->second.travel.destination==40;
    TC_LOG_INFO("scripts","Hunter clue persistence {}: stored departure=22 destination=40",ok?"PASS":"FAIL");if(ok)Finish(9000101,3);return ok;
}
static bool CreateInternal(ChatHandler* h,std::string name,uint32 gold,bool fixture=false,std::string* boardError=nullptr) {
    auto reject=[&](char const* text){if(boardError)*boardError=text;h->SendSysMessage(text);return false;};
    if(!normalizePlayerName(name))return reject("Nom de joueur invalide.");
    CharacterDatabase.EscapeString(name);
    auto q=CharacterDatabase.PQuery("SELECT guid,level FROM characters WHERE name='{}' AND deleteDate IS NULL",name);
    if(!q){return reject("Joueur introuvable.");}
    uint32 target=q->Fetch()[0].GetUInt32(),level=q->Fetch()[1].GetUInt8();
    auto buyerPlayer=h->GetPlayer();
    if(gold>100000 || (buyerPlayer && !gold) || (!buyerPlayer && gold && !fixture)) {return reject("Prime : 1 a 100000 pieces d'or, depuis un personnage. Console : 0 pour les tests uniquement.");}
    uint32 buyer=buyerPlayer?uint32(buyerPlayer->GetGUID().GetCounter()):fixture?9000100:0;
    uint32 stake=gold*10000,oldMoney=buyerPlayer?buyerPlayer->GetMoney():0;
    if(fixture){
        if(buyerPlayer || target!=9000101)return false;
        auto f=CharacterDatabase.Query("SELECT money FROM characters WHERE guid=9000100 AND account=0 AND name='Huntbuyer'");
        if(!f)return false;oldMoney=f->Fetch()[0].GetUInt32();
    }
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it=contracts.find(target);
    if(it!=contracts.end() && (!it->second.outcome || it->second.paymentState==1)) {TC_LOG_INFO("scripts","Hunter contract rejected duplicate target={}",target);return reject("Desole, impossible : un contrat est deja en cours sur cette cible, ou sa prime reste a recuperer.");}
    auto reserved=CharacterDatabase.PQuery("SELECT COUNT(*) FROM hunter_proto_contract c LEFT JOIN hunter_proto_payment p ON p.id=c.payment_id WHERE c.target={} AND (c.outcome=0 OR p.state=1)",target);
    if(!reserved || reserved->Fetch()[0].GetUInt64()){return reject("Creation refusee : cible reservee ou base indisponible.");}
    if(contracts.size()>=1000 && it==contracts.end()){return reject("Limite de contrats du prototype atteinte.");}
    if(stake>oldMoney){return reject("Vous n'avez pas assez d'or pour reserver cette prime.");}
    Contract c;c.target=target;c.buyer=buyer;c.level=level;c.stake=stake;c.position=HunterProto::GetPoint(false).p;
    auto tx=CharacterDatabase.BeginTransaction();
    if(buyerPlayer){buyerPlayer->SetMoney(oldMoney-stake);buyerPlayer->SaveGoldToDB(tx);}
    else if(fixture)tx->PAppend("UPDATE characters SET money={} WHERE guid={}",oldMoney-stake,buyer);
    auto notice=AddNotice(tx,target,"Un contrat a ete mis sur votre tete",
        "Un mercenaire vous traque. Une prime de "+std::to_string(gold)+" pieces d'or a ete reservee. S'il vous tue, il remportera cette somme. Si vous le vainquez, vous pourrez recuperer la prime sur son corps. Seuls vous et vos familiers pouvez le blesser.");
    tx->PAppend("INSERT INTO hunter_proto_payment(target,buyer,amount,state,alert_mail) VALUES ({},{},{},0,{})",target,buyer,stake,notice.id);
    tx->PAppend("DELETE FROM hunter_proto_pursuit WHERE target={}",target);
    tx->PAppend("INSERT INTO hunter_proto_contract(target,buyer,map,x,y,z,o,level,health,outcome,payment_id) VALUES ({},{},1,{},{},{},{},{},1,0,LAST_INSERT_ID()) ON DUPLICATE KEY UPDATE buyer=VALUES(buyer),map=1,x=VALUES(x),y=VALUES(y),z=VALUES(z),o=VALUES(o),level=VALUES(level),health=1,outcome=0,payment_id=VALUES(payment_id)",target,c.buyer,c.position.GetPositionX(),c.position.GetPositionY(),c.position.GetPositionZ(),c.position.GetOrientation(),level);
    if(!Commit(tx)){if(buyerPlayer)buyerPlayer->SetMoney(oldMoney);return reject("Echec de la sauvegarde : aucun contrat cree, prime non prelevee.");}
    auto check=CharacterDatabase.PQuery("SELECT payment_id FROM hunter_proto_contract WHERE target={}",target);
    // A committed contract stays reserved if the follow-up read is temporarily unavailable.
    if(check)c.payment=check->Fetch()[0].GetUInt64();
    if(it!=contracts.end())if(auto previous=Actor(it->second))previous->DespawnOrUnsummon();
    contracts[target]=c;Publish(notice);HunterTravelEvents::Watch(TargetGuid(target).GetRawValue(),c.payment);
    if(auto player=ObjectAccessor::FindConnectedPlayer(TargetGuid(target)))ChatHandler(player->GetSession()).SendSysMessage("Attention : un contrat a ete mis sur votre tete. Consultez votre courrier.");
    TC_LOG_INFO("scripts","Hunter contract created target={} level={} stake={}",target,level,stake);
    h->PSendSysMessage("Contrat cree : %u pieces d'or reservees. La cible a recu un courrier.",gold);return true;
}
static bool Create(ChatHandler* h,std::string name,Optional<uint32> gold) {return CreateInternal(h,std::move(name),gold.value_or(250));}
static bool FinanceSelfTest(ChatHandler* h,uint32 stage) {
    if(h->GetPlayer())return false;
    auto fixture=CharacterDatabase.Query("SELECT guid FROM characters WHERE account=0 AND ((guid=9000100 AND name='Huntbuyer') OR (guid=9000101 AND name='Hunttarget'))");
    if(!fixture || fixture->GetRowCount()!=2)return false;
    auto money=[](uint32 guid){auto q=CharacterDatabase.PQuery("SELECT money FROM characters WHERE guid={}",guid);return q?q->Fetch()[0].GetUInt32():0u;};
    std::lock_guard<std::recursive_mutex> lock(mutex);
    bool ok=false;
    if(stage==1){
        ok=money(9000100)==5000000 && money(9000101)==0;
        // Force a database rollback: neither the debit nor its mail may survive.
        auto bad=CharacterDatabase.BeginTransaction();bad->Append("UPDATE characters SET money=123 WHERE guid=9000100");
        AddNotice(bad,9000101,"Hunter test rollback","This disposable notice must never be committed.");
        bad->Append("SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='hunter_test_rollback'");
        ok=ok && !Commit(bad) && money(9000100)==5000000;
        ok=ok && CreateInternal(h,"Hunttarget",250,true) && money(9000100)==2500000;
        ok=ok && !CreateInternal(h,"Hunttarget",250,true) && money(9000100)==2500000;
        if(ok){Finish(9000101,2);ok=contracts.at(9000101).paymentState==1;}
    } else if(stage==2){
        auto it=contracts.find(9000101);
        ok=it!=contracts.end() && it->second.paymentState==1 && it->second.outcome==2 && it->second.stake==2500000 && money(9000100)==2500000 && money(9000101)==0;
        if(ok){auto& c=it->second;ok=CreditReward(c) && money(9000101)==2500000 && !CreditReward(c) && money(9000101)==2500000;}
        if(ok){ok=CreateInternal(h,"Hunttarget",250,true) && money(9000100)==0;if(ok){Finish(9000101,3);Finish(9000101,3);ok=contracts.at(9000101).paymentState==4 && !CreateInternal(h,"Hunttarget",250,true);}}
    }
    TC_LOG_INFO("scripts","Hunter finance selftest stage={} {}",stage,ok?"PASS":"FAIL");return ok;
}
static bool List(ChatHandler* h) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    for(auto const& pair:contracts){auto const& c=pair.second;h->PSendSysMessage("Cible GUID %u ; niveau %u ; %s ; issue %u ; prime %u or ; paiement %u (0=reserve,1=butin,2=recupere,3=mercenaire,4=rembourse).",c.target,c.level,Actor(c)?"present":"reserve",c.outcome,c.stake/10000,c.paymentState);auto t=Actor(c)?ReadTravel(Actor(c)):c.travel;h->PSendSysMessage("Traque : phase %u (0=repos,1=vers fly,2=vol,3=attente,4=bloque) ; derniere piste %u -> %u ; trajet aerien %u ; prochain point %u.",t.phase,t.clueSource,t.destination,t.path,t.nextNode);}
    if(contracts.empty())h->SendSysMessage("Aucun contrat.");return true;
}
static bool Cancel(ChatHandler* h,uint32 target) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if(!contracts.count(target))return false;Finish(target,3);return true;
}
};
void Register();
}







#include "HunterBoard.inc"
namespace HunterContracts { void Register(){new ActorScript();new Commands();new World();new BoardScript();} }
