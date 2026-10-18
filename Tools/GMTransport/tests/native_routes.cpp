#include "../shared/NativeRoute.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cassert>
int main(int argc,char** argv) {
 if(argc!=2) return 2;
 std::ifstream input(argv[1]);std::string line;unsigned tested=0;bool ok=true;
 while(std::getline(input,line)) {
  std::istringstream in(line);uint32_t entry;size_t count;in>>entry>>count;std::vector<GMRoute::Node> nodes(count);
  for(auto& n:nodes) in>>n.p.x>>n.p.y>>n.p.z>>n.wait>>n.map>>n.flags>>n.arrival>>n.departure>>n.uid;
  if(!GMRoute::ValidNative(nodes)){std::cout<<"Invalid original "<<entry<<"\n";ok=false;continue;}
  ++tested;
  for(size_t i=1;i<nodes.size();++i) if(nodes[i-1].map==nodes[i].map && nodes[i-1].flags!=1) {
   auto changed=nodes;auto n=nodes[i];n.p=(nodes[i-1].p+n.p)*0.5;n.wait=0;n.flags=0;n.arrival=n.departure=0;n.uid=2000000000;
   changed.insert(changed.begin()+i,n);assert(GMRoute::ValidNative(changed));changed.erase(changed.begin()+i);assert(GMRoute::ValidNative(changed));break;
  }
  auto broken=nodes;broken[0].flags=1;assert(!GMRoute::ValidNative(broken));
  broken=nodes;broken[1].uid=broken[0].uid;assert(!GMRoute::ValidNative(broken));
  broken=nodes;broken[0].p.x=std::numeric_limits<double>::infinity();assert(!GMRoute::ValidNative(broken));
 }
 std::cout<<"Native original / insert / delete / invalid transition / duplicate UID: "<<tested<<" paths\n";
 return ok?0:1;
}
