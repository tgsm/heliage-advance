#pragma once

#include "arm7/arm7.h"
#include "bios.h"
#include "bus.h"
#include "cartridge.h"
#include "keypad.h"
#include "interrupts.h"
#include "ppu.h"
#include "timer.h"

class GBA {
public:
    GBA(BIOS& bios, Cartridge& cartridge);

    void Run();
private:
    PPU ppu;
    Bus bus;
    ARM7 arm7;
    Keypad keypad;
    Interrupts interrupts;
    Timers timers;
};
