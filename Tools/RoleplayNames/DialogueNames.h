#pragma once
#include "NameParts.h"
namespace RoleplayNames {
inline std::string_view DialogueName(std::string_view full,char token,bool player=true) {
 std::string_view first,last;
 bool split=player && Split(full,first,last);
 switch(token) {
  case 'P':case 'p':return split?first:full;
  case 'F':case 'f':return split?last:std::string_view{};
  default:return full;
 }
}
}
