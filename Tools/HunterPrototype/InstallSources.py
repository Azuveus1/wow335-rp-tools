from pathlib import Path
import shutil,datetime
root=Path(r'C:/TC/TrinityCore');task=Path(__file__).parent
stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S');backup=Path(r'C:/TC')/('hunter-prototype-source-backup-'+stamp)
target=root/'src/server/scripts/Custom/custom_hunter_navigation.cpp'
if target.exists():backup.mkdir(parents=True,exist_ok=True);shutil.copy2(target,backup/target.name)
shutil.copy2(task/'server'/target.name,target)
p=root/'src/server/scripts/Custom/custom_script_loader.cpp';raw=p.read_bytes();s=raw.decode('utf-8',errors='surrogateescape')
if 'void AddSC_hunter_navigation_prototype();' not in s:
 backup.mkdir(parents=True,exist_ok=True);(backup/p.name).write_bytes(raw)
 s=s.replace('void AddCustomScripts()', 'void AddSC_hunter_navigation_prototype();\n\nvoid AddCustomScripts()',1)
 s=s.replace('    AddSC_gm_transport();','    AddSC_gm_transport();\n    AddSC_hunter_navigation_prototype();',1)
 p.write_bytes(s.encode('utf-8',errors='surrogateescape'))
print('Source backup:',backup)
# Patch only precise function anchors, idempotently, preserving other local work.
def patch(relative, transform):
 p=root/relative;raw=p.read_bytes();s=raw.decode('utf-8',errors='surrogateescape')
 new=transform(s)
 if new!=s:
  backup.mkdir(parents=True,exist_ok=True);(backup/p.name).write_bytes(raw)
  p.write_bytes(new.encode('utf-8',errors='surrogateescape'))
shutil.copy2(task/'server/HunterCombat.h',root/'src/server/game/Entities/Unit/HunterCombat.h')
def core(s):
 if '#include "HunterCombat.h"' not in s:s='#include "HunterCombat.h"\n'+s
 if 'HunterCombat::Relation' not in s:
  anchor='    ASSERT(target);'
  start=s.index('bool WorldObject::IsValidAttackTarget(');end=s.index('bool WorldObject::IsValidAssistTarget(',start)
  part=s[start:end].replace(anchor,anchor+'\n    int hunterRelation = HunterCombat::Relation(ToUnit(), target->ToUnit());\n    if (hunterRelation == 0) return false;',1)
  part=part.replace('    // CvC case -', '    if (hunterRelation == 1) return true; // Standard visibility, death and immunity checks above still apply.\n\n    // CvC case -',1)
  s=s[:start]+part+s[end:]
  start=s.index('bool WorldObject::IsValidAssistTarget(')
  part=s[start:].replace(anchor,anchor+'\n    if (HunterCombat::IsHunter(target->ToUnit()) && this != target) return false;',1)
  s=s[:start]+part
 # Player-owned traps are WorldObjects rather than Units. Resolve their owner too.
 s=s.replace('HunterCombat::Relation(ToUnit(), target->ToUnit())','HunterCombat::Relation(ToUnit() ? ToUnit() : GetAffectingPlayer(), target->ToUnit())')
 if 'hunterPrivateReaction' not in s:
  start=s.index('ReputationRank WorldObject::GetReactionTo(');pos=s.index('{',start)+1
  s=s[:pos]+'''\n    int hunterPrivateReaction = HunterCombat::Relation(ToUnit() ? ToUnit() : GetAffectingPlayer(), target->ToUnit());
    if (hunterPrivateReaction >= 0) return hunterPrivateReaction == 1 ? REP_HOSTILE : REP_FRIENDLY;
'''+s[pos:]
 return s
patch('src/server/game/Entities/Object/Object.cpp',core)
def damage(s):
 if '#include "HunterCombat.h"' not in s:s='#include "HunterCombat.h"\n'+s
 for signature,body in [
 ('uint32 Unit::DealDamage(', '    if (HunterCombat::Relation(attacker, victim) == 0) return 0;'),
 ('void Unit::Kill(', '    if (HunterCombat::Relation(attacker, victim) == 0) return;')]:
  start=s.index(signature);pos=s.index('{',start)+1
  if body not in s[start:pos+250]:s=s[:pos]+'\n'+body+'\n'+s[pos:]
 if 'HunterCombat::ViewerFaction(this, target)' not in s:
  anchor='            else if (index == UNIT_FIELD_BYTES_2 || index == UNIT_FIELD_FACTIONTEMPLATE)'
  if s.count(anchor)!=1:raise RuntimeError('Client faction update anchor changed; refusing ambiguous patch.')
  s=s.replace(anchor,'''            else if (index == UNIT_FIELD_FACTIONTEMPLATE && HunterCombat::IsHunter(this))
                fieldBuffer << HunterCombat::ViewerFaction(this, target);
'''+anchor,1)
 if 'HunterCombat::IsTarget(this, target)' not in s:
  anchor='                uint32 appendValue = m_uint32Values[UNIT_NPC_FLAGS];'
  if s.count(anchor)!=1:raise RuntimeError('NPC flag update anchor changed; refusing ambiguous patch.')
  s=s.replace(anchor,anchor+'''\n                if (HunterCombat::IsHunter(this) && HunterCombat::IsTarget(this, target))
                    appendValue &= ~UNIT_NPC_FLAG_GOSSIP;
''',1)
 return s
