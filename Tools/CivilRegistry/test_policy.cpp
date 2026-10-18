#include "RegistryPolicy.h"
#include <cassert>
#include <iostream>
using namespace CivilRegistry;
int main(){
 assert(First("Martin Vendacier")=="Martin"&&Family("Martin Vendacier")=="Vendacier");
 assert(Family("Martin").empty()&&First("Martin")=="Martin");
 assert(Common("Martin Vendacier",0,"Elise Clairbois",1)=="Vendacier");
 assert(Common("Elise Clairbois",1,"Martin Vendacier",0)=="Vendacier");
 assert(Common("Martin Vendacier",0,"Jean Clairbois",0)=="Vendacier");
 assert(Common("Elise Clairbois",1,"Anne Vendacier",1)=="Clairbois");
 assert(Due(1,0)&&!Due(100,101)&&!Due(100+Month-1,100)&&Due(100+Month,100));
 Design d{};assert(!Unpack(DesignCount,d)&&!Decode(0,0,d)&&!Decode(0x9000,0x8000,d));
 for(uint32_t n=0;n<DesignCount;++n){assert(Unpack(n,d)&&Valid(d)&&Pack(d)==n);Design decoded{};assert(Decode(Low(n),High(n),decoded)&&decoded==d);}
 d={170,0,0,0,0};assert(!Valid(d));d={0,-1,0,0,0};assert(!Valid(d));
 std::cout<<"PASS: complete tabard palette roundtrip ("<<DesignCount<<" designs), invalid markers, legacy/full names, all couple gender rules, exact rolling-month boundary.\n";
}
