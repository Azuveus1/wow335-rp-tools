#pragma once
#include "Creature.h"
#include "Player.h"
#include "CreatureAI.h"
#include "HunterPolicy.h"

// A private combat relationship. Never changes a player's faction.
namespace HunterCombat
{
constexpr uint32 Entry = 9100101;
constexpr int32 TargetSlot = 9100101;
constexpr int32 ClaimSlot = 9100102;
inline bool IsHunter(Unit const* u) { return u && u->GetEntry() == Entry && u->ToCreature(); }
inline ObjectGuid Target(Unit const* u)
{
    auto c=IsHunter(u)?u->ToCreature():nullptr;
    return c && c->AI()?c->AI()->GetGUID(TargetSlot):ObjectGuid::Empty;
}
inline void SetTarget(Creature* c,ObjectGuid target) { if(IsHunter(c) && c->AI())c->AI()->SetGUID(target,TargetSlot); }
inline uint32 ViewerFaction(Unit const* hunter,Unit const* viewer)
{
    auto player=viewer?viewer->GetCharmerOrOwnerPlayerOrPlayerItself():nullptr;
    return player && player->GetGUID()==Target(hunter)?14u:35u;
}
inline bool IsTarget(Unit const* hunter, Unit const* other)
{
    Player const* player = other ? other->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
    return player && player->GetGUID() == Target(hunter);
}
// -1: unrelated units; 0: forbidden; 1: private enemies.
inline int Relation(Unit const* a, Unit const* b)
{
    if (!IsHunter(a) && !IsHunter(b)) return -1;
    auto pa=a?a->GetCharmerOrOwnerPlayerOrPlayerItself():nullptr;
    auto pb=b?b->GetCharmerOrOwnerPlayerOrPlayerItself():nullptr;
    return RelationIds(IsHunter(a),IsHunter(b),Target(a).GetRawValue(),Target(b).GetRawValue(),pa?pa->GetGUID().GetRawValue():0,pb?pb->GetGUID().GetRawValue():0,a==b);
}
}


