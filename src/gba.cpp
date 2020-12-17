#include "gba.h"
#include "logging.h"

GBA::GBA(BIOS& bios, Cartridge& cartridge)
    : mmu(bios, cartridge, keypad, ppu), arm7(mmu, ppu), ppu(mmu) {
    LINFO("powering on...");
}

void GBA::Run() {
    arm7.Step(false);
}
