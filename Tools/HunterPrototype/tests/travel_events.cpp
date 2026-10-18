#include "../shared/HunterTravelEvents.h"
#include <cassert>
#include <iostream>
int main() {
 using namespace HunterTravelEvents;
 assert(!Publish(42,22,40,0));
 Watch(42,100);
 assert(!Publish(42,0,40,0));
 assert(Publish(42,22,40,0));
 Watch(42,100); // World updates must not erase an incoming clue.
 auto events=Drain();assert(events.size()==1 && events[0].generation==100 && events[0].destination==40);
 assert(Drain().empty());
 Publish(42,22,30,0);Publish(42,30,40,0);
 events=Drain();assert(events.size()==1 && events[0].source==30 && events[0].destination==40);
 Publish(42,22,40,0);Watch(42,101);
 assert(Drain().empty()); // A replacement contract cannot inherit a stale alert.
 Publish(42,22,40,0);Unwatch(42);
 assert(Drain().empty() && !Publish(42,22,40,0));
 std::cout << "PASS: only watched contracts, latest clue, idempotent subscription, contract replacement and cancellation\n";
}
