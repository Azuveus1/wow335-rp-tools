#include "../shared/BoardInput.h"
#include <cassert>
#include <iostream>
int main(){
 HunterBoardInput::Request r;
 assert(HunterBoardInput::Parse("Testeur;250",r) && r.name=="Testeur" && r.gold==250);
 assert(HunterBoardInput::Parse(" Testeur ; 1 ",r) && r.name=="Testeur" && r.gold==1);
 assert(HunterBoardInput::Parse("Testeur;100000",r) && r.gold==100000);
 for(auto bad:{"","Testeur",";250","Testeur;0","Testeur;-1","Testeur;250.5","Testeur;1e3","Testeur;100001","Testeur;9999999999999999999999","Testeur;250;replay","Testeur;250junk","P|tibou;250","P\ntibou;250"})assert(!HunterBoardInput::Parse(bad,r));
 assert(!HunterBoardInput::Parse(std::string(49,'a')+";1",r));
 for(unsigned n=1;n<=100000;n+=137)assert(HunterBoardInput::Parse("Testeur;"+std::to_string(n),r) && r.gold==n);
 std::cout<<"PASS: board input handles names and integer gold, rejects malformed/overflow/replayed payloads.\n";
}
