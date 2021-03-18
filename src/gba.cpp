#include "gba.h"
#include "logging.h"

GBA::GBA(BIOS& bios, Cartridge& cartridge)
    : bus(bios, cartridge, keypad, ppu), arm7(bus, ppu), ppu(bus) {
    LINFO("powering on...");
}

void GBA::Run() {
    arm7.Step(false);
}