patch('src/server/game/Entities/Unit/Unit.cpp',damage)
shutil.copy2(task/'server/HunterContracts.inc',root/'src/server/scripts/Custom/HunterContracts.inc')
# Include the policy-only header next to the core adapter.
shutil.copy2(task/'server/HunterPolicy.h',root/'src/server/game/Entities/Unit/HunterPolicy.h')
# Bounty corpses are private and never use group money distribution.
def loot_player(s):
 if '#include "HunterCombat.h"' not in s:s='#include "HunterCombat.h"\n'+s
 if 'Hunter bounty private corpse' not in s:
  start=s.index('bool Player::isAllowedToLoot(Creature const* creature) const');pos=s.index('{',start)+1
  s=s[:pos]+'''\n    // Hunter bounty private corpse: group membership never grants access.
    if (HunterCombat::IsHunter(creature))
        return creature->isDead() && HunterCombat::IsTarget(creature, this) && creature->loot.gold > 0;
'''+s[pos:]
  start=s.index('void Player::SendLoot(');end=s.index('\nvoid Player::',start+10)
  part=s[start:end];anchor='        loot = &creature->loot;'
  if part.count(anchor)!=1:raise RuntimeError('Private loot open anchor changed.')
  part=part.replace(anchor,'''        if (HunterCombat::IsHunter(creature) && (loot_type != LOOT_CORPSE || !isAllowedToLoot(creature)))
        {
            SendLootError(guid, LOOT_ERROR_DIDNT_KILL);
            return;
        }
'''+anchor,1)
  s=s[:start]+part+s[end:]
 return s
patch('src/server/game/Entities/Player/Player.cpp',loot_player)
def loot_money(s):
 if '#include "HunterCombat.h"' not in s:s='#include "HunterCombat.h"\n'+s
 if 'HunterCombat::ClaimSlot' not in s:
  start=s.index('void WorldSession::HandleLootMoneyOpcode(');end=s.index('void WorldSession::HandleLootOpcode(',start)
  part=s[start:end];anchor='                loot = &creature->loot;'
  if part.count(anchor)!=1:raise RuntimeError('Private money loot anchor changed.')
  part=part.replace(anchor,'''                if (HunterCombat::IsHunter(creature))
                {
                    if (!player->isAllowedToLoot(creature)) { player->SendLootError(guid, LOOT_ERROR_DIDNT_KILL); return; }
                    creature->AI()->SetGUID(player->GetGUID(), HunterCombat::ClaimSlot);
                    return; // The contract transaction credits exactly one player.
                }
'''+anchor,1)
  s=s[:start]+part+s[end:]
 return s
patch('src/server/game/Handlers/LootHandler.cpp',loot_money)
shutil.copy2(task/'server/HunterBoard.inc',root/'src/server/scripts/Custom/HunterBoard.inc')
shutil.copy2(task/'shared/BoardInput.h',root/'src/server/scripts/Custom/HunterBoardInput.h')
shutil.copy2(task/'shared/HunterTravelEvents.h',root/'src/server/game/Entities/Unit/HunterTravelEvents.h')
shutil.copy2(task/'server/HunterTaxi.h',root/'src/server/scripts/Custom/HunterTaxi.h')
def player_taxi_clue(s):
 if '#include "HunterTravelEvents.h"' not in s:s='#include "HunterTravelEvents.h"\n'+s
 if 'HunterTravelEvents::Publish' not in s:
  start=s.index('bool Player::ActivateTaxiPathTo(std::vector<uint32> const& nodes,');end=s.index('bool Player::ActivateTaxiPathTo(uint32 taxi_path_id',start)
  part=s[start:end];anchor='    if (sWorld->getBoolConfig(CONFIG_INSTANT_TAXI))'
  if part.count(anchor)!=1:raise RuntimeError('Accepted taxi departure anchor changed.')
  part=part.replace(anchor,'''    // Only a paid, validated flightmaster itinerary is a clue. Scripted taxi
    // spells and rejected/failed selections never disclose a destination.
    if (npc)
        HunterTravelEvents::Publish(GetGUID().GetRawValue(), nodes.front(), nodes.back(), GetTeam());

'''+anchor,1);s=s[:start]+part+s[end:]
 return s
patch('src/server/game/Entities/Player/Player.cpp',player_taxi_clue)
