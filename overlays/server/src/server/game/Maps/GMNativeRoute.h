#pragma once
#include "GMRouteMath.h"
#include <set>
namespace GMRoute {
// Native paths contain spline control points and transition sentinels as well as stops.
inline bool ValidNative(std::vector<Node> const& nodes) {
    if(nodes.size()<4 || nodes.size()>1024) return false;
    size_t legSize=0,frames=0,legFrames=0;
    bool skip=false;std::set<uint32_t> ids;
    for(size_t i=0;i<nodes.size();++i) {
        auto const& n=nodes[i];
        if(!std::isfinite(n.p.x)||!std::isfinite(n.p.y)||!std::isfinite(n.p.z)||std::abs(n.p.x)>17000||std::abs(n.p.y)>17000||std::abs(n.p.z)>10000||!std::isfinite(n.wait)||n.wait<0||n.wait>600||n.wait!=std::floor(n.wait)||n.flags>2||n.map==InheritMap) return false;
        if(n.uid && !ids.insert(n.uid).second) return false;
        if(i && (nodes[i-1].map!=n.map || (nodes[i-1].flags&1))) {if(legSize<4) return false;legSize=0;}
        ++legSize;
        if(i && nodes[i-1].map==n.map && !(nodes[i-1].flags&1) && Length(n.p-nodes[i-1].p)<0.01) return false;
        if(skip) {skip=false;continue;}
        if(i+1<nodes.size() && ((n.flags&1)||n.map!=nodes[i+1].map)) {
            if(!frames || legFrames<2) return false;
            skip=true;legFrames=0;
        } else {++frames;++legFrames;}
    }
    return legSize>=4 && legFrames>=2 && frames>=4;
}
}
