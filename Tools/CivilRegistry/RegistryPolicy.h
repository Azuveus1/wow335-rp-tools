#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
namespace CivilRegistry {
constexpr uint32_t Registrar=9100300,Notary=9100301,Paper=9100302,Tabard=9100303,Ring=9100304,Jeweler=2849;
constexpr uint32_t RenameCost=100000,ContractCost=250000,Month=30*24*60*60;
constexpr std::array<int,5> Counts{170,17,6,17,51};
using Design=std::array<int,5>;
constexpr uint32_t DesignCount=170*17*6*17*51;
inline bool Valid(Design const& d){for(unsigned i=0;i<5;++i)if(d[i]<0||d[i]>=Counts[i])return false;return true;}
inline uint32_t Pack(Design const& d){uint32_t n=0;for(int i=4;i>=0;--i)n=n*Counts[i]+d[i];return n;}
inline bool Unpack(uint32_t n,Design& d){if(n>=DesignCount)return false;for(unsigned i=0;i<5;++i){d[i]=n%Counts[i];n/=Counts[i];}return true;}
inline uint16_t Low(uint32_t n){return uint16_t(0x8000|(n&0xfff));}
inline uint16_t High(uint32_t n){return uint16_t(0x8000|((n>>12)&0xfff));}
inline bool Decode(uint16_t low,uint16_t high,Design& d){return (low&0xf000)==0x8000&&(high&0xf000)==0x8000&&Unpack((low&0xfff)|((high&0xfff)<<12),d);}
inline std::string First(std::string const& n){return n.substr(0,n.find(' '));}
inline std::string Family(std::string const& n){auto at=n.find(' ');return at==std::string::npos?"":n.substr(at+1);}
inline std::string Common(std::string const& buyer,uint8_t buyerGender,std::string const& partner,uint8_t partnerGender){return Family(buyerGender!=partnerGender&&partnerGender==0?partner:buyer);}
inline bool Due(uint64_t now,uint64_t last){return !last||(now>=last&&now-last>=Month);}
}
