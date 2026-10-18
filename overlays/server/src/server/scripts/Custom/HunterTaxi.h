#pragma once
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "Creature.h"
#include "PathGenerator.h"
#include "DisableMgr.h"
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "Log.h"
#include <queue>
#include <unordered_map>
#include <limits>
#include <algorithm>
#include <cmath>

namespace HunterTaxi {
enum Phase : uint32 {Idle,Walking,Flying,Waiting,Blocked};
struct State {
 uint32 clueSource=0,destination=0,phase=Idle,source=0,path=0,nextNode=0,flightTo=0;
};
inline bool Node(uint32 id,uint32 map) {
 auto n=sTaxiNodesStore.LookupEntry(id);if(!n || n->ContinentID!=map || !n->MountCreatureID[0])return false;
 constexpr size_t width=sizeof(TaxiMask::value_type)*8;
 size_t bit=id-1;return bit/width<sTaxiNodesMask.size() && (sTaxiNodesMask[bit/width] & (TaxiMask::value_type(1)<<(bit%width)));
}
inline bool Path(uint32 id,uint32 map) {
 if(id>=sTaxiPathNodesByPath.size())return false;auto const& p=sTaxiPathNodesByPath[id];if(p.size()<2)return false;
 for(auto n:p)if(!n || n->ContinentID!=map)return false;return true;
}
inline double Length(uint32 id) {
 auto const& points=sTaxiPathNodesByPath[id];double sum=0;
 for(size_t i=1;i<points.size();++i){auto a=points[i-1]->Loc,b=points[i]->Loc;double x=a.X-b.X,y=a.Y-b.Y,z=a.Z-b.Z;sum+=std::sqrt(x*x+y*y+z*z);}
 return std::max(sum,1.0);
}
// All regular Horde nodes are known to the NPC; costs are physical path lengths.
inline std::vector<uint32> Route(uint32 source,uint32 dest,uint32 map) {
 if(!Node(source,map) || !Node(dest,map))return {};
 using Item=std::pair<double,uint32>;std::priority_queue<Item,std::vector<Item>,std::greater<Item>> todo;
 std::unordered_map<uint32,double> distances;std::unordered_map<uint32,uint32> previous;
 distances[source]=0;todo.push({0,source});
 while(!todo.empty()) {
  auto [distance,current]=todo.top();todo.pop();if(distance!=distances[current])continue;if(current==dest)break;
  auto edges=sTaxiPathSetBySource.find(current);if(edges==sTaxiPathSetBySource.end())continue;
  for(auto const& e:edges->second) {
   uint32 next=e.first,path=e.second.ID;if(!Node(next,map) || !Path(path,map))continue;
   double candidate=distance+Length(path);auto old=distances.find(next);
   if(old==distances.end() || candidate<old->second){distances[next]=candidate;previous[next]=current;todo.push({candidate,next});}
  }
 }
 if(!distances.count(dest))return {};
 std::vector<uint32> result{dest};for(uint32 n=dest;n!=source;){n=previous.at(n);result.push_back(n);}std::reverse(result.begin(),result.end());return result;
}
class Driver {
 Creature* me;
 State state;
 Position goal;
 bool launch=false,prepared=false,interrupted=false;
 uint32 timer=0,elapsed=0,legs=0,airEnd=0,airStart=0;
 std::string reason;
 void GroundFlags(){me->RemoveUnitFlag(UNIT_FLAG_ON_TAXI);me->SetDisableGravity(false);me->SetCanFly(false);me->Dismount();}
 void Stop(){me->StopMoving();me->GetMotionMaster()->Clear();me->GetMotionMaster()->MoveIdle();}
 void Fail(std::string why){reason=std::move(why);state.phase=Blocked;launch=false;Stop();GroundFlags();Report("blocked");}
 void Report(char const* action){TC_LOG_INFO("scripts","Hunter travel {}: target={} phase={} clueSource={} destination={} source={} path={} node={} position=({},{},{}) reason={}",action,me->GetGUID().ToString(),state.phase,state.clueSource,state.destination,state.source,state.path,state.nextNode,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),reason);}
 bool WalkLeg() {
  // At an intermediate airport the previous flight has already put us beside
  // the next departure point; a zero-length ground path is not a failure.
  if(me->GetExactDist2d(&goal)<1.5f && std::abs(me->GetPositionZ()-goal.GetPositionZ())<2.f){state.phase=Flying;FlightChunk();return true;}
  if(!DisableMgr::IsPathfindingEnabled(me->GetMapId()) || ++legs>64){Fail("ground_navigation_unavailable");return false;}
  PathGenerator path(me);bool built=path.CalculatePath(goal.GetPositionX(),goal.GetPositionY(),goal.GetPositionZ(),false);
  if(!built || (path.GetPathType()&PATHFIND_SHORT)){path.SetUseStraightPath(true);built=path.CalculatePath(goal.GetPositionX(),goal.GetPositionY(),goal.GetPositionZ(),false);}
  uint32 flags=path.GetPathType();
  if(!built || flags&(PATHFIND_SHORTCUT|PATHFIND_NOPATH|PATHFIND_NOT_USING_PATH|PATHFIND_SHORT) || path.GetPath().size()<2){Fail("ground_navmesh_refused_"+std::to_string(flags));return false;}
  auto last=path.GetPath().back();if(me->GetExactDist(last.x,last.y,last.z)<.5f){Fail("ground_no_progress");return false;}
  Movement::MoveSplineInit move(me);move.MovebyPath(path.GetPath());move.SetWalk(false);move.Launch();elapsed=0;return true;
 }
 void FlightChunk() {
  if(!Path(state.path,me->GetMapId())){Fail("unsupported_flight_path");return;}
  auto const& path=sTaxiPathNodesByPath[state.path];
  if(state.nextNode>=path.size()){Land();return;}
  uint32 mount=sObjectMgr->GetTaxiMountDisplayId(state.source,HORDE,false);if(!mount){Fail("missing_horde_mount");return;}
  me->Mount(mount);me->SetCanFly(true);me->SetDisableGravity(true);me->SetUnitFlag(UNIT_FLAG_ON_TAXI);me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
  airStart=state.nextNode;airEnd=std::min(uint32(path.size()),airStart+60u);
  auto end=path[airEnd-1]->Loc;me->GetMap()->LoadGrid(end.X,end.Y);
  Movement::MoveSplineInit move(me);move.Path().push_back(G3D::Vector3(me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()));
  for(uint32 i=airStart;i<airEnd;++i){auto p=path[i]->Loc;move.Path().push_back(G3D::Vector3(p.X,p.Y,p.Z));}
  if(airEnd==path.size())if(auto node=sTaxiNodesStore.LookupEntry(state.flightTo))move.Path().push_back(G3D::Vector3(node->Pos.X,node->Pos.Y,node->Pos.Z));
  move.SetFirstPointId(airStart);move.SetFly();move.SetWalk(false);move.SetVelocity(32.0f);move.Launch();elapsed=0;Report("flight_chunk");
 }
 void Plan(bool atNode=false) {
  GroundFlags();reason.clear();state.path=0;state.nextNode=0;legs=0;
  state.source=atNode?state.flightTo:sObjectMgr->GetNearestTaxiNode(me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),me->GetMapId(),HORDE);
  auto route=Route(state.source,state.destination,me->GetMapId());
  if(route.empty()){Fail("no_horde_flight_route_on_this_map");return;}
  if(route.size()==1){state.phase=Waiting;me->SetHomePosition(me->GetPosition());Report("waiting_at_destination");return;}
  state.flightTo=route[1];uint32 cost=0;sObjectMgr->GetTaxiPath(state.source,state.flightTo,state.path,cost);
  auto source=sTaxiNodesStore.LookupEntry(state.source);goal.Relocate(source->Pos.X,source->Pos.Y,source->Pos.Z,me->GetOrientation());
  // The tested safe standing point beside Doras is supplied by the caller.
  if(state.source==23 && me->GetMapId()==1)goal=orgFlight;
  state.phase=Walking;launch=true;Report("to_flightmaster");
 }
 void Land() {
  GroundFlags();me->SetHomePosition(me->GetPosition());Report("landed");
  if(!allowDeparture){state.phase=Idle;state.source=state.flightTo;launch=false;return;}
  if(state.flightTo==state.destination){state.phase=Waiting;state.source=state.flightTo;state.path=0;state.nextNode=0;Report("waiting_at_destination");}
  else Plan(true);
 }
