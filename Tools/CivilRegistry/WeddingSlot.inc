// Independent cosmetic equipment. The actual item instance is kept intact,
// outside character_inventory while worn, with an exclusive owner/slot row.
// Transfers between the bag and this slot are one CharacterDB transaction.
struct WornRing {uint32 item=0,spouse=0;std::string engraving;};
struct RingToken {uint32 value=0;uint64 until=0;};
std::unordered_map<uint32,RingToken> ringTokens;
WornRing Worn(uint32 player){
 auto q=CharacterDatabase.PQuery("SELECT w.item_guid,r.spouse,r.engraving FROM rp_registry_worn_ring w JOIN rp_registry_ring r ON r.item_guid=w.item_guid JOIN item_instance i ON i.guid=w.item_guid WHERE w.player={} AND r.wearer={} AND i.owner_guid={} AND i.itemEntry={}",player,player,player,Ring);
 if(!q)return {};auto f=q->Fetch();return {f[0].GetUInt32(),f[1].GetUInt32(),f[2].GetString()};
}
void RingState(Player* p){auto worn=Worn(Id(p));RingToken t{urand(1,0x7fffffff),Now()+120};ringTokens[Id(p)]=t;Send(p,"RINGSELF|"+std::to_string(t.value)+"|"+std::to_string(worn.item)+"|"+worn.engraving);}
void RingView(Player* p,std::string const& id){
 if(id.empty()||id.size()>10||id.find_first_not_of("0123456789")!=std::string::npos)return;
 auto n=std::stoull(id);if(n>std::numeric_limits<uint32>::max())return;
 if(!n||n==Id(p)){RingState(p);return;}
 auto other=ObjectAccessor::FindConnectedPlayer(Guid(uint32(n)));
 if(!other||!p->IsWithinDistInMap(other,28.0f)||!p->InSamePhase(other)||!p->CanSeeOrDetect(other))return;
 auto worn=Worn(uint32(n));Send(p,"RINGOTHER|"+id+"|"+std::to_string(worn.item)+"|"+worn.engraving);
}
bool RingTokenOK(Player* p,std::string const& value){auto it=ringTokens.find(Id(p));if(it==ringTokens.end()||Now()>it->second.until||value!=std::to_string(it->second.value))return false;ringTokens.erase(it);return Ready(p);}
bool AccessibleRing(Player* p,Item* item){
 if(!item||item->GetEntry()!=Ring||item->GetOwnerGUID()!=p->GetGUID()||item->GetCount()!=1||item->IsInTrade())return false;
 return item->IsEquipped()||p->IsInventoryPos(item->GetBagSlot(),item->GetSlot());
}
void AppendWearRing(CharacterDatabaseTransaction tx,uint32 player,Item* item){
 item->DeleteFromInventoryDB(tx);item->SetState(ITEM_CHANGED);item->SaveToDB(tx);
 tx->PAppend("INSERT INTO rp_registry_worn_ring(player,item_guid) VALUES ({},{})",player,item->GetGUID().GetCounter());
}
void AppendRemoveRing(CharacterDatabaseTransaction tx,uint32 player,uint32 item){tx->PAppend("DELETE FROM rp_registry_worn_ring WHERE player={} AND item_guid={}",player,item);}
Item* LoadWornRing(uint32 player,uint32 low){
 auto q=CharacterDatabase.PQuery("SELECT creatorGuid,giftCreatorGuid,count,duration,charges,flags,enchantments,randomPropertyId,durability,playedTime,text FROM item_instance WHERE guid={} AND owner_guid={} AND itemEntry={}",low,player,Ring);
 if(!q)return nullptr;auto item=new Item();if(!item->LoadFromDB(low,Guid(player),q->Fetch(),Ring)){delete item;return nullptr;}return item;
}
void WearRing(Player* p){
 if(Worn(Id(p)).item){Tell(p,"Une alliance est deja portee dans votre emplacement dedie.");return;}
 auto q=CharacterDatabase.PQuery("SELECT r.item_guid,r.spouse FROM rp_registry_ring r WHERE r.wearer={} ORDER BY r.item_guid DESC",Id(p));
 Item* item=nullptr;uint32 family=FamilyId(Id(p));
 if(q)do{
  auto f=q->Fetch();if(family&&FamilyId(f[1].GetUInt32())!=family)continue;
  auto candidate=p->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(f[0].GetUInt32()));if(AccessibleRing(p,candidate)){item=candidate;break;}
 }while(q->NextRow());
 if(!item){Tell(p,"Aucune alliance gravee pour vous n'est disponible dans vos sacs ou sur vos doigts.");return;}
 uint8 bag=item->GetBagSlot(),slot=item->GetSlot();bool equipped=item->IsEquipped();
 p->MoveItemFromInventory(bag,slot,true);
 auto tx=CharacterDatabase.BeginTransaction();AppendWearRing(tx,Id(p),item);p->SaveInventoryAndGoldToDB(tx);
 if(!Commit(tx)){
  if(equipped){p->EquipItem(uint16((bag<<8)|slot),item,true);item->SetState(ITEM_CHANGED,p);}
  else {ItemPosCountVec original;original.emplace_back(uint16((bag<<8)|slot),1);p->MoveItemToInventory(original,item,true,true);}
  auto restore=CharacterDatabase.BeginTransaction();p->SaveInventoryAndGoldToDB(restore);Commit(restore);Tell(p,"L'alliance n'a pas pu etre deplacee. Elle reste dans votre equipement ou vos sacs.");return;
 }
 delete item;Tell(p,"Alliance portee dans son emplacement dedie, sans occuper un anneau de combat.");
}
void RemoveRing(Player* p){
 auto worn=Worn(Id(p));if(!worn.item)return;
 auto item=LoadWornRing(Id(p),worn.item);if(!item){Tell(p,"L'alliance ne peut pas etre chargee. Aucun objet n'a ete deplace.");return;}
 ItemPosCountVec dest;auto error=p->CanStoreItem(NULL_BAG,NULL_SLOT,dest,item,false);
 if(error!=EQUIP_ERR_OK){p->SendEquipError(error,nullptr,nullptr,Ring);delete item;return;}
 p->MoveItemToInventory(dest,item,true,true);
 auto tx=CharacterDatabase.BeginTransaction();AppendRemoveRing(tx,Id(p),worn.item);p->SaveInventoryAndGoldToDB(tx);
 if(!Commit(tx)){
  p->MoveItemFromInventory(item->GetBagSlot(),item->GetSlot(),true);delete item;Tell(p,"Retrait refuse : l'alliance reste dans son emplacement dedie.");return;
 }
 Tell(p,"Alliance remise dans vos sacs. Sa gravure est conservee.");
}
bool RingMessage(Player* p,std::string const& text){
 if(text.rfind("RINGVIEW|",0)==0){RingView(p,text.substr(9));return true;}
 bool equip=text.rfind("RINGEQUIP|",0)==0,remove=text.rfind("RINGREMOVE|",0)==0;
 if(!equip&&!remove)return false;
 auto token=text.substr(equip?10:11);
 if(!RingTokenOK(p,token)){Tell(p,"Reouvrez votre fiche avant de modifier l'alliance, hors combat.");RingState(p);return true;}
 if(equip)WearRing(p);else RemoveRing(p);RingState(p);return true;
}
