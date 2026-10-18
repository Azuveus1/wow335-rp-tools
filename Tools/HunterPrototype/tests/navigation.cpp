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
 float a[3]={-4438.2f,15.7436f,1631.2f},b[3]={-4316.f,61.9445f,1677.8f};
 float ext[3]={3,5,3},start[3],end[3];dtPolyRef first=0,last=0;
 assert(dtStatusSucceed(query->findNearestPoly(a,ext,&filter,&first,start)) && first);
 assert(dtStatusSucceed(query->findNearestPoly(b,ext,&filter,&last,end)) && last);
 for(int direction=0;direction<2;++direction) {
  dtPolyRef path[4096];int count=0;auto from=direction?last:first,to=direction?first:last;auto src=direction?end:start,dst=direction?start:end;
  assert(dtStatusSucceed(query->findPath(from,to,src,dst,&filter,path,&count,4096)) && count>0);
  if(path[count-1]!=to){std::cerr<<"INCOMPLETE navmesh corridor direction "<<direction<<" polygons="<<count<<"\n";return 1;}
  float points[4096*3];int pointsCount=0;assert(dtStatusSucceed(query->findStraightPath(src,dst,path,count,points,nullptr,nullptr,&pointsCount,4096)));
  double length=0;for(int i=1;i<pointsCount;++i){double dx=points[i*3]-points[(i-1)*3],dy=points[i*3+1]-points[(i-1)*3+1],dz=points[i*3+2]-points[(i-1)*3+2];length+=std::sqrt(dx*dx+dy*dy+dz*dz);}
  std::cout<<"PASS: "<<(direction?"flightmaster -> inn":"inn -> flightmaster")<<" navmesh corridor="<<count<<" corners="<<pointsCount<<" length="<<length<<"m, tiles="<<loaded<<"\n";
  std::ofstream out(direction?"build/hunter-route-return.txt":"build/hunter-route-outbound.txt");for(int i=0;i<pointsCount;++i)out<<points[i*3+2]<<' '<<points[i*3]<<' '<<points[i*3+1]<<'\n';
 }
 dtFreeNavMeshQuery(query);dtFreeNavMesh(mesh);
}
