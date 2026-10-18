#pragma once
#include "RouteMath.h"
namespace GMRoute {
// WoW's camera view is rotation-only: subtract the camera origin before projection.
// Keep off-screen coordinates so the UI can clip links crossing the viewport.
template<class Matrix>
bool Project(Matrix const& m, Vec world, Vec camera, double& x, double& y) {
    Vec p=world-camera;
    double w=p.x*m.a3+p.y*m.b3+p.z*m.c3+m.d3;
    if(!std::isfinite(w)||w<=0.001) return false;
    x=((p.x*m.a0+p.y*m.b0+p.z*m.c0+m.d0)/w+1)*0.5;
    y=((p.x*m.a1+p.y*m.b1+p.z*m.c1+m.d1)/w+1)*0.5;
    return std::isfinite(x)&&std::isfinite(y);
}
}
