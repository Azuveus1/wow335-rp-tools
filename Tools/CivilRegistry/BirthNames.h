#pragma once
#include "DatabaseEnv.h"
#include <string>
namespace BirthNames {
inline bool Taken(std::string name,uint32 owner=0){
 CharacterDatabase.EscapeString(name);
 auto q=CharacterDatabase.PQuery("SELECT player FROM rp_registry_name_claim WHERE name='{}' AND player<>{}",name,owner);
 return bool(q);
}
inline std::string Get(uint32 player){auto q=CharacterDatabase.PQuery("SELECT name FROM rp_registry_birth_name WHERE player={}",player);return q?q->Fetch()[0].GetString():"";}
inline std::string Escape(std::string name){CharacterDatabase.EscapeString(name);return name;}
inline void AppendIdentity(CharacterDatabaseTransaction tx,uint32 player,std::string const& name){
 // A duplicate belonging to another identity must fail the transaction. Unlike
 // INSERT IGNORE/REPLACE this cannot silently steal or discard a reservation.
 auto escaped=Escape(name);
 tx->PAppend("INSERT INTO rp_registry_name_claim(name,player) SELECT '{}',{} WHERE NOT EXISTS(SELECT 1 FROM rp_registry_name_claim WHERE name='{}' AND player={})",escaped,player,escaped,player);
 if(name.find(' ')!=std::string::npos)
  tx->PAppend("INSERT INTO rp_registry_birth_name(player,name,created_at) SELECT {},'{}',UNIX_TIMESTAMP() WHERE NOT EXISTS(SELECT 1 FROM rp_registry_birth_name WHERE player={})",player,escaped,player);
}
inline void AppendRename(CharacterDatabaseTransaction tx,uint32 player,std::string const& oldName,std::string const& newName){
 AppendIdentity(tx,player,newName);
 if(oldName!=newName)tx->PAppend("DELETE FROM rp_registry_name_claim WHERE player={} AND name='{}' AND NOT EXISTS(SELECT 1 FROM rp_registry_birth_name WHERE player={} AND name='{}')",player,Escape(oldName),player,Escape(oldName));
}
}
