#include "../server/HunterPolicy.h"
#include <cassert>
#include <iostream>
int main() {
 using HunterCombat::RelationIds;
 // Each target also represents all pets/guardians resolved to that owner by the adapter.
 for(unsigned target=1;target<=80;++target)for(unsigned player=0;player<=80;++player) {
  int expected=player==target?1:0;
  assert(RelationIds(true,false,target,0,0,player,false)==expected);
  assert(RelationIds(false,true,0,target,player,0,false)==expected);
  assert(RelationIds(false,false,0,0,target,player,false)==-1);
  assert(RelationIds(true,true,target,player,0,0,false)==0);
 }
 assert(RelationIds(true,false,0,0,0,0,false)==0);
 assert(RelationIds(true,true,1,1,1,1,true)==0);
 std::cout<<"PASS: exclusive target/owner relationships, outsiders, other hunters and unbound actor fail closed.\n";
}
