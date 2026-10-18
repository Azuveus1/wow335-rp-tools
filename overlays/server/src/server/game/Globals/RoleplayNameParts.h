#pragma once
#include <string_view>
namespace RoleplayNames {
// Existing character packets/cache entries have 48-byte UTF-8 name buffers.
constexpr size_t MaxBytes=47;
inline bool Split(std::string_view name,std::string_view& first,std::string_view& last) {
 if(name.empty() || name.size()>MaxBytes)return false;
 auto space=name.find(' ');
 if(space==std::string_view::npos || !space || space+1==name.size() || name.find(' ',space+1)!=std::string_view::npos)return false;
 first=name.substr(0,space);last=name.substr(space+1);
 for(unsigned char c:name)if(c<32 || c==127)return false;
 return true;
}
}
