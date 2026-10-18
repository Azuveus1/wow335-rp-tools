#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "WorldSession.h"
#include "Random.h"
#include "Log.h"
#include "RPGreetingPolicy.h"
#include <chrono>
namespace {
static_assert(uint32(RPGreetings::Greet)==TEXT_EMOTE_GREET && uint32(RPGreetings::Hail)==TEXT_EMOTE_HAIL
 && uint32(RPGreetings::Hello)==TEXT_EMOTE_HELLO && uint32(RPGreetings::Salute)==TEXT_EMOTE_SALUTE
 && uint32(RPGreetings::Wave)==TEXT_EMOTE_WAVE,"Greeting emote IDs must match the core protocol");
class RPGreetingScript : public PlayerScript {
 RPGreetings::Cooldowns cooldowns;
public:
 RPGreetingScript():PlayerScript("rp_targeted_greetings") {
  TC_LOG_INFO("scripts","RP greetings ready: selected friendly NPC, range=10m, cooldown=600s per player/NPC");
 }
 void OnTextEmote(Player* player,uint32 emote,uint32 /*variant*/,ObjectGuid guid)override {
  if(!player || guid.IsEmpty() || !RPGreetings::GreetingEmote(emote) || player->GetTarget()!=guid)return;
  Creature* npc=ObjectAccessor::GetCreature(*player,guid);if(!npc)return;
  constexpr uint32 socialFlags=UNIT_NPC_FLAG_GOSSIP|UNIT_NPC_FLAG_QUESTGIVER|UNIT_NPC_FLAG_VENDOR
   |UNIT_NPC_FLAG_TRAINER|UNIT_NPC_FLAG_FLIGHTMASTER|UNIT_NPC_FLAG_INNKEEPER|UNIT_NPC_FLAG_BANKER
   |UNIT_NPC_FLAG_AUCTIONEER|UNIT_NPC_FLAG_STABLEMASTER;
  RPGreetings::Conditions c;
  c.playerReady=player->IsInWorld() && player->IsAlive() && !player->IsInCombat() && !player->IsInFlight()
   && !player->HasUnitState(UNIT_STATE_DIED|UNIT_STATE_STUNNED|UNIT_STATE_CONFUSED|UNIT_STATE_FLEEING);
  c.npcReady=npc->IsInWorld() && npc->IsAlive() && !npc->IsInCombat()
   && !npc->HasUnitState(UNIT_STATE_DIED|UNIT_STATE_STUNNED|UNIT_STATE_CONFUSED|UNIT_STATE_FLEEING);
  c.selected=true;
  c.nearby=npc->IsWithinDistInMap(player,RPGreetings::Range);
  c.visible=player->CanSeeOrDetect(npc) && npc->CanSeeOrDetect(player);
  c.lineOfSight=npc->IsWithinLOSInMap(player);
  c.nonHostile=!npc->IsHostileTo(player) && !player->IsHostileTo(npc);
  c.socialNpc=npc->GetCreatureType()==CREATURE_TYPE_HUMANOID || (npc->GetNpcFlags()&socialFlags);
  c.uncontrolled=!npc->IsControlledByPlayer() && !npc->IsPet() && !npc->IsTotem();
  if(!RPGreetings::Allowed(emote,c))return;
  // Persistent spawn identities retain their cooldown across grid unload/reload.
  RPGreetings::Key key{player->GetGUID().GetRawValue(),npc->GetSpawnId()?npc->GetSpawnId():npc->GetGUID().GetRawValue(),npc->GetMapId(),npc->GetInstanceId()};
  auto now=uint64(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
  if(!cooldowns.Try(key,now))return;
  bool french=player->GetSession()->GetSessionDbcLocale()==LOCALE_frFR;
  std::string text=RPGreetings::Reply(player->GetName(),uint8(player->GetNativeGender()),urand(0,1)!=0,french);
  if(!npc->isMoving())npc->SetFacingToObject(player);
  npc->HandleEmoteCommand(emote==TEXT_EMOTE_SALUTE?EMOTE_ONESHOT_SALUTE:EMOTE_ONESHOT_WAVE);
  npc->Say(text,LANG_UNIVERSAL,player);
 }
};
}
void AddSC_rp_greetings(){new RPGreetingScript();}
