#include "gba.h"
#include "logging.h"
#include "types.h"

GBA::GBA(Cartridge& cartridge)
    : mmu(cartridge, ppu), arm7(mmu, ppu) {
    LINFO("powering on...");
}

void GBA::Run() {
    arm7.Step(true);
}
