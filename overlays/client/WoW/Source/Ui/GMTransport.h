#pragma once
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
void GMTransportRegister();
void GMTransportUnregister();
void GMTransportClearPreview();
bool GMTransportPosition(uint32_t entry,uint32_t time,int32_t& map,C3Vector& position,float& facing);

class ShipPath;
void GMTransportPrepareShip(uint32_t entry,ShipPath& path);
