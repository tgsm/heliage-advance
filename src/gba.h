#pragma once

#include "arm7.h"
#include "cartridge.h"
#include "mmu.h"
#include "ppu.h"

class GBA {
public:
    GBA(Cartridge& cartridge);

    void Run();
private:
    MMU mmu;
    ARM7 arm7;
    PPU ppu;
};
