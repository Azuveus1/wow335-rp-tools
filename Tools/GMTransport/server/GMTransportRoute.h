#pragma once
#include "Define.h"
#include "GMRouteMath.h"
#include <string>
#include "DBCStores.h"
struct GameObjectTemplate;
struct TransportTemplate;
class Player;
class ChatHandler;
namespace GMTransport {
TC_GAME_API GameObjectTemplate const* Template(uint32 entry);
TC_GAME_API TransportTemplate const* Path(uint32 entry);
TC_GAME_API bool Position(uint32 entry,uint32 time,GMRoute::Pose& pose);
TC_GAME_API void Load();
TC_GAME_API void Sync(Player* player);
TC_GAME_API uint32 Save(uint32 source,GMRoute::Route const& route);
TC_GAME_API bool Activate(uint32 entry,Player* player);
TC_GAME_API bool Deactivate(uint32 entry,Player* player);
TC_GAME_API void LoadNative();
TC_GAME_API TaxiPathNodeList const* NativePath(uint32 entry);
TC_GAME_API void Catalogue(Player* player);
TC_GAME_API bool Edit(Player* player,uint32 entry);
TC_GAME_API uint32 Revision(uint32 entry);
TC_GAME_API bool Update(uint32 entry,uint32 revision,GMRoute::Route const& route,Player* player);
TC_GAME_API bool UpdateNative(uint32 entry,uint32 revision,double speed,std::vector<GMRoute::Node> const& nodes);
TC_GAME_API bool AnnouncementCommand(ChatHandler*,char const*);
TC_GAME_API void LoadAnnouncements();
TC_GAME_API void TickAnnouncements();
TC_GAME_API void CustomAnnouncements(uint32 entry,uint32 oldTime,uint32 diff);
TC_GAME_API void NativeAnnouncement(uint32 entry,uint32 node,bool departure);
TC_GAME_API bool SuppressTalk(uint32 entry,int32 script,uint32 type,uint32 action);
}
