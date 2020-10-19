#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>
#include "frontend/frontend.h"
#include "logging.h"
#include "ppu.h"

PPU::PPU() {
    vram.fill(0x00);
    pram.fill(0x00);
    framebuffer.fill(0x00);

    StartNewScanline();
}

void PPU::AdvanceCycles(u8 cycles) {
    for (; cycles > 0; cycles--) {
        vcycles++;
        Tick();
    }
}

void PPU::Tick() {
    ASSERT(!scheduler_entries.empty());

    // Check if it's time to run our scheduled function
    if (scheduler_entries.front().first == vcycles) {
        // Run our scheduled function
        scheduler_entries.front().second();

        // Remove our function from the queue
        scheduler_entries.erase(scheduler_entries.begin());
    }
}

void PPU::ScheduleNewFunction(u64 cycles_from_now, const std::function<void()>& function) {
    scheduler_entries.push_back(std::make_pair(vcycles + cycles_from_now, function));
}

void PPU::StartNewScanline() {
    ScheduleNewFunction(960, std::bind(&PPU::StartHBlank, this));
}

void PPU::StartHBlank() {
    dispstat.flags.hblank = true;
    ScheduleNewFunction(272, std::bind(&PPU::EndHBlank, this));
}

void PPU::EndHBlank() {
    dispstat.flags.hblank = false;
    vcount++;

    if (vcount >= 228) {
        vcount = 0;
        dispstat.flags.vblank = false;
        StartNewScanline();
    } else if (vcount == 160) {
        dispstat.flags.vblank = true;
        StartVBlankLine();
        Render();

        HandleFrontendEvents();
    } else if (vcount >= 160) {
        StartVBlankLine();
    } else {
        StartNewScanline();
    }
}

void PPU::StartVBlankLine() {
    StartNewScanline();
}

void PPU::Render() {
    switch (dispcnt.flags.bg_mode) {
        case 4:
            for (std::size_t i = 0; i < framebuffer.size(); i++) {
                u8 palette_index = vram.at(i) * 2;
                u16 color = ReadPRAM(palette_index);
                framebuffer[i] = color;
            }

            break;
        default:
            LERROR("PPU: unimplemented BG mode %u", dispcnt.flags.bg_mode);
            break;
    }

    DisplayFramebuffer(framebuffer);
    framebuffer.fill(0x0000);
}

u16 PPU::ReadPRAM(u32 addr) const {
    return (pram.at(addr + 1) << 8) | pram.at(addr);
}
