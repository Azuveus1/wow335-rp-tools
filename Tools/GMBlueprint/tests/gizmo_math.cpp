#include "../shared/GizmoMath.h"
#include <cassert>
#include <limits>
#include <iostream>
using namespace GMGizmo;
int main() {
 Vec anchor{1000,-2000,30},p;
 Drag d;assert(d.Begin(anchor,1,{{1000,-2010,40},{0,10,-10}}));
 assert(d.Update({{1000,-2010,40},{4,10,-10}},false,p));assert(std::abs(p.x-1004)<1e-9 && p.y==anchor.y && p.z==anchor.z);
 assert(d.Update({{1000,-2010,40},{14,10,-10}},true,p));assert(std::abs(p.x-1005)<1e-9);
 assert(d.Update({{1000,-2010,40},{15,10,-10}},false,p));assert(std::abs(p.x-1006)<1e-9);
 assert(d.Update({{1000,-2010,40},{-5,10,-10}},false,p));assert(std::abs(p.x-986)<1e-9);
 assert(d.Begin(anchor,2,{{990,-2000,35},{10,0,-5}}));assert(d.Update({{990,-2000,35},{10,3,-5}},false,p));assert(p.x==anchor.x && std::abs(p.y+1997)<1e-9 && p.z==anchor.z);
 assert(d.Begin(anchor,3,{{1000,-2010,35},{0,10,-5}}));assert(d.Update({{1000,-2010,35},{0,10,-3}},false,p));assert(p.x==anchor.x && p.y==anchor.y && std::abs(p.z-32)<1e-9);
 double scalar=0;
 assert(!AxisParameter({},1,{{},{1,0,0}},scalar));
 assert(!AxisParameter({},1,{{},{1,0.01,0}},scalar));
 assert(!AxisParameter({},0,{{},{0,1,0}},scalar));
 assert(!AxisParameter({},1,{{0,-10,5},{0,-10,5}},scalar));
 assert(!AxisParameter({},1,{{},{std::numeric_limits<double>::quiet_NaN(),1,1}},scalar));
 assert(!d.Begin(anchor,3,{{},{0,0,0}}) && !d.axis);

 // Two-axis planes preserve the excluded coordinate, including at large world positions.
 anchor={0,0,0};
 assert(d.Begin(anchor,4,{{0,-10,10},{0,10,-10}}));assert(d.Update({{0,-10,10},{3,14,-10}},false,p));assert(std::abs(p.x-3)<1e-9 && std::abs(p.y-4)<1e-9 && p.z==0);
 assert(d.Begin(anchor,5,{{0,-10,10},{0,10,-10}}));assert(d.Update({{0,-10,10},{3,10,-8}},false,p));assert(std::abs(p.x-3)<1e-9 && p.y==0 && std::abs(p.z-2)<1e-9);
 assert(d.Begin(anchor,6,{{-10,0,10},{10,0,-10}}));assert(d.Update({{-10,0,10},{10,3,-8}},false,p));assert(p.x==0 && std::abs(p.y-3)<1e-9 && std::abs(p.z-2)<1e-9);
 assert(d.Begin(anchor,7,{{0,-10,10},{0,10,-10}}));assert(d.Update({{0,-10,10},{3,14,-10}},false,p));assert(p.x>0 && p.y>0 && p.z>0);
 assert(!d.Begin(anchor,4,{{0,-10,0},{0,10,0}}));
 RotateDrag turn;Quat q;
 assert(turn.Begin({},1,{},{{10,0,0},{-10,1,0}}));assert(turn.Update({{10,0,0},{-10,0,1}},false,q));
 Vec rotated=Rotate(q,{0,1,0});assert(std::abs(rotated.y)<1e-9 && std::abs(rotated.z-1)<1e-9 && Unit(q));
 assert(turn.Begin({},2,q,{{0,10,0},{0,-10,1}}));assert(turn.Update({{0,10,0},{1,-10,0}},false,q));assert(Unit(q));
 rotated=Rotate(q,{0,1,0});assert(std::abs(rotated.x-1)<1e-9 && std::abs(rotated.y)<1e-9);
 assert(turn.Begin({},3,{},{{0,0,10},{1,0,-10}}));assert(turn.Update({{0,0,10},{0,1,-10}},true,q));assert(std::abs(Yaw(q)-0.15707963267948966)<1e-9);
 assert(turn.Begin({},3,{},{{0,0,10},{-1,0.01,-10}}));assert(turn.Update({{0,0,10},{-1,-0.01,-10}},false,q));assert(std::abs(Yaw(q)-2*std::atan(0.01))<1e-9);
 assert(turn.Begin({},1,{},{{0,-10,0},{1,10,0}}) && turn.edgeOn);assert(turn.Update({{0,-10,0},{1,10,1}},false,q) && Unit(q));assert(std::abs(q.x)>0.1);
 assert(!Unit({0,0,0,0}) && !Unit({0,0,0,1.1}) && !Unit({std::numeric_limits<double>::quiet_NaN(),0,0,1}));
 for(int i=0;i<10000;++i) q=Multiply(Rotation(i%3+1,0.003),q);assert(Unit(q));
 std::cout<<"PASS: XY/XZ/YZ constraints, XYZ view plane, full world-axis rotations, quaternion composition, Ctrl precision, angle wrap, edge-on rings and normalization\n";
 std::cout<<"PASS: X/Y/Z isolation, world coordinates, perspective rays, continuous Ctrl precision, negative movement, parallel/behind-camera/invalid ray rejection\n";
}
