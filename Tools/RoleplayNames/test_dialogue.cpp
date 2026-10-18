#include "DialogueNames.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <string>
struct WOWGUID {uint64_t m_guid=0;};
constexpr int TYPEMASK_UNIT=8,TYPEMASK_PLAYER=16,GENDER_NOT_APPLICABLE=0;
struct NameCache {char m_name[48]{};int m_sex=0;};
struct ObjectData {int m_type=TYPEMASK_PLAYER;};
struct UnitData {uint8_t m_unitFieldBytes0[4]{};};
struct PlayerData {uint8_t m_playerBytes3[4]{},m_playerFieldBytes[4]{};};
struct CGUnit_C {
 ObjectData object;ObjectData* m_obj=&object;UnitData data;UnitData* m_unit=&data;
 const char* full="Martin Ventdacier";
 const char* GetUnitName(void*,int){return full;}
 bool IsActivePlayer()const{return false;}
 static const char* GetDisplayClassName(const CGUnit_C*,const NameCache*){return "Mage";}
 static const char* GetDisplayRaceName(const CGUnit_C*,const NameCache*){return "Human";}
};
struct CGPlayer_C:CGUnit_C {PlayerData data;PlayerData* m_player=&data;int GetPVPFactionIndex()const{return 0;}};
static CGPlayer_C player;
static NameCache cached;
static int lookup=0;
void* ClntObjMgrObjectPtr(WOWGUID,int,const char*,int){return lookup==1?&player:nullptr;}
struct Cache {NameCache* GetRecord(uint64_t,WOWGUID,void*,void*,bool){return lookup==2?&cached:nullptr;}} g_nameDBCache;
struct AchievementDB {const void* GetRecord(int){return nullptr;}} g_achievementDB;
struct CGTooltip {template<class... T>static const char* GetAchievementHyperlink(T...){return "achievement";}};
int g_clientGameTime=0;
size_t SStrLen(const char* s){return std::strlen(s);}
const char* SStrChr(const char* s,char c){return std::strchr(s,c);}
void SStrCopy(char* out,const char* in,size_t n){if(n){auto len=std::min(n-1,std::strlen(in));std::memcpy(out,in,len);out[len]=0;}}
void SStrPack(char* out,const char* in,size_t n){auto len=std::strlen(out);if(len<n)SStrCopy(out+len,in,n-len);}
void SStrLower(char* s){while(*s){if(*s>='A' && *s<='Z')*s+=32;++s;}}
template<class... T>void SStrPrintf(char* out,size_t n,const char* fmt,T... args){std::snprintf(out,n,fmt,args...);}
int SStrToInt(const char* s){return std::atoi(s);}
int GetWorldState(int){return 0;}
const char* FrameScript_GetText(const char*,int,int){return "";}
void ParseCountDownTimerText(char*,uint32_t,int){}
void GenderConditional(char*,uint32_t,int,const CGUnit_C*,const NameCache*,const char**){}
bool QuestParserParseToken(char*,uint32_t,const NameCache*,const char*,int32_t,WOWGUID,const CGUnit_C*,const char**);
#include "QuestParserUnderTest.inc"
std::string parse(const char* text,const char* overrideName=nullptr){char out[512];assert(QuestParserParseText(text,out,sizeof(out),WOWGUID{},overrideName,0));return out;}
int main() {
 for(int context:{0,1,2}) {
  lookup=context;SStrCopy(cached.m_name,"Martin Ventdacier",sizeof(cached.m_name));
  const char* explicitName=context?nullptr:"Martin Ventdacier";
  assert(parse("Bonjour $P !",explicitName)=="Bonjour Martin !");
  assert(parse("Bonjour monsieur $F.",explicitName)=="Bonjour monsieur Ventdacier.");
  assert(parse("$N / $n / $p / $f",explicitName)=="Martin Ventdacier / Martin Ventdacier / Martin / Ventdacier");
  assert(parse("$P$F$N",explicitName)=="MartinVentdacierMartin Ventdacier");
  assert(parse("$C $R$B$P",explicitName)=="Mage Human\nMartin");
 }
 lookup=1;assert(parse("$P $F","Élise Étoile")=="Élise Étoile");
 assert(parse("$P|$F|$N","Testeur")=="Testeur||Testeur");
 player.m_obj->m_type=TYPEMASK_UNIT;player.full="Garde Royal";
 assert(parse("$P|$F|$N")=="Garde Royal||Garde Royal");
 lookup=0;char out[64];assert(!QuestParserParseText("$P/$F/$N",out,sizeof(out),WOWGUID{},nullptr,0));assert(std::string(out)=="$P/$F/$N");
 assert(!QuestParserParseText("$X",out,sizeof(out),WOWGUID{},nullptr,0));assert(std::string(out)=="$X");
 char small[12];std::memset(small,0x55,sizeof(small));QuestParserParseText("$P$F",small,8,WOWGUID{},"Martin Ventdacier",0);assert(small[7]==0 && small[8]==0x55);
 std::puts("PASS: actual quest parser; unit/cache/explicit names, first/family/full, lowercase, punctuation, adjacent markers, accents, legacy, NPCs, missing data, existing markers and bounded output");
}