public:
 bool allowDeparture=true;
 Position orgFlight{1677.8f,-4316.f,61.9445f,5.25344f};
 explicit Driver(Creature* owner):me(owner){}
 bool FlyingNow()const{return state.phase==Flying;}
 bool Travelling()const{return state.phase==Walking || state.phase==Flying;}
 State Export()const {
  State out=state;
  if(FlyingNow() && !launch && !me->movespline->Finalized())out.nextNode=std::max(airStart,std::min(airEnd,uint32(std::max(int32(0),me->movespline->currentPathIdx()))));
  return out;
 }
 void Restore(State const& saved){prepared=false;state=saved;if(state.phase==Flying){launch=true;}else if(state.destination && state.phase!=Waiting && state.phase!=Blocked){state.phase=Idle;launch=true;}}
 void Clue(uint32 source,uint32 destination){state.clueSource=source;state.destination=destination;reason.clear();Report("clue_received");if(!FlyingNow())me->Say("Une piste ! Je rejoins le maitre de vol.",LANG_UNIVERSAL);if(!FlyingNow()){state.phase=Idle;launch=true;prepared=false;}}
 void Tick(uint32 diff) {
  if(!state.destination || state.phase==Blocked || (state.phase==Waiting && !launch) || (!allowDeparture && !FlyingNow()))return;
  if(me->IsInCombat()){interrupted=true;return;}
  if(interrupted){interrupted=false;launch=true;}
  timer+=diff;elapsed+=diff;if(timer<250)return;timer=0;
  if(launch){if(!prepared){Stop();prepared=true;return;}launch=false;prepared=false;if(FlyingNow())FlightChunk();else {Plan();launch=false;if(state.phase==Walking)WalkLeg();}return;}
  if(state.phase==Walking) {
   if(elapsed>300000){Fail("ground_travel_timeout");return;}
   if(!me->movespline->Finalized())return;
   if(me->GetExactDist2d(&goal)<1.5f && std::abs(me->GetPositionZ()-goal.GetPositionZ())<2.f){state.phase=Flying;FlightChunk();}
   else WalkLeg();
  } else if(FlyingNow()) {
   if(elapsed>1200000){Fail("flight_timeout");return;}
   if(me->movespline->Finalized()){state.nextNode=airEnd;auto const& path=sTaxiPathNodesByPath[state.path];if(state.nextNode>=path.size())Land();else FlightChunk();}
  }
 }
};
}

