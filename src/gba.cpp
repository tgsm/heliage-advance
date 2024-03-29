#include "gba.h"
#include "common/logging.h"

GBA::GBA(BIOS& bios, Cartridge& cartridge)
    : ppu(bus, interrupts),
      bus(bios, cartridge, keypad, ppu, interrupts, arm7, timers),
      arm7(bus, timers),
      timers(interrupts, ppu) {
    LINFO("powering on...");
}

void GBA::Run() {
    arm7.Step(false);
}
