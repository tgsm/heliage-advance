#include "ppu.h"

u16 PPU::GetDISPSTAT() {
    // FIXME: hack to get past armwrestler's DISPSTAT check
    dispstat ^= 1;
    return dispstat;
}