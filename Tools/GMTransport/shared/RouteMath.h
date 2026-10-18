#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// Shared verbatim by server, client and tests. Closed Catmull-Rom path,
// arc-length lookup for constant travel speed, optional waits at numbered nodes.
namespace GMRoute {
struct Vec { double x=0,y=0,z=0; };
inline Vec operator+(Vec a,Vec b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec operator-(Vec a,Vec b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline Vec operator*(Vec a,double s) { return {a.x*s,a.y*s,a.z*s}; }
inline double Length(Vec a) { return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z); }
constexpr uint32_t InheritMap = UINT32_MAX;
struct Node { Vec p; double wait=0; uint32_t map=InheritMap; uint32_t flags=0,arrival=0,departure=0,uid=0; };
struct Pose { Vec p; double facing=0; bool stopped=false; uint32_t map=0; };
struct Route {
    uint32_t map=0;
    double speed=10;
    std::vector<Node> nodes;
    std::vector<double> starts,departures,ends;
    std::vector<std::vector<double>> lengths;
    uint32_t period=0;
    uint32_t NodeMap(size_t i) const { return nodes[i].map==InheritMap?map:nodes[i].map; }
    bool Transition(size_t i) const { return NodeMap(i)!=NodeMap((i+1)%nodes.size()); }
    Vec Curve(size_t i,double t) const {
        const size_t n=nodes.size();
        Vec a=nodes[(i+n-1)%n].p,b=nodes[i].p,c=nodes[(i+1)%n].p,d=nodes[(i+2)%n].p;
        if(Transition(i)) return b; // There is no spatial curve between continents.
        if(Transition((i+n-1)%n)) a=b*2-c;
        if(Transition((i+1)%n)) d=c*2-b;
        return (b*2+(c-a)*t+(a*2-b*5+c*4-d)*(t*t)+(b*3-a-c*3+d)*(t*t*t))*0.5;
    }
    double Facing(size_t i,double t) const {
        if(Transition(i)) return Facing((i+nodes.size()-1)%nodes.size(),1);
        Vec v=Curve(i,std::min(1.0,t+0.0001))-Curve(i,std::max(0.0,t-0.0001));
        return std::atan2(v.y,v.x)+3.141592653589793;
    }
    bool Build() {
        period=0; starts.clear(); departures.clear(); ends.clear(); lengths.clear();
        if(nodes.size()<3 || nodes.size()>128 || !std::isfinite(speed) || speed<1 || speed>40) return false;
        for(auto& n:nodes) if(n.map==InheritMap) n.map=map;
        map=nodes.front().map;
        // Each visit to a continent needs at least two points to define its heading.
        for(size_t i=0;i<nodes.size();++i)
            if(Transition(i) && Transition((i+nodes.size()-1)%nodes.size())) return false;
        double total=0,clock=0;
        for(size_t i=0;i<nodes.size();++i) {
            const Node& n=nodes[i];
            if(!std::isfinite(n.p.x)||!std::isfinite(n.p.y)||!std::isfinite(n.p.z)||
               std::abs(n.p.x)>17000||std::abs(n.p.y)>17000||std::abs(n.p.z)>10000||
               !std::isfinite(n.wait)||n.wait<0||n.wait>600) return false;
            double chord=Length(nodes[(i+1)%nodes.size()].p-n.p);
            if(!Transition(i) && (chord<1 || chord>2000)) return false;
            std::vector<double> arc(33,0);
            Vec prev=Curve(i,0);
            for(int j=1;j<=32;++j) { Vec p=Curve(i,j/32.0); arc[j]=arc[j-1]+Length(p-prev); prev=p; }
            total+=arc.back();
            starts.push_back(clock); clock+=n.wait*1000; departures.push_back(clock);
            clock+=arc.back()/speed*1000; ends.push_back(clock); lengths.push_back(arc);
        }
        if(total>50000 || clock>86400000 || !std::isfinite(clock)) return false;
        period=static_cast<uint32_t>(std::ceil(clock)); return period>0;
    }
    Pose At(double ms) const {
        if(!period) return {};
        ms=std::fmod(std::max(0.0,ms),double(period));
        for(size_t i=0;i<nodes.size();++i) {
            if(ms<departures[i]) return {nodes[i].p,Facing(i,0),true,NodeMap(i)};
            if(ms<ends[i]) {
                const auto& arc=lengths[i];
                double distance=(ms-departures[i])/(ends[i]-departures[i])*arc.back();
                auto it=std::upper_bound(arc.begin(),arc.end(),distance);
                size_t j=std::min<size_t>(32,std::max<size_t>(1,it-arc.begin()));
                double t=(j-1+(distance-arc[j-1])/std::max(1e-9,arc[j]-arc[j-1]))/32.0;
                return {Curve(i,t),Facing(i,t),false,NodeMap(i)};
            }
        }
        return {nodes[0].p,Facing(0,0),false,NodeMap(0)};
    }
};
}
