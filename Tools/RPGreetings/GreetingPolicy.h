#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
namespace RPGreetings {
constexpr uint64_t CooldownMs=10*60*1000;
constexpr float Range=10.0f;
enum TextEmote : uint32_t {Greet=48,Hail=53,Hello=55,Salute=78,Wave=101};
inline bool GreetingEmote(uint32_t emote){return emote==Greet || emote==Hail || emote==Hello || emote==Salute || emote==Wave;}
struct Conditions {
 bool playerReady=false,npcReady=false,selected=false,nearby=false;
 bool visible=false,lineOfSight=false,nonHostile=false,socialNpc=false,uncontrolled=false;
};
inline bool Allowed(uint32_t emote,Conditions const& c) {
 return GreetingEmote(emote) && c.playerReady && c.npcReady && c.selected && c.nearby
  && c.visible && c.lineOfSight && c.nonHostile && c.socialNpc && c.uncontrolled;
}
struct Key {
 uint64_t player=0,npc=0;
 uint32_t map=0,instance=0;
 bool operator==(Key const& k)const{return player==k.player && npc==k.npc && map==k.map && instance==k.instance;}
};
struct Hash {
 size_t operator()(Key const& k)const {
  auto h=std::hash<uint64_t>{}(k.player);
  auto mix=[&](uint64_t value){h^=std::hash<uint64_t>{}(value)+size_t(0x9e3779b9)+(h<<6)+(h>>2);};
  mix(k.npc);mix(k.map);mix(k.instance);return h;
 }
};
class Cooldowns {
 std::mutex mutex;
 std::unordered_map<Key,uint64_t,Hash> deadlines;
 uint64_t nextSweep=0;
 size_t capacity;
public:
 explicit Cooldowns(size_t maxEntries=16384):capacity(maxEntries){}
 bool Try(Key key,uint64_t now) {
  std::lock_guard<std::mutex> lock(mutex);
  if(now>=nextSweep) {
   for(auto i=deadlines.begin();i!=deadlines.end();)if(now>=i->second)i=deadlines.erase(i);else ++i;
   nextSweep=now+60000;
  }
  auto found=deadlines.find(key);
  if(found!=deadlines.end() && now<found->second)return false;
  if(found==deadlines.end() && deadlines.size()>=capacity)return false;
  deadlines[key]=now+CooldownMs;return true;
 }
};
inline std::string Reply(std::string_view name,uint8_t gender,bool formal,bool french=true) {
 auto space=name.find(' ');
 auto first=name.substr(0,space);
 auto family=space==std::string_view::npos?std::string_view{}:name.substr(space+1);
 if(first.empty())return french?"Bonjour !":"Hello!";
 if(formal && !family.empty() && gender<=1) {
  std::string title=french?(gender==1?"madame ":"monsieur "):(gender==1?"Ms. ":"Mr. ");
  return std::string(french?"Bonjour ":"Hello, ")+title+std::string(family)+".";
 }
 return std::string(french?"Bonjour ":"Hello, ")+std::string(first)+(french?" !":"!");
}
}
