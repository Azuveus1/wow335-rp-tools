#include "GreetingPolicy.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdlib>
int main(int argc,char** argv) {
 using namespace RPGreetings;
 Conditions good{true,true,true,true,true,true,true,true,true};
 for(auto emote:{48u,53u,55u,78u,101u})assert(Allowed(emote,good));
 if(argc==2)assert(Allowed(uint32_t(std::strtoul(argv[1],nullptr,10)),good));
 assert(!Allowed(10,good));
 for(bool Conditions::* member:{&Conditions::playerReady,&Conditions::npcReady,&Conditions::selected,&Conditions::nearby,&Conditions::visible,&Conditions::lineOfSight,&Conditions::nonHostile,&Conditions::socialNpc,&Conditions::uncontrolled}){
  auto denied=good;denied.*member=false;assert(!Allowed(101,denied));
 }
 assert(Reply("Alice Plume d'acier",1,false)=="Bonjour Alice !");
 assert(Reply("Alice Plume d'acier",1,true)=="Bonjour madame Plume d'acier.");
 assert(Reply("Martin Ventdacier",0,true)=="Bonjour monsieur Ventdacier.");
 assert(Reply("Élise Étoile",1,false)=="Bonjour Élise !");
 assert(Reply("Testeur",1,true)=="Bonjour Testeur !");
 assert(Reply("Alice Plume",2,true)=="Bonjour Alice !");
 assert(Reply("Alice Plume",1,true,false)=="Hello, Ms. Plume.");
 Cooldowns cooldown;Key k{1,2,1,0};
 assert(cooldown.Try(k,100));assert(!cooldown.Try(k,101));assert(!cooldown.Try(k,600099));
 assert(cooldown.Try(k,600100)); // Repeated greetings never postpone the deadline.
 assert(cooldown.Try({2,2,1,0},600100));assert(cooldown.Try({1,3,1,0},600100));
 assert(cooldown.Try({1,2,1,1},600100));assert(cooldown.Try({1,2,0,0},600100));
 Cooldowns bounded(2);assert(bounded.Try(k,0));assert(bounded.Try({2,2,1,0},0));
 assert(!bounded.Try({3,2,1,0},1));assert(!bounded.Try(k,2));assert(bounded.Try({3,2,1,0},600000));
 Cooldowns concurrent;std::atomic<int> accepted{0};std::vector<std::thread> threads;
 for(int i=0;i<16;++i)threads.emplace_back([&]{if(concurrent.Try(k,100))++accepted;});
 for(auto& thread:threads)thread.join();assert(accepted==1);
 std::cout<<"PASS: targeted greeting policy, all eligibility gates, first/family/gender/localization, exact ten-minute pair cooldown, bounded cleanup and concurrent repeat protection\n";
}
