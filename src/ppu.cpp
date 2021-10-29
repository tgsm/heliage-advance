#include "bus.h"
#include "frontend/frontend.h"
#include "logging.h"
#include "ppu.h"

PPU::PPU(Bus& bus_)
    : bus(bus_) {
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

    const auto& next_entry = scheduler_entries.front();

    // Check if it's time to run our scheduled function
    if (next_entry.first == vcycles) {
        // Run our scheduled function
        next_entry.second();

        // Remove our function from the queue
        scheduler_entries.pop_front();
    }
}

void PPU::ScheduleNewFunction(u64 cycles_from_now, const std::function<void()>& function) {
    scheduler_entries.push_back(std::make_pair(vcycles + cycles_from_now, function));
}

void PPU::StartNewScanline() {
    ScheduleNewFunction(960, [this]() { StartHBlank(); });
}

void PPU::StartHBlank() {
    dispstat.flags.hblank = true;
    ScheduleNewFunction(272, [this]() { EndHBlank(); });
}

void PPU::EndHBlank() {
    dispstat.flags.hblank = false;
    RenderScanline();
    vcount++;

    if (vcount >= 228) {
        vcount = 0;
        dispstat.flags.vblank = false;
        StartNewScanline();
    } else if (vcount == 160) {
        dispstat.flags.vblank = true;
        StartVBlankLine();

        DisplayFramebuffer(framebuffer);
        framebuffer.fill(0x0000);

        HandleFrontendEvents(&bus.GetKeypad());
    } else if (vcount >= 160) {
        StartVBlankLine();
    } else {
        StartNewScanline();
    }
}

void PPU::StartVBlankLine() {
    StartNewScanline();
}

void PPU::RenderScanline() {
    if (vcount >= 160) {
        return;
    }

    switch (dispcnt.flags.bg_mode) {
        case 3:
            for (std::size_t i = 0; i < 240; i++) {
                u16 color = ReadVRAM<u16>((vcount * 240 * sizeof(u16)) + i * sizeof(u16));
                framebuffer.at(vcount * 240 + i) = color;
            }

            break;
        case 4:
            for (std::size_t i = 0; i < 240; i++) {
                u8 palette_index = vram.at((vcount * 240) + i) * sizeof(u16);
                u16 color = ReadPRAM(palette_index);
                framebuffer.at((vcount * 240) + i) = color;
            }

            break;
        default:
            LERROR("PPU: unimplemented BG mode {}", dispcnt.flags.bg_mode);
            break;
    }
}

u16 PPU::ReadPRAM(u32 addr) const {
    return (pram.at(addr + 1) << 8) | pram.at(addr);
}
