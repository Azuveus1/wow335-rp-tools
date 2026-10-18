#pragma once
#include <string>
#include <string_view>
#include <cstddef>
namespace RoleplayNames {
struct NamePrefix {std::size_t begin=0,end=0;explicit operator bool()const{return end>begin;}};
inline bool CommandSpace(char c){return c==' '||c=='\t';}
// Keep command suffixes intact. Prefer an existing complete identity to the
// legacy first token; a quoted name is explicit, even when it doesn't exist.
template<class Exists> NamePrefix CommandNamePrefix(std::string_view text,Exists exists){
 std::size_t begin=0;while(begin<text.size()&&CommandSpace(text[begin]))++begin;
 if(begin==text.size()||text[begin]=='|')return {};
 if(text[begin]=='"'){
  auto end=text.find('"',begin+1);
  if(end!=std::string_view::npos&&(end+1==text.size()||CommandSpace(text[end+1])))return {begin+1,end};
  return {};
 }
 auto first=begin;while(first<text.size()&&!CommandSpace(text[first]))++first;
 auto second=first;while(second<text.size()&&CommandSpace(text[second]))++second;
 if(second==text.size())return {};
 auto end=second;while(end<text.size()&&!CommandSpace(text[end]))++end;
 // Character names contain exactly one space; don't absorb quoted parameters.
 if(second!=first+1||text[first]!=' '||text[second]=='"'||text[second]=='|')return {};
 if(exists(std::string(text.substr(begin,end-begin))))return {begin,end};
 return {};
}
}
