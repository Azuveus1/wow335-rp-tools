#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourAlloc.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
struct Header {uint32_t magic,version,mmapVersion,size;char liquids,pad[3];};
int main() {
 const std::filesystem::path folder="C:/TC/data/mmaps";
 dtNavMeshParams params{};std::ifstream map(folder/"001.mmap",std::ios::binary);map.read(reinterpret_cast<char*>(&params),sizeof(params));assert(map);
 auto mesh=dtAllocNavMesh();assert(dtStatusSucceed(mesh->init(&params)));
 int loaded=0;
 for(int x=26;x<=30;++x)for(int y=38;y<=42;++y) {
  char name[30];std::snprintf(name,sizeof(name),"001%02d%02d.mmtile",x,y);std::ifstream tile(folder/name,std::ios::binary);if(!tile)continue;
  Header h{};tile.read(reinterpret_cast<char*>(&h),sizeof(h));assert(h.magic==0x4d4d4150 && h.version==DT_NAVMESH_VERSION);
  auto data=static_cast<unsigned char*>(dtAlloc(h.size,DT_ALLOC_PERM));tile.read(reinterpret_cast<char*>(data),h.size);assert(tile);
  assert(dtStatusSucceed(mesh->addTile(data,h.size,DT_TILE_FREE_DATA,0,nullptr)));++loaded;
 }
 auto query=dtAllocNavMeshQuery();assert(dtStatusSucceed(query->init(mesh,8192)));
 dtQueryFilter filter;filter.setIncludeFlags(1);filter.setExcludeFlags(0);

 float candidates[][3]={{-4446,17,1655},{-4443,17,1654},{-4448,17,1650},{-4441,17,1658},{-4450,20,1660},{-4438,18,1650}};
 for(auto const& point:candidates){float ext[3]={.5f,10,.5f},nearest[3];dtPolyRef poly=0;
 if(dtStatusSucceed(query->findNearestPoly(point,ext,&filter,&poly,nearest)) && poly)
 std::cout<<"Ground x="<<nearest[2]<<" y="<<nearest[0]<<" z="<<nearest[1]<<"\n";
 else std::cout<<"No ground near "<<point[2]<<" "<<point[0]<<"\n";
 }
 dtFreeNavMeshQuery(query);dtFreeNavMesh(mesh);
}
