#pragma once
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>
// The core reports accepted player taxi itineraries, never live player positions.
namespace HunterTravelEvents {
struct Taxi {uint64_t target=0,generation=0;uint32_t source=0,destination=0,team=0;};
inline std::mutex guard;
inline std::unordered_map<uint64_t,uint64_t> watched;
inline std::unordered_map<uint64_t,Taxi> pending;
inline void Watch(uint64_t target,uint64_t generation) {std::lock_guard<std::mutex> lock(guard);auto it=watched.find(target);if(it==watched.end() || it->second!=generation){watched[target]=generation;pending.erase(target);}}
inline void Unwatch(uint64_t target) {std::lock_guard<std::mutex> lock(guard);watched.erase(target);pending.erase(target);}
inline bool Publish(uint64_t target,uint32_t source,uint32_t destination,uint32_t team) {
 std::lock_guard<std::mutex> lock(guard);auto it=watched.find(target);if(it==watched.end() || !source || !destination)return false;
 pending[target]={target,it->second,source,destination,team};return true;
}
inline std::vector<Taxi> Drain() {std::lock_guard<std::mutex> lock(guard);std::vector<Taxi> out;out.reserve(pending.size());for(auto const& pair:pending)out.push_back(pair.second);pending.clear();return out;}
}
