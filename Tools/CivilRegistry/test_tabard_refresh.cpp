#include "RegistryPolicy.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
using INVENTORY_SLOTS=int;
constexpr int INVSLOT_TABARD=18;
struct VisibleItem_C {int m_itemId=0;unsigned GetDisplayID(){return 20621;}};
void SErrDisplayAppFatalMessage(char const*,char const*,int){assert(false);}
struct Data{VisibleItem_C m_playerVisibleItems[19];};
struct CGPlayer_C{Data storage;Data* m_player=&storage;int draws=0,weapons=0;
void LoadGuildTextures(unsigned display){assert(display==20621);++draws;}
void UpdateObjComponentVisualsSlot(VisibleItem_C const*,int){++weapons;}
void UpdateInventoryComponent(INVENTORY_SLOTS,VisibleItem_C const*);
};
#include "TabardRefreshUnderTest.inc"
int main(){CGPlayer_C player;VisibleItem_C old{int(CivilRegistry::Tabard)};
player.storage.m_playerVisibleItems[18]=old;player.UpdateInventoryComponent(18,&old);assert(player.draws==1&&player.weapons==0);
old.m_itemId=100;player.storage.m_playerVisibleItems[15]=old;player.UpdateInventoryComponent(15,&old);assert(player.draws==1&&player.weapons==1);
old.m_itemId=5976;player.storage.m_playerVisibleItems[18]=old;player.UpdateInventoryComponent(18,&old);assert(player.draws==1&&player.weapons==2);
std::cout<<"PASS: actual same-item update path redraws family tabard without re-equipping; weapon and guild-tabard paths preserved.\n";}
