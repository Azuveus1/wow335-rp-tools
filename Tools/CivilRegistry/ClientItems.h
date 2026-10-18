#pragma once
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "RegistryPolicy.h"
// These three server-defined items deliberately reuse existing client artwork.
// All other item lookups keep the original DBC behavior.
inline ItemRec* RegistryItemRecord(int32_t id) {
    int source=0,index=0;
    switch(id) {
    case CivilRegistry::Paper: source=5863;index=0;break;
    case CivilRegistry::Tabard: source=5976;index=1;break;
    case CivilRegistry::Ring: source=7337;index=2;break;
    default:return g_itemDB.GetRecord(id);
    }
    auto original=g_itemDB.GetRecord(source);
    if(!original)return nullptr;
    static ItemRec records[3];
    if(records[index].m_id!=id){records[index]=*original;records[index].m_id=id;}
    return &records[index];
}
