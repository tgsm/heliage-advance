#include "gba.h"
#include "logging.h"

GBA::GBA(Cartridge& cartridge)
    : mmu(cartridge, keypad, ppu), arm7(mmu, ppu), ppu(mmu) {
    LINFO("powering on...");
}

void GBA::Run() {
    arm7.Step(false);
}
