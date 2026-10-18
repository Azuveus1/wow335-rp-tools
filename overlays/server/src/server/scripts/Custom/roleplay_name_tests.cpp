#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "RBAC.h"
#include "CharacterCache.h"
#include <cstring>
using namespace Trinity::ChatCommands;
class RoleplayNameTests : public CommandScript {
public:
 RoleplayNameTests():CommandScript("roleplay_name_tests"){}
 static bool Run(ChatHandler* handler) {
  if(handler->GetPlayer())return false;
  auto valid=[](std::string name){return normalizePlayerName(name) && ObjectMgr::CheckPlayerName(name,LOCALE_frFR,true)==CHAR_NAME_SUCCESS;};
  std::string name="Martin vENTDACIER";
  bool ok=normalizePlayerName(name) && name=="Martin Ventdacier"
    && valid("Martin Ventdacier") && valid("Martin Clairbois") && valid("Elise Ventdacier")
    && valid("Élise Ventdacier") && !valid("Martin") && !valid("Martin ")
    && !valid(" Martin Ventdacier") && !valid("Martin  Ventdacier")
    && !valid("Martin Ventdacier Autre") && !valid("Martin Ventdacier1")
    && !valid("Martin Treizelongnom") && !valid("Martin\tVentdacier")
    && ObjectMgr::CheckPlayerName("Martin",LOCALE_frFR,false)==CHAR_NAME_SUCCESS;
  auto fixture=sCharacterCache->GetCharacterGuidByName("Rpfirst Rpstone");
  if(!fixture.IsEmpty()) {
   auto second=sCharacterCache->GetCharacterGuidByName("Rpfirst Rpriver");
   auto third=sCharacterCache->GetCharacterGuidByName("Rpother Rpstone");
   ok=ok && fixture.GetCounter()==9000200 && second.GetCounter()==9000201 && third.GetCounter()==9000202;
   auto target=[&](char* text,uint32 id){ObjectGuid guid;return handler->extractPlayerTarget(text,nullptr,&guid)&&guid.GetCounter()==id;};
   char full[]="Rpfirst Rpstone";ok=target(full,9000200)&&ok;
   char shared[]="Rpfirst Rpriver";ok=target(shared,9000201)&&ok;
   char quoted[]="\"Rpfirst Rpstone\" 25 reason";ok=target(quoted,9000200)&&ok;
   auto tail=strtok(nullptr," ");ok=ok&&tail&&std::strcmp(tail,"25")==0;
   char parameters[]="Rpfirst Rpstone 10 reason";ok=target(parameters,9000200)&&ok;
   tail=strtok(nullptr," ");ok=ok&&tail&&std::strcmp(tail,"10")==0;
   char link[]="|cffffffff|Hplayer:Rpfirst Rpstone|h[Rpfirst Rpstone]|h|r 15";ok=target(link,9000200)&&ok;
   tail=strtok(nullptr," ");ok=ok&&tail&&std::strcmp(tail,"15")==0;
   if(!sCharacterCache->GetCharacterGuidByName("Rpfirst").IsEmpty()) {
    char legacy[]="Rpfirst 20 reason";ok=target(legacy,9000203)&&ok;
    tail=strtok(nullptr," ");ok=ok&&tail&&std::strcmp(tail,"20")==0;
   }
   TC_LOG_INFO("scripts","Roleplay command names selftest {}: complete names preferred over same first-name legacy identity, quoted names, hyperlinks, suffix cursor",ok?"PASS":"FAIL");
  }
  TC_LOG_INFO("scripts","Roleplay names selftest {}: real core validation, normalization, shared first/family names, accents, malformed names, legacy login and full-name cache",ok?"PASS":"FAIL");
  handler->SendSysMessage(ok?"Roleplay names selftest PASS":"Roleplay names selftest FAIL");return ok;
 }
 ChatCommandTable GetCommands()const override {return {{"rpnametest",Run,rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,Console::Yes}};}
};
void AddSC_roleplay_names_tests(){new RoleplayNameTests();}
