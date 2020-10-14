#pragma once

#include "types.h"

class PPU {
public:
    u16 GetDISPSTAT();
private:
    u16 dispstat = 0x0000;
};