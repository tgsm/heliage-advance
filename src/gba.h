#pragma once

#include "arm7/arm7.h"
#include "bios.h"
#include "bus.h"
#include "cartridge.h"
#include "keypad.h"
#include "ppu.h"

class GBA {
public:
    GBA(BIOS& bios, Cartridge& cartridge);

    void Run();
private:
    Bus bus;
    ARM7 arm7;
    PPU ppu;
    Keypad keypad;
};
