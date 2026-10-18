#include "../shared/RouteMath.h"
#include "../shared/RouteProjection.h"
#include <cassert>
#include <limits>
#include <iostream>
int main() {
 struct Matrix {
  double a0=1,a1=0,a2=0,a3=0,b0=0,b1=1,b2=0,b3=0;
  double c0=0,c1=0,c2=0,c3=-1,d0=0,d1=0,d2=0,d3=0;
 } matrix;
 double x,y,x2,y2;
 GMRoute::Vec camera{-5700,-2900,200},target{-5700,-2900,190};
 assert(GMRoute::Project(matrix,target,camera,x,y) && std::abs(x-0.5)<1e-10 && std::abs(y-0.5)<1e-10);
 assert(GMRoute::Project(matrix,target-camera,{},x2,y2) && x==x2 && y==y2);
 assert(!GMRoute::Project(matrix,{-5700,-2900,210},camera,x,y));
 assert(GMRoute::Project(matrix,{-5600,-2900,190},camera,x,y) && x>1); // Retain off-screen link endpoints.
 GMRoute::Route r; r.map=1; r.speed=10;
 r.nodes={{{0,0,10},5},{{100,0,10},0},{{100,100,30},2},{{0,100,30},0}};
 assert(r.Build()); assert(r.period>40000);
 assert(r.At(1000).stopped && GMRoute::Length(r.At(1000).p-r.nodes[0].p)<1e-8);
 assert(!r.At(6000).stopped);
 for(size_t i=0;i<4;++i) {
   assert(GMRoute::Length(r.Curve(i,0)-r.nodes[i].p)<1e-8);
   assert(GMRoute::Length(r.Curve(i,1)-r.nodes[(i+1)%4].p)<1e-8);
 }
 assert(GMRoute::Length(r.At(r.period+1234).p-r.At(1234).p)<1e-8);
 for(uint32_t t=0;t<r.period;t+=100) { auto p=r.At(t); assert(std::isfinite(p.p.x)&&std::isfinite(p.facing)); }
 auto original=r; r.nodes[1].p=r.nodes[0].p; assert(!r.Build());
 r=original; r.nodes[0].p.x=std::numeric_limits<double>::quiet_NaN(); assert(!r.Build());
 r=original; r.speed=0; assert(!r.Build());
 r=original; r.nodes[0].wait=-1; assert(!r.Build());
 r=original; r.nodes.resize(2); assert(!r.Build());
 r=original; r.nodes.resize(129); assert(!r.Build());
 // Two continents: no interpolation across the transition, but waits still apply.
 GMRoute::Route multi;multi.speed=10;multi.map=1;
 multi.nodes={{{0,0,20},2,1},{{100,0,20},3,1},{{-9000,100,30},4,0},{{-8900,100,30},0,0}};
 assert(multi.Build());assert(multi.period==29000);
 assert(multi.At(14999).map==1 && multi.At(14999).stopped);
 assert(multi.At(15000).map==0 && multi.At(15000).stopped);
 assert(GMRoute::Length(multi.At(15000).p-multi.nodes[2].p)<1e-8);
 assert(multi.At(28999).map==0 && multi.At(29000).map==1);
 assert(std::abs(multi.Curve(0,0.5).x-50)<1e-8); // Other continent cannot affect tangents.
 assert(std::isfinite(multi.Facing(1,0)));
 // A return leg on the first continent joins the first point normally.
 multi.nodes.push_back({{200,100,20},0,1});multi.nodes.push_back({{100,100,20},0,1});
 assert(multi.Build());assert(!multi.Transition(5));
 // Three-map loop and periodicity.
 multi.nodes[4].map=571;multi.nodes[5].map=571;assert(multi.Build());
 bool seen0=false,seen1=false,seen571=false;
 for(uint32_t t=0;t<multi.period;t+=10) {auto p=multi.At(t);seen0|=p.map==0;seen1|=p.map==1;seen571|=p.map==571;assert(std::isfinite(p.facing));assert(multi.At(t+multi.period).map==p.map);}
 assert(seen0 && seen1 && seen571);
 multi.nodes.erase(multi.nodes.begin()+5);assert(!multi.Build()); // Reject isolated arrival node.
 std::cout<<"PASS: two/three-map loops, transitions, pauses, map-zero, boundary tangents and isolated-node rejection\n";
 std::cout<<"PASS: closed curve, waits, periodic motion, finite poses and invalid route rejection\n";
}
