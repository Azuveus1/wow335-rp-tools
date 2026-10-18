#include "../shared/GizmoMath.h"
#include "Engine/Source/Tempest/C4Quaternion.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include <G3D/Matrix3.h>
#include <G3D/Quat.h>
#include <cassert>
#include <iostream>
int main() {
 using namespace GMGizmo;
 for(int i=0;i<500;++i) {
  Quat q=Multiply(Rotation(3,i*.127),Multiply(Rotation(2,i*.071),Rotation(1,i*.093)));
  assert(Unit(q));Vec v{1.2,-2.3,3.4};auto expected=Rotate(q,v);
  C4Quaternion client;client.x=float(q.x);client.y=float(q.y);client.z=float(q.z);client.w=float(q.w);
  C44Matrix matrix(client);C3Vector transformed=C3Vector(float(v.x),float(v.y),float(v.z))*matrix;
  auto near=[&](double x,double y,double z,double tolerance){assert(std::abs(x-expected.x)<tolerance && std::abs(y-expected.y)<tolerance && std::abs(z-expected.z)<tolerance);};
  near(transformed.x,transformed.y,transformed.z,1e-5);
  G3D::Matrix3 collision(G3D::Quat(float(q.x),float(q.y),float(q.z),float(q.w)));
  auto server=collision*G3D::Vector3(float(v.x),float(v.y),float(v.z));near(server.x,server.y,server.z,1e-5);
  auto restored=collision.inverse()*server;assert((restored-G3D::Vector3(float(v.x),float(v.y),float(v.z))).length()<1e-5);
  C4QuaternionCompressed encoded;encoded=client;C4Quaternion decoded(encoded);C44Matrix received(decoded);
  C3Vector afterPacket=C3Vector(float(v.x),float(v.y),float(v.z))*received;near(afterPacket.x,afterPacket.y,afterPacket.z,0.01);
 }
 std::cout<<"PASS: 500 mixed 3D rotations match client transforms, server collision matrices, inverse transforms and compressed client packet rotations\n";
}
