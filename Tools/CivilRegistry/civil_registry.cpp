#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "Creature.h"
#include "Item.h"
#include "Bag.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "CharacterCache.h"
#include "DatabaseEnv.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "RBAC.h"
#include "Log.h"
#include "GameTime.h"
#include "RegistryPolicy.h"
#include "BirthNames.h"
#include "MapManager.h"
#include "Map.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Group.h"
#include "GroupMgr.h"
#include <mutex>
#include <unordered_map>
#include <sstream>
#include <limits>
#include <algorithm>

namespace Registry {
using namespace CivilRegistry;
std::recursive_mutex mutex;
ObjectGuid Guid(uint32 id){return ObjectGuid::Create<HighGuid::Player>(id);}
uint32 Id(Player* p){return uint32(p->GetGUID().GetCounter());}
uint64 Now(){return uint64(GameTime::GetGameTime());}
bool Commit(CharacterDatabaseTransaction tx){return CharacterDatabase.AsyncCommitTransaction(tx).m_future.get();}
std::string Esc(std::string value){CharacterDatabase.EscapeString(value);return value;}
void Tell(Player* p,std::string const& text){ChatHandler(p->GetSession()).SendSysMessage(text.c_str());}
void Send(Player* p,std::string const& text){WorldPacket data;ChatHandler::BuildChatPacket(data,CHAT_MSG_WHISPER,LANG_ADDON,p,p,"RPREG\t"+text);p->SendDirectMessage(&data);}
bool Ready(Player* p){return p&&p->IsInWorld()&&p->IsAlive()&&!p->IsInCombat()&&!p->IsInFlight();}
struct Identity {uint32 id=0;std::string name;uint8 gender=0;};
Identity Read(uint32 id){auto q=CharacterDatabase.PQuery("SELECT name,gender FROM characters WHERE guid={} AND deleteDate IS NULL",id);if(!q)return {};return {id,q->Fetch()[0].GetString(),q->Fetch()[1].GetUInt8()};}
uint32 FamilyId(uint32 id){auto q=CharacterDatabase.PQuery("SELECT family_id FROM rp_family_member WHERE player={}",id);return q?q->Fetch()[0].GetUInt32():0;}
uint64 Last(uint32 id){auto q=CharacterDatabase.PQuery("SELECT last_changed FROM rp_registry_name WHERE player={}",id);return q?q->Fetch()[0].GetUInt64():0;}
bool NameAllowed(Player* p,std::string const& name){return ObjectMgr::CheckPlayerName(name,p->GetSession()->GetSessionDbcLocale(),true)==CHAR_NAME_SUCCESS&&!sObjectMgr->IsReservedName(name);}
bool Available(std::string const& name,uint32 id){if(BirthNames::Taken(name,id))return false;auto q=CharacterDatabase.PQuery("SELECT guid FROM characters WHERE name='{}' AND guid<>{}",Esc(name),id);return !q;}
void AppendName(CharacterDatabaseTransaction tx,Identity const& who,std::string const& name,char const* reason){
 BirthNames::AppendRename(tx,who.id,who.name,name);
 tx->PAppend("UPDATE characters SET name='{}' WHERE guid={}",Esc(name),who.id);
 tx->PAppend("DELETE FROM character_declinedname WHERE guid={}",who.id);
 tx->PAppend("REPLACE INTO rp_registry_name(player,last_changed) VALUES ({},{})",who.id,Now());
 tx->PAppend("INSERT INTO rp_registry_history(player,old_name,new_name,reason,changed_at) VALUES ({},'{}','{}','{}',{})",who.id,Esc(who.name),Esc(name),reason,Now());
}
void PublishName(uint32 id,std::string const& name){
 if(auto p=ObjectAccessor::FindConnectedPlayer(Guid(id))){p->SetName(name);Tell(p,"Votre nom officiel est maintenant : "+name+".");}
 sCharacterCache->UpdateCharacterData(Guid(id),name);
 if(auto cache=sCharacterCache->GetCharacterCacheByGuid(Guid(id)))if(auto guild=sGuildMgr->GetGuildById(cache->GuildId))guild->UpdateMemberName(Guid(id),name);
 if(auto p=ObjectAccessor::FindConnectedPlayer(Guid(id))) {
  if(auto group=p->GetGroup())group->UpdateMemberName(Guid(id),name);
  if(auto group=p->GetOriginalGroup())group->UpdateMemberName(Guid(id),name);
 }else if(auto q=CharacterDatabase.PQuery("SELECT guid FROM group_member WHERE memberGuid={}",id))if(auto group=sGroupMgr->GetGroupByDbStoreId(q->Fetch()[0].GetUInt32()))group->UpdateMemberName(Guid(id),name);
}
struct Session {uint32 nonce=0;uint64 until=0;ObjectGuid npc;char kind=0;uint32 owner=0;std::string ownerName,partnerName;};
std::unordered_map<uint32,Session> sessions;
Session Open(Player* p,char kind,ObjectGuid npc={},uint32 owner=0){Session s{urand(1,0x7fffffff),Now()+120,npc,kind,owner};sessions[Id(p)]=s;return s;}
bool At(Player* p,Session const& s,uint32 entry){auto npc=p->GetNPCIfCanInteractWith(s.npc,UNIT_NPC_FLAG_GOSSIP);return Ready(p)&&npc&&npc->GetEntry()==entry;}
std::string Nonce(Session const& s){return std::to_string(s.nonce);}
struct Contract {uint32 owner=0,partner=0;std::string ownerName,partnerName,surname;};
Contract ReadContract(uint32 owner){auto q=CharacterDatabase.PQuery("SELECT partner,owner_name,partner_name,surname FROM rp_registry_contract WHERE owner={}",owner);if(!q)return {};auto f=q->Fetch();return {owner,f[0].GetUInt32(),f[1].GetString(),f[2].GetString(),f[3].GetString()};}
void RemovePaper(Player* p){if(!p->HasItemCount(Paper,1,true))return;p->DestroyItemCount(Paper,1,true);auto tx=CharacterDatabase.BeginTransaction();p->SaveInventoryAndGoldToDB(tx);Commit(tx);}
bool Give(Player* p,uint32 entry){
 if(p->HasItemCount(entry,1,true))return true;
 ItemPosCountVec dest;auto result=p->CanStoreNewItem(NULL_BAG,NULL_SLOT,dest,entry,1);
 if(result!=EQUIP_ERR_OK){p->SendEquipError(result,nullptr,nullptr,entry);return false;}
 auto item=p->StoreNewItem(dest,entry,true);if(!item)return false;
 p->SendNewItem(item,1,true,false);auto tx=CharacterDatabase.BeginTransaction();p->SaveInventoryAndGoldToDB(tx);
 if(!Commit(tx)){Tell(p,"Sauvegarde de l'objet en attente. Reparlez au notaire avant de poursuivre.");return false;}return true;
}
#include "WeddingSlot.inc"
void VisitRegistrar(Player* p,ObjectGuid npc){auto s=Open(p,'R',npc);bool free=Family(p->GetName()).empty()&&!FamilyId(Id(p))&&!Last(Id(p));Send(p,"REG|"+Nonce(s)+"|"+p->GetName()+"|"+(free?"0":"10"));}
void View(Player* p,ObjectGuid npc={}){
 auto c=ReadContract(Id(p));if(!c.owner){Tell(p,"Vous ne possedez aucun contrat de mariage en cours.");return;}
 if(!Give(p,Paper))return;
 auto s=Open(p,'C',npc);Send(p,"DOC|"+Nonce(s)+"|"+c.ownerName+"|"+c.partnerName+"|"+c.surname);
}
void Rename(Player* p,Session const& s,std::string surname){
 if(!At(p,s,Registrar)){Tell(p,"Retournez aupres de l'officier d'etat civil.");return;}
 if(!normalizePlayerName(surname)||surname.find(' ')!=std::string::npos){Tell(p,"Saisissez uniquement le nouveau nom de famille, sans espace.");return;}
 auto fid=FamilyId(Id(p));std::vector<Identity> people;
 if(fid){auto q=CharacterDatabase.PQuery("SELECT player FROM rp_family_member WHERE family_id={}",fid);if(q)do{people.push_back(Read(q->Fetch()[0].GetUInt32()));}while(q->NextRow());if(people.size()!=2){Tell(p,"Le dossier familial est incomplet. Aucun changement effectue.");return;}}
 else people.push_back(Read(Id(p)));
 bool free=!fid&&Family(p->GetName()).empty()&&!Last(Id(p));
 std::vector<std::string> names;
 for(auto const& who:people){
  if(!who.id){Tell(p,"Un des personnages du dossier n'existe plus.");return;}
  auto name=First(who.name)+" "+surname;
  if(!NameAllowed(p,name)||!Available(name,who.id)){Tell(p,"Ce nom complet est invalide ou deja utilise : "+name);return;}
  if(name==who.name){Tell(p,"Ce nom est deja votre nom de famille.");return;}
  if(!free&&!Due(Now(),Last(who.id))){Tell(p,"Le changement est limite a une fois tous les 30 jours, pour chacun des epoux.");return;}
  if(std::find(names.begin(),names.end(),name)!=names.end()){Tell(p,"Les deux epoux ne peuvent pas porter le meme prenom et nom complet.");return;}names.push_back(name);
 }
 uint32 cost=free?0:RenameCost;if(p->GetMoney()<cost){Tell(p,"Il faut 10 pieces d'or pour changer le nom de famille.");return;}
 auto tx=CharacterDatabase.BeginTransaction();uint32 old=p->GetMoney();p->SetMoney(old-cost);p->SaveGoldToDB(tx);
 for(size_t i=0;i<people.size();++i)AppendName(tx,people[i],names[i],free?"initial":"rename");
 if(fid)tx->PAppend("UPDATE rp_family SET surname='{}' WHERE id={}",Esc(surname),fid);
 if(!Commit(tx)){p->SetMoney(old);Tell(p,"Enregistrement refuse, aucun paiement effectue.");return;}
 for(size_t i=0;i<people.size();++i)PublishName(people[i].id,names[i]);
 Tell(p,free?"Premiere inscription gratuite effectuee.":"Nom de famille mis a jour. Paiement : 10 pieces d'or pour le dossier.");
}
void Buy(Player* p,ObjectGuid npc){
 if(!Ready(p))return;
 if(FamilyId(Id(p))||Family(p->GetName()).empty()){Tell(p,"Il faut etre celibataire et posseder un nom de famille.");return;}
 if(CharacterDatabase.PQuery("SELECT player FROM rp_registry_participant WHERE player={}",Id(p))){Tell(p,"Vous participez deja a un contrat. Consultez votre document ou annulez-le.");return;}
 ItemPosCountVec dest;if(p->CanStoreNewItem(NULL_BAG,NULL_SLOT,dest,Paper,1)!=EQUIP_ERR_OK){Tell(p,"Liberez une place dans vos sacs.");return;}
 if(p->GetMoney()<ContractCost){Tell(p,"Le contrat coute 25 pieces d'or.");return;}
 auto tx=CharacterDatabase.BeginTransaction();uint32 old=p->GetMoney();p->SetMoney(old-ContractCost);p->SaveGoldToDB(tx);
 tx->PAppend("INSERT INTO rp_registry_contract(owner,owner_name) VALUES ({},'{}')",Id(p),Esc(p->GetName()));
 tx->PAppend("INSERT INTO rp_registry_participant(player,owner) VALUES ({},{})",Id(p),Id(p));
 if(!Commit(tx)){p->SetMoney(old);Tell(p,"Achat refuse, aucun paiement effectue.");return;}
 Tell(p,"Contrat achete pour 25 pieces d'or. Selectionnez votre partenaire puis demandez sa signature.");View(p,npc);
}
bool WearingRing(Player* p,uint32 spouse){
 auto worn=Worn(Id(p));if(worn.item&&worn.spouse==spouse)return true;
 for(uint8 slot=EQUIPMENT_SLOT_FINGER1;slot<=EQUIPMENT_SLOT_FINGER2;++slot)if(auto item=p->GetItemByPos(INVENTORY_SLOT_BAG_0,slot)){
  if(item->GetEntry()!=Ring)continue;
  auto q=CharacterDatabase.PQuery("SELECT wearer,spouse FROM rp_registry_ring WHERE item_guid={}",item->GetGUID().GetCounter());
  if(q&&q->Fetch()[0].GetUInt32()==Id(p)&&q->Fetch()[1].GetUInt32()==spouse)return true;
 }return false;
}
void BuyRings(Player* p,Session const& session,std::string name){
 if(!At(p,session,Jeweler)||!normalizePlayerName(name))return;
 auto target=sCharacterCache->GetCharacterGuidByName(name);auto other=Read(uint32(target.GetCounter()));auto c=ReadContract(Id(p));
 if(!c.owner||c.partner||!other.id||other.id==Id(p)){Tell(p,"Saisissez le nom complet d'un autre personnage existant et conservez votre contrat vierge.");return;}
 if(FamilyId(Id(p))||FamilyId(other.id)||Family(other.name).empty()){Tell(p,"Les deux partenaires doivent etre celibataires et avoir un nom de famille.");return;}
 if(p->GetMoney()<RenameCost){Tell(p,"La paire d'alliances gravees coute 10 pieces d'or.");return;}
 if(CharacterDatabase.PQuery("SELECT item_guid FROM rp_registry_ring WHERE contract_owner={}",Id(p))){Tell(p,"Des alliances ont deja ete gravees pour ce contrat. Conservez-les ou annulez le contrat avant d'en acheter une nouvelle paire.");return;}
 ItemPosCountVec dest;auto err=p->CanStoreNewItem(NULL_BAG,NULL_SLOT,dest,Ring,2);if(err!=EQUIP_ERR_OK){Tell(p,"Il faut deux places libres dans vos sacs.");return;}
 std::vector<Item*> rings;
 // One StoreNewItem call can return only one pointer, so create each distinct
 // non-stackable ring separately and journal its actual instance GUID.
 for(int i=0;i<2;++i){ItemPosCountVec one;if(p->CanStoreNewItem(NULL_BAG,NULL_SLOT,one,Ring,1)!=EQUIP_ERR_OK)break;auto item=p->StoreNewItem(one,Ring,true);if(!item)break;rings.push_back(item);}
 if(rings.size()!=2){for(auto item:rings){uint32 count=1;p->DestroyItemCount(item,count,true);}Tell(p,"Impossible de preparer les alliances. Aucun paiement effectue.");return;}
 auto tx=CharacterDatabase.BeginTransaction();auto old=p->GetMoney();p->SetMoney(old-RenameCost);
 for(int i=0;i<2;++i){uint32 wearer=i?Id(p):other.id,spouse=i?other.id:Id(p);std::string engraving=i?other.name:p->GetName();auto item=rings[i];item->SetText("Gravure : "+engraving);
  tx->PAppend("INSERT INTO rp_registry_ring(item_guid,wearer,spouse,engraving,contract_owner) VALUES ({},{},{},'{}',{})",item->GetGUID().GetCounter(),wearer,spouse,Esc(engraving),Id(p));}
 p->SaveInventoryAndGoldToDB(tx);
 if(!Commit(tx)){p->SetMoney(old);for(auto item:rings){uint32 count=1;p->DestroyItemCount(item,count,true);}auto cleanup=CharacterDatabase.BeginTransaction();p->SaveInventoryAndGoldToDB(cleanup);Commit(cleanup);Tell(p,"Achat refuse, aucun paiement effectue.");return;}
 for(auto item:rings)p->SendNewItem(item,1,true,false);
 Tell(p,"Deux alliances gravees achetees pour 10 po. Donnez a votre partenaire celle portant votre nom et equipez celle portant le sien.");
}
void Offer(Player* p){
 auto c=ReadContract(Id(p));auto other=p->GetSelectedPlayer();
 if(!c.owner||c.partner||!p->HasItemCount(Paper,1)){Tell(p,"Il faut un contrat vierge dans vos sacs.");return;}
 if(!Ready(other)||other==p||!p->IsWithinDistInMap(other,10)||!p->IsWithinLOSInMap(other)||!p->InSamePhase(other)){Tell(p,"Selectionnez votre partenaire, a moins de 10 metres.");return;}
 if(FamilyId(Id(p))||FamilyId(Id(other))||Family(p->GetName()).empty()||Family(other->GetName()).empty()){Tell(p,"Les deux partenaires doivent etre celibataires et posseder un nom de famille.");return;}
 if(CharacterDatabase.PQuery("SELECT player FROM rp_registry_participant WHERE player={}",Id(other))){Tell(p,"Ce personnage participe deja a un contrat.");return;}
 std::string common=Common(p->GetName(),uint8(p->GetNativeGender()),other->GetName(),uint8(other->GetNativeGender()));
 if(!WearingRing(p,Id(other))||!WearingRing(other,Id(p))){Tell(p,"Vous devez tous deux porter au doigt une alliance gravee au nom de l'autre.");return;}
 auto s=Open(other,'S',{},Id(p));sessions[Id(other)].ownerName=p->GetName();sessions[Id(other)].partnerName=other->GetName();Send(other,"SIGN|"+Nonce(s)+"|"+p->GetName()+"|"+common);
 Tell(p,"Demande de signature envoyee. Votre partenaire doit accepter le nom commun : "+common+".");
}
void Sign(Player* p,Session const& s){
 auto owner=ObjectAccessor::FindConnectedPlayer(Guid(s.owner));auto c=ReadContract(s.owner);
 if(!Ready(owner)||!Ready(p)||!owner->IsWithinDistInMap(p,10)||!owner->IsWithinLOSInMap(p)||!owner->InSamePhase(p)||!c.owner||c.partner||!owner->HasItemCount(Paper,1)){Tell(p,"La demande de signature a expire. Rapprochez-vous et recommencez.");return;}
 if(FamilyId(Id(p))||FamilyId(s.owner)||Family(p->GetName()).empty()||Family(owner->GetName()).empty())return;
 if(owner->GetName()!=s.ownerName||p->GetName()!=s.partnerName||!WearingRing(p,s.owner)||!WearingRing(owner,Id(p))){Tell(p,"Les noms ou les alliances ont change. Demandez une nouvelle signature.");return;}
 auto common=Common(owner->GetName(),uint8(owner->GetNativeGender()),p->GetName(),uint8(p->GetNativeGender()));
 auto tx=CharacterDatabase.BeginTransaction();
 tx->PAppend("INSERT INTO rp_registry_participant(player,owner) VALUES ({},{})",Id(p),s.owner);
 tx->PAppend("UPDATE rp_registry_contract SET partner={},owner_name='{}',partner_name='{}',surname='{}' WHERE owner={} AND partner=0",Id(p),Esc(owner->GetName()),Esc(p->GetName()),Esc(common),s.owner);
 if(!Commit(tx)){Tell(p,"Signature refusee : un autre contrat a peut-etre deja ete signe.");return;}
 Tell(p,"Contrat signe. Le nom commun sera : "+common+".");Tell(owner,"Votre partenaire a signe. Rapportez le contrat au notaire.");View(owner);
}
void Finalize(Player* p,Session const& s){
 if(!At(p,s,Notary)){Tell(p,"Rapportez le document au notaire matrimonial.");return;}
 auto c=ReadContract(Id(p));if(!c.owner||!c.partner||!p->HasItemCount(Paper,1)){Tell(p,"Il faut un contrat signe dans vos sacs.");return;}
 auto a=Read(c.owner),b=Read(c.partner);
 if(!a.id||!b.id||a.name!=c.ownerName||b.name!=c.partnerName||Common(a.name,a.gender,b.name,b.gender)!=c.surname){Tell(p,"L'identite des partenaires a change. Annulez ce document et faites signer un nouveau contrat.");return;}
 if(FamilyId(a.id)||FamilyId(b.id))return;
 std::string na=First(a.name)+" "+c.surname,nb=First(b.name)+" "+c.surname;
 if(na==nb||!NameAllowed(p,na)||!NameAllowed(p,nb)||!Available(na,a.id)||!Available(nb,b.id)){Tell(p,"Le mariage produirait un nom complet invalide ou deja utilise. Aucun changement effectue.");return;}
 auto q=CharacterDatabase.Query("SELECT COALESCE(MAX(id),0)+1 FROM rp_family");uint32 fid=q->Fetch()[0].GetUInt32();
 auto tx=CharacterDatabase.BeginTransaction();
 tx->PAppend("INSERT INTO rp_family(id,surname,design,created_at) VALUES ({},'{}',0,{})",fid,Esc(c.surname),Now());
 tx->PAppend("INSERT INTO rp_family_member(player,family_id) VALUES ({},{}),({},{})",a.id,fid,b.id,fid);
 AppendName(tx,a,na,"marriage");AppendName(tx,b,nb,"marriage");
 tx->PAppend("DELETE FROM rp_registry_participant WHERE owner={}",c.owner);tx->PAppend("DELETE FROM rp_registry_contract WHERE owner={}",c.owner);
 tx->PAppend("UPDATE rp_registry_ring SET contract_owner=0 WHERE contract_owner={}",c.owner);
 if(!Commit(tx)){Tell(p,"Le mariage n'a pas pu etre enregistre. Votre contrat est conserve.");return;}
 PublishName(a.id,na);PublishName(b.id,nb);RemovePaper(p);
 Tell(p,"Mariage enregistre ! Les deux epoux peuvent retirer leur tabard familial aupres du notaire.");
 if(auto partner=ObjectAccessor::FindConnectedPlayer(Guid(b.id)))Tell(partner,"Votre mariage est officiellement enregistre.");
}
void Cancel(Player* p,Session const& s){
 if(!At(p,s,Notary)){Tell(p,"Adressez-vous au notaire pour annuler le contrat.");return;}
 auto tx=CharacterDatabase.BeginTransaction();tx->PAppend("DELETE FROM rp_registry_participant WHERE owner={}",Id(p));tx->PAppend("DELETE FROM rp_registry_contract WHERE owner={}",Id(p));
 tx->PAppend("UPDATE rp_registry_ring SET contract_owner=0 WHERE contract_owner={}",Id(p));
 if(Commit(tx)){RemovePaper(p);Tell(p,"Contrat annule. Les frais d'achat ne sont pas rembourses.");}
}
uint32 DesignFor(uint32 id){auto q=CharacterDatabase.PQuery("SELECT f.design FROM rp_family f JOIN rp_family_member m ON f.id=m.family_id WHERE m.player={}",id);return q?q->Fetch()[0].GetUInt32():0;}
void StyleItem(Player* p,Item* item,uint32 design){if(!item||item->GetEntry()!=Tabard)return;
 item->SetEnchantment(PERM_ENCHANTMENT_SLOT,Low(design),0,0);item->SetEnchantment(TEMP_ENCHANTMENT_SLOT,High(design),0,0);item->SetState(ITEM_CHANGED,p);
 if(item->IsEquipped())p->SetVisibleItemSlot(item->GetSlot(),item);
}
void Refresh(Player* p){
 uint32 design=DesignFor(Id(p));
 for(uint8 slot=EQUIPMENT_SLOT_START;slot<BANK_SLOT_BAG_END;++slot){auto item=p->GetItemByPos(INVENTORY_SLOT_BAG_0,slot);StyleItem(p,item,design);if(auto bag=item?item->ToBag():nullptr)for(uint32 i=0;i<bag->GetBagSize();++i)StyleItem(p,bag->GetItemByPos(uint8(i)),design);}
 auto tx=CharacterDatabase.BeginTransaction();p->SaveInventoryAndGoldToDB(tx);Commit(tx);
}
void Editor(Player* p,ObjectGuid npc){if(!FamilyId(Id(p))){Tell(p,"Vous ne faites partie d'aucune famille enregistree.");return;}auto s=Open(p,'E',npc);Send(p,"EMBLEM|"+Nonce(s)+"|"+std::to_string(DesignFor(Id(p))));}
void SaveDesign(Player* p,Session const& s,std::string const& text){
 if(!At(p,s,Notary))return;Design d{};std::istringstream in(text);std::string bit;
 for(unsigned i=0;i<5;++i){if(!std::getline(in,bit,',')||bit.empty()||bit.size()>3||bit.find_first_not_of("0123456789")!=std::string::npos)return;d[i]=std::stoi(bit);}if(std::getline(in,bit,',')||!Valid(d))return;
 uint32 fid=FamilyId(Id(p));if(!fid)return;auto tx=CharacterDatabase.BeginTransaction();tx->PAppend("UPDATE rp_family SET design={} WHERE id={}",Pack(d),fid);
 if(!Commit(tx)){Tell(p,"Le blason n'a pas ete sauvegarde.");return;}
 auto q=CharacterDatabase.PQuery("SELECT player FROM rp_family_member WHERE family_id={}",fid);if(q)do{if(auto other=ObjectAccessor::FindConnectedPlayer(Guid(q->Fetch()[0].GetUInt32())))Refresh(other);}while(q->NextRow());Tell(p,"Blason familial sauvegarde pour les deux epoux.");
}
class NPC : public CreatureScript {public:NPC():CreatureScript("npc_civil_registry"){}
 struct AI : ScriptedAI {using ScriptedAI::ScriptedAI;
 bool OnGossipHello(Player* p) override {std::lock_guard<std::recursive_mutex> lock(mutex);if(!Ready(p))return true;ClearGossipMenuFor(p);p->PlayerTalkClass->GetGossipMenu().SetMenuId(urand(1,0x7fffffff));
  if(me->GetEntry()==Jeweler){p->PrepareQuestMenu(me->GetGUID());AddGossipItemFor(p,GOSSIP_ICON_VENDOR,"Voir la boutique habituelle",GOSSIP_SENDER_MAIN,8);AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Acheter deux alliances gravees pour mon contrat (10 po)",GOSSIP_SENDER_MAIN,9,"Votre partenaire selectionne recevra l'alliance portant votre nom. Acheter la paire pour 10 po ?",0,false);}
  else if(me->GetEntry()==Registrar)AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Inscrire ou changer mon nom de famille (gratuit / 10 po)",GOSSIP_SENDER_MAIN,1);
  else {AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Acheter un contrat de mariage (25 po)",GOSSIP_SENDER_MAIN,2,"Acheter ce document pour 25 pieces d'or ?",0,false);AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Consulter / signer mon document",GOSSIP_SENDER_MAIN,3);AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Enregistrer le mariage (contrat signe)",GOSSIP_SENDER_MAIN,4);AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Retirer mon tabard familial (gratuit)",GOSSIP_SENDER_MAIN,5);AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Personnaliser le blason familial",GOSSIP_SENDER_MAIN,6);AddGossipItemFor(p,GOSSIP_ICON_CHAT,"Annuler mon contrat non enregistre",GOSSIP_SENDER_MAIN,7,"Annuler ce contrat, sans remboursement des 25 po ?",0,false);}
  SendGossipMenuFor(p,1,me->GetGUID());return true;}
 bool OnGossipSelect(Player* p,uint32 menu,uint32 index) override {std::lock_guard<std::recursive_mutex> lock(mutex);if(!Ready(p)||p->GetNPCIfCanInteractWith(me->GetGUID(),UNIT_NPC_FLAG_GOSSIP)!=me||menu!=p->PlayerTalkClass->GetGossipMenu().GetMenuId())return true;auto action=GetGossipActionFor(p,index);ClearGossipMenuFor(p);CloseGossipMenuFor(p);
  auto s=Open(p,'C',me->GetGUID());
  if(me->GetEntry()==Jeweler){if(action==8)p->GetSession()->SendListInventory(me->GetGUID());else if(action==9){s=Open(p,'J',me->GetGUID());Send(p,"RINGS|"+Nonce(s));}return true;}
  if(me->GetEntry()==Registrar){if(action==1)VisitRegistrar(p,me->GetGUID());return true;}
  switch(action){case 2:Buy(p,me->GetGUID());break;case 3:View(p,me->GetGUID());break;case 4:Finalize(p,s);break;case 5:if(FamilyId(Id(p))&&Give(p,Tabard))Refresh(p);else Tell(p,"Un mariage enregistre est necessaire.");break;case 6:Editor(p,me->GetGUID());break;case 7:Cancel(p,s);break;}return true;
 }
 };CreatureAI* GetAI(Creature* c)const override{return new AI(c);}
};
class Document : public ItemScript {public:Document():ItemScript("item_civil_registry"){}bool OnUse(Player* p,Item* item,SpellCastTargets const&)override{std::lock_guard<std::recursive_mutex> lock(mutex);p->SendEquipError(EQUIP_ERR_NONE,item);if(Ready(p)&&item->GetOwnerGUID()==p->GetGUID())View(p);return true;}};
class CosmeticItem : public ItemScript {public:explicit CosmeticItem(char const* name):ItemScript(name){}};
class Hooks : public PlayerScript {public:Hooks():PlayerScript("civil_registry_hooks"){}
 void OnLogin(Player* p,bool)override{std::lock_guard<std::recursive_mutex> lock(mutex);if(FamilyId(Id(p)))Refresh(p);}
 void OnLogout(Player* p)override{std::lock_guard<std::recursive_mutex> lock(mutex);sessions.erase(Id(p));ringTokens.erase(Id(p));}
 void OnChat(Player* p,uint32,uint32 lang,std::string& message,Player* receiver)override{
  if(lang!=LANG_ADDON||receiver!=p||message.rfind("RPREG\t",0)!=0)return;
  std::string text=message.substr(6);message.clear();if(text.size()>240||!p->IsInWorld())return;
  std::lock_guard<std::recursive_mutex> lock(mutex);
  if(RingMessage(p,text))return;
  if(!Ready(p))return;
  if(text.rfind("TIP|",0)==0){
   // Read-only engraving lookup, confined to the requesting character's bags.
   unsigned bag=0,slot=0;char extra=0;std::istringstream input(text.substr(4));char comma=0;
   if(!(input>>bag>>comma>>slot)||comma!=','||(input>>extra)||bag>255||slot>255)return;
   auto item=p->GetItemByPos(uint8(bag),uint8(slot));if(!item||item->GetEntry()!=Ring)return;
   auto q=CharacterDatabase.PQuery("SELECT engraving FROM rp_registry_ring WHERE item_guid={}",item->GetGUID().GetCounter());if(q)Send(p,"TIP|"+std::to_string(bag)+","+std::to_string(slot)+"|"+q->Fetch()[0].GetString());return;
  }
  auto a=text.find('|'),b=a==std::string::npos?a:text.find('|',a+1);if(a==std::string::npos)return;
  auto op=text.substr(0,a),token=text.substr(a+1,b==std::string::npos?b:b-a-1),arg=b==std::string::npos?"":text.substr(b+1);
  auto it=sessions.find(Id(p));if(it==sessions.end()||token!=Nonce(it->second)||Now()>it->second.until){Tell(p,"Le formulaire a expire. Reparlez au fonctionnaire ou rouvrez le document.");return;}
  auto s=it->second;sessions.erase(it);
  if(op=="RENAME"&&s.kind=='R')Rename(p,s,arg);
  else if(op=="RINGS"&&s.kind=='J')BuyRings(p,s,arg);
  else if(op=="OFFER"&&s.kind=='C')Offer(p);
  else if(op=="SIGN"&&s.kind=='S')Sign(p,s);
  else if(op=="EMBLEM"&&s.kind=='E')SaveDesign(p,s,arg);
 }
};
class Tests : public CommandScript {public:Tests():CommandScript("civil_registry_tests"){}
 static bool Run(ChatHandler* h){
  if(h->GetPlayer())return false;
  std::lock_guard<std::recursive_mutex> lock(mutex);bool ok=true;unsigned count=0;
  for(auto const& pair:sObjectMgr->GetAllCreatureData()){
   auto const& data=pair.second;if(data.id!=Registrar&&data.id!=Notary)continue;++count;
   auto map=sMapMgr->CreateBaseMap(data.mapId);map->LoadGrid(data.spawnPoint.GetPositionX(),data.spawnPoint.GetPositionY());
   auto npc=map->GetCreatureBySpawnId(pair.first);float floor=map->GetHeight(data.spawnPoint.GetPositionX(),data.spawnPoint.GetPositionY(),data.spawnPoint.GetPositionZ()+2,true,10);
   bool good=npc&&dynamic_cast<NPC::AI*>(npc->AI())&&std::isfinite(floor)&&std::abs(floor-data.spawnPoint.GetPositionZ())<2;
   ok=ok&&good;TC_LOG_INFO("scripts","Civil registry NPC {} entry={} spawn={} floor={} z={}",good?"PASS":"FAIL",data.id,pair.first,floor,data.spawnPoint.GetPositionZ());
  }
  ok=ok&&count==2;
  auto ring=Item::CreateItem(Ring,1),paper=Item::CreateItem(Paper,1),tabard=Item::CreateItem(Tabard,1);
  bool items=ring&&paper&&tabard&&ring->GetTemplate()->InventoryType==INVTYPE_FINGER&&!ring->IsSoulBound()&&paper->GetTemplate()->Spells[0].SpellId==24740&&tabard->GetTemplate()->InventoryType==INVTYPE_TABARD;
  ok=ok&&items;delete ring;delete paper;delete tabard;
  TC_LOG_INFO("scripts","Civil registry inventory objects {}: usable paper, tradable ring, wearable family tabard",items?"PASS":"FAIL");
  auto a=Read(9000300),b=Read(9000301);
  if(a.name!="Rpfirst Rpstone"||b.name!="Rpother Rpriver"){TC_LOG_ERROR("scripts","Civil registry selftest FAIL: fixtures missing");return false;}
  auto query=CharacterDatabase.PQuery("SELECT account FROM characters WHERE guid IN ({},{})",a.id,b.id);
  if(!query)return false;do{if(query->Fetch()[0].GetUInt32()!=0)return false;}while(query->NextRow());
  auto born=CharacterDatabase.BeginTransaction();BirthNames::AppendIdentity(born,a.id,a.name);BirthNames::AppendIdentity(born,b.id,b.name);ok=Commit(born)&&ok;
  ok=ok&&!Available(b.name,a.id)&&Available("Rpfirst Rpcliff",a.id)&&Due(Now(),0);
  // Exercise the same persistent name/history helper used by the live forms.
  auto tx=CharacterDatabase.BeginTransaction();AppendName(tx,a,"Rpfirst Rpcliff","test");AppendName(tx,b,"Rpother Rpcliff","test");
  tx->PAppend("INSERT INTO rp_family(id,surname,design,created_at) VALUES (9000300,'Rpcliff',{}, {})",DesignCount-1,Now());
  tx->PAppend("INSERT INTO rp_family_member(player,family_id) VALUES ({},9000300),({},9000300)",a.id,b.id);
  ok=Commit(tx)&&ok;
  ok=ok&&Read(a.id).name=="Rpfirst Rpcliff"&&Read(b.id).name=="Rpother Rpcliff"&&FamilyId(a.id)==FamilyId(b.id)&&FamilyId(a.id)==9000300&&DesignFor(a.id)==DesignCount-1&&!Due(Now(),Last(a.id));
  PublishName(a.id,"Rpfirst Rpcliff");PublishName(b.id,"Rpother Rpcliff");
  ok=ok&&sCharacterCache->GetCharacterGuidByName("Rpfirst Rpcliff")==Guid(a.id)&&sCharacterCache->GetCharacterGuidByName("Rpother Rpcliff")==Guid(b.id);
  ok=ok&&BirthNames::Get(a.id)==a.name&&BirthNames::Get(b.id)==b.name&&BirthNames::Taken(a.name)&&BirthNames::Taken("RPFIRST RPSTONE")&&!BirthNames::Taken(a.name,a.id)&&!Available(a.name,b.id);
  auto stolen=CharacterDatabase.BeginTransaction();AppendName(stolen,Read(b.id),a.name,"test");
  TC_LOG_INFO("scripts","Birth identity collision test: expected reservation duplicate follows");
  ok=!Commit(stolen)&&ok;ok=ok&&Read(b.id).name=="Rpother Rpcliff";
  auto newCharacter=CharacterDatabase.BeginTransaction();BirthNames::AppendIdentity(newCharacter,9000302,a.name);
  ok=!Commit(newCharacter)&&ok;ok=ok&&BirthNames::Get(9000302).empty();
  auto restoreBirth=CharacterDatabase.BeginTransaction();AppendName(restoreBirth,Read(a.id),a.name,"testreturn");ok=Commit(restoreBirth)&&ok;
  ok=ok&&Read(a.id).name==a.name&&BirthNames::Get(a.id)==a.name&&!BirthNames::Taken("Rpfirst Rpcliff");
  auto marryAgain=CharacterDatabase.BeginTransaction();AppendName(marryAgain,Read(a.id),"Rpfirst Rpcliff","test");ok=Commit(marryAgain)&&ok;
  TC_LOG_INFO("scripts","Birth identity protection {}: immutable original identity, case-insensitive creation/rename rejection, own-name restoration and release of marital alias",ok?"PASS":"FAIL");
  auto souvenir=Item::CreateItem(Ring,1);souvenir->SetOwnerGUID(Guid(a.id));souvenir->SetText("Gravure : Rpother Rpriver");auto ringId=uint32(souvenir->GetGUID().GetCounter());
  auto create=CharacterDatabase.BeginTransaction();souvenir->SaveToDB(create);
  create->PAppend("INSERT INTO character_inventory(guid,bag,slot,item) VALUES ({},0,23,{})",a.id,ringId);
  create->PAppend("INSERT INTO rp_registry_ring(item_guid,wearer,spouse,engraving,contract_owner) VALUES ({},{},{},'Rpother Rpriver',0)",ringId,a.id,b.id);ok=Commit(create)&&ok;
  auto wear=CharacterDatabase.BeginTransaction();AppendWearRing(wear,a.id,souvenir);ok=Commit(wear)&&ok;
  auto worn=Worn(a.id);ok=ok&&worn.item==ringId&&worn.spouse==b.id&&worn.engraving=="Rpother Rpriver";
  auto locations=CharacterDatabase.PQuery("SELECT COUNT(*) FROM character_inventory WHERE item={}",ringId);ok=ok&&locations->Fetch()[0].GetUInt32()==0;
  delete souvenir;auto restored=LoadWornRing(a.id,ringId);ok=ok&&restored&&restored->GetGUID().GetCounter()==ringId&&restored->GetText()=="Gravure : Rpother Rpriver";
  auto putBack=CharacterDatabase.BeginTransaction();AppendRemoveRing(putBack,a.id,ringId);putBack->PAppend("INSERT INTO character_inventory(guid,bag,slot,item) VALUES ({},0,23,{})",a.id,ringId);ok=Commit(putBack)&&ok;delete restored;
  locations=CharacterDatabase.PQuery("SELECT COUNT(*) FROM character_inventory WHERE item={}",ringId);ok=ok&&!Worn(a.id).item&&locations->Fetch()[0].GetUInt32()==1;
  TC_LOG_INFO("scripts","Wedding slot persistence {}: actual Item save/load, same GUID and engraving, bag-to-slot-to-bag transaction, exclusive inventory location",ok?"PASS":"FAIL");
  auto before=CharacterDatabase.PQuery("SELECT money FROM characters WHERE guid={}",a.id)->Fetch()[0].GetUInt32();
  auto fail=CharacterDatabase.BeginTransaction();AppendName(fail,Read(a.id),"Rpfirst Rpfailed","test");
  fail->PAppend("UPDATE characters SET money=1 WHERE guid={}",a.id);
  // Deliberate duplicate fixture membership: the entire transaction must roll
  // back, including money, the character name and its new history row.
  fail->PAppend("INSERT INTO rp_family_member(player,family_id) VALUES ({},9000300)",a.id);
  TC_LOG_INFO("scripts","Civil registry rollback test: expected duplicate fixture membership follows");
  ok=!Commit(fail)&&ok;
  auto after=CharacterDatabase.PQuery("SELECT money FROM characters WHERE guid={}",a.id)->Fetch()[0].GetUInt32();
  ok=ok&&before==after&&Read(a.id).name=="Rpfirst Rpcliff";
  auto history=CharacterDatabase.PQuery("SELECT COUNT(*) FROM rp_registry_history WHERE player={}",a.id);
  ok=ok&&history->Fetch()[0].GetUInt32()==3;
  TC_LOG_INFO("scripts","Civil registry selftest {}: NPC placement, actual unique-name checks, shared surname transaction, history, cooldown, family membership, style persistence and live name cache",ok?"PASS":"FAIL");
  h->SendSysMessage(ok?"Civil registry selftest PASS":"Civil registry selftest FAIL");return ok;
 }
 Trinity::ChatCommands::ChatCommandTable GetCommands()const override {using namespace Trinity::ChatCommands;return {{"civiltest",Run,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes}};}
};
}
void AddSC_civil_registry(){new Registry::NPC();new Registry::Document();new Registry::CosmeticItem("item_civil_registry_tabard");new Registry::CosmeticItem("item_civil_registry_ring");new Registry::Hooks();new Registry::Tests();TC_LOG_INFO("scripts","Civil registry loaded: names, marriage contracts and independent family tabards");}

