#pragma once
#include <cmath>
namespace GMGizmo {
struct Vec {double x=0,y=0,z=0;};
struct Ray {Vec origin,direction;};
inline double Dot(Vec a,Vec b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool Finite(Vec p) {return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
// Closest point on an axis to the cursor ray. Reject an axis viewed end-on.
inline bool AxisParameter(Vec anchor,int axis,Ray ray,double& value) {
    if(axis<1||axis>3||!Finite(anchor)||!Finite(ray.origin)||!Finite(ray.direction)) return false;
    double norm=std::sqrt(Dot(ray.direction,ray.direction));if(norm<1e-9) return false;
    Vec d{ray.direction.x/norm,ray.direction.y/norm,ray.direction.z/norm};
    Vec v{ray.origin.x-anchor.x,ray.origin.y-anchor.y,ray.origin.z-anchor.z};
    double cosine=axis==1?d.x:axis==2?d.y:d.z;
    double along=axis==1?v.x:axis==2?v.y:v.z;
    double denominator=1-cosine*cosine;if(denominator<0.0025) return false;
    value=(along-cosine*Dot(d,v))/denominator;
    double rayDistance=cosine*value-Dot(d,v);
    return std::isfinite(value)&&rayDistance>=0;
}
inline Vec Add(Vec a,Vec b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec Sub(Vec a,Vec b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec Scale(Vec p,double s) {return {p.x*s,p.y*s,p.z*s};}
inline Vec Cross(Vec a,Vec b) {return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline Vec Axis(int axis) {return {axis==1?1.:0.,axis==2?1.:0.,axis==3?1.:0.};}
inline bool PlanePoint(Vec anchor,Vec normal,Ray ray,Vec& p) {
    if(!Finite(anchor)||!Finite(normal)||!Finite(ray.origin)||!Finite(ray.direction)) return false;
    double norm=std::sqrt(Dot(ray.direction,ray.direction));if(norm<1e-9) return false;
    Vec d=Scale(ray.direction,1/norm);double denominator=Dot(normal,d);
    if(std::abs(denominator)<0.025) return false;
    double t=Dot(normal,Sub(anchor,ray.origin))/denominator;
    if(!std::isfinite(t)||t<0||t>100000) return false;
    p=Add(ray.origin,Scale(d,t));return Finite(p);
}
// Modes 1/2/3 constrain one axis; 4/5/6 constrain XY/XZ/YZ; 7 uses the view plane.
struct Drag {
    int axis=0;Vec origin,lastPoint,normal,offsetPoint;double last=0,offset=0;
    bool Begin(Vec p,int a,Ray ray) {
        axis=0;if(a<1||a>7) return false;
        if(a<=3) {double scalar;if(!AxisParameter(p,a,ray,scalar)) return false;last=scalar;}
        else {
            normal=a==4?Axis(3):a==5?Axis(2):a==6?Axis(1):Sub(p,ray.origin);
            double norm=std::sqrt(Dot(normal,normal));if(norm<1e-9) return false;normal=Scale(normal,1/norm);
            if(!PlanePoint(p,normal,ray,lastPoint)) return false;
        }
        origin=p;offset=0;offsetPoint={};axis=a;return true;
    }
    bool Update(Ray ray,bool fine,Vec& p) {
        if(!axis) return false;
        if(axis<=3) {
            double scalar;if(!AxisParameter(origin,axis,ray,scalar)) return false;
            double change=(scalar-last)*(fine?0.1:1);last=scalar;
            if(!std::isfinite(change)||std::abs(change)>1000) return false;
            offset+=change;p=Add(origin,Scale(Axis(axis),offset));
        } else {
            Vec point;if(!PlanePoint(origin,normal,ray,point)) return false;
            Vec change=Sub(point,lastPoint);lastPoint=point;
            if(Dot(change,change)>1000000) return false;
            offsetPoint=Add(offsetPoint,Scale(change,fine?0.1:1));
            if(axis==4)offsetPoint.z=0;else if(axis==5)offsetPoint.y=0;else if(axis==6)offsetPoint.x=0;
            p=Add(origin,offsetPoint);
        }
        return Finite(p);
    }
};
struct Quat {double x=0,y=0,z=0,w=1;};
inline bool Unit(Quat q) {double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;return std::isfinite(n)&&std::abs(n-1)<0.002;}
inline Quat Normalize(Quat q) {double n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);if(!std::isfinite(n)||n<1e-9) return {};return {q.x/n,q.y/n,q.z/n,q.w/n};}
inline Quat Multiply(Quat a,Quat b) {
    return Normalize({a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z});
}
inline Quat Rotation(int axis,double angle) {Vec n=Scale(Axis(axis),std::sin(angle/2));return {n.x,n.y,n.z,std::cos(angle/2)};}
inline double Yaw(Quat q) {return std::atan2(2*(q.w*q.z+q.x*q.y),1-2*(q.y*q.y+q.z*q.z));}
inline Vec Rotate(Quat q,Vec v) {
    Vec t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
    return {v.x+q.w*t.x+q.y*t.z-q.z*t.y,v.y+q.w*t.y+q.z*t.x-q.x*t.z,v.z+q.w*t.z+q.x*t.y-q.y*t.x};
}
inline Vec RingPoint(int axis,double angle) {
    if(axis==1)return {0,std::cos(angle),std::sin(angle)};
    if(axis==2)return {std::sin(angle),0,std::cos(angle)};
    return {std::cos(angle),std::sin(angle),0};
}
struct RotateDrag {
    int axis=0;Vec origin,viewNormal,lastPoint,tangent;Quat initial;double last=0,angle=0,radius=1;bool edgeOn=false;
    bool Angle(Ray ray,double& result) const {
        Vec p;if(!PlanePoint(origin,Axis(axis),ray,p)) return false;p=Sub(p,origin);
        if(Dot(p,p)<1e-8) return false;
        result=axis==1?std::atan2(p.z,p.y):axis==2?std::atan2(p.x,p.z):std::atan2(p.y,p.x);return true;
    }
    bool Begin(Vec p,int a,Quat q,Ray ray) {
        axis=0;if(a<1||a>3||!Unit(q)) return false;origin=p;axis=a;
        double norm=std::sqrt(Dot(ray.direction,ray.direction));if(norm<1e-9){axis=0;return false;}
        edgeOn=std::abs(Dot(Axis(a),Scale(ray.direction,1/norm)))<0.12;
        if(edgeOn) {
            viewNormal=Sub(p,ray.origin);norm=std::sqrt(Dot(viewNormal,viewNormal));if(norm<1e-9){axis=0;return false;}viewNormal=Scale(viewNormal,1/norm);
            tangent=Cross(Axis(a),viewNormal);norm=std::sqrt(Dot(tangent,tangent));if(norm<1e-9 || !PlanePoint(p,viewNormal,ray,lastPoint)){axis=0;return false;}tangent=Scale(tangent,1/norm);
            Vec delta=Sub(lastPoint,p);radius=std::fmax(0.25,std::sqrt(Dot(delta,delta)));
        } else if(!Angle(ray,last)){axis=0;return false;}
        initial=q;angle=0;return true;
    }
    bool Update(Ray ray,bool fine,Quat& q) {
        if(!axis) return false;double delta;
        if(edgeOn) {Vec point;if(!PlanePoint(origin,viewNormal,ray,point)) return false;delta=Dot(Sub(point,lastPoint),tangent)/radius;lastPoint=point;}
        else {double next;if(!Angle(ray,next)) return false;delta=std::remainder(next-last,6.283185307179586);last=next;}
        angle+=delta*(fine?0.1:1);q=Multiply(Rotation(axis,angle),initial);return Unit(q);
    }
};
}
