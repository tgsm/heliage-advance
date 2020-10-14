#include "gba.h"
#include "logging.h"
#include "types.h"

GBA::GBA(Cartridge& cartridge)
    : mmu(cartridge, ppu), arm7(mmu) {
    LINFO("powering on...");
}

void GBA::Run() {
    for (u64 i = 0; ; i++) {
        arm7.Step(true);
        // for debugging armwrestler's drawtext function
        // arm7.Step(i >= 196836);
    }
}
