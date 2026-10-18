#pragma once
#include <string>
#include <cstdint>
#include <cctype>
namespace HunterBoardInput {
struct Request {std::string name;uint32_t gold=0;};
inline std::string Trim(std::string s) {
    auto space=[](unsigned char c){return std::isspace(c)!=0;};
    while(!s.empty() && space(s.front()))s.erase(s.begin());
    while(!s.empty() && space(s.back()))s.pop_back();return s;
}
inline bool Parse(std::string const& text,Request& out) {
    out={};if(text.size()>80)return false;
    auto pos=text.find(';');if(pos==std::string::npos || text.find(';',pos+1)!=std::string::npos)return false;
    auto name=Trim(text.substr(0,pos)),amount=Trim(text.substr(pos+1));
    if(name.empty() || name.size()>48 || amount.empty() || amount.size()>6)return false;
    for(unsigned char c:name)if(c<32 || c==127 || c=='|')return false;
    uint32_t gold=0;for(char c:amount){if(c<'0'||c>'9')return false;gold=gold*10+uint32_t(c-'0');if(gold>100000)return false;}
    if(!gold)return false;out={name,gold};return true;
}
}
