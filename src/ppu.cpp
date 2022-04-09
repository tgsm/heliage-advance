#include "bus.h"
#include "frontend/frontend.h"
#include "common/logging.h"
#include "ppu.h"

constexpr u32 TILE_WIDTH = 8;
constexpr u32 TILE_HEIGHT = 8;
constexpr u32 TILE_SIZE_IN_BYTES = 32;

PPU::PPU(Bus& bus_, Interrupts& interrupts_)
    : bus(bus_), interrupts(interrupts_) {
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
    if (dispstat.flags.hblank_irq) {
        interrupts.RequestInterrupt(Interrupts::Bits::HBlank);
    }
    ScheduleNewFunction(272, [this]() { EndHBlank(); });
}

void PPU::EndHBlank() {
    dispstat.flags.hblank = false;
    RenderScanline();
    vcount++;

    if (vcount == dispstat.flags.vcount_setting && dispstat.flags.vcounter_irq) {
        interrupts.RequestInterrupt(Interrupts::Bits::VCounterMatch);
    }

    if (vcount >= 228) {
        vcount = 0;
        dispstat.flags.vblank = false;
        StartNewScanline();
    } else if (vcount == 160) {
        if (dispstat.flags.vblank_irq) {
            interrupts.RequestInterrupt(Interrupts::Bits::VBlank);
        }

        dispstat.flags.vblank = true;
        StartVBlankLine();

        DisplayFramebuffer(framebuffer);
        framebuffer = {};

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
        case 0:
            if (dispcnt.flags.screen_display0) {
                RenderTiledBGScanline<0>();
            }

            if (dispcnt.flags.screen_display1) {
                RenderTiledBGScanline<1>();
            }

            if (dispcnt.flags.screen_display2) {
                RenderTiledBGScanline<2>();
            }

            if (dispcnt.flags.screen_display3) {
                RenderTiledBGScanline<3>();
            }

            break;
        case 3:
            for (std::size_t i = 0; i < GBA_SCREEN_WIDTH; i++) {
                u16 color = ReadVRAM<u16>((vcount * GBA_SCREEN_WIDTH * sizeof(u16)) + i * sizeof(u16));
                framebuffer.at(vcount * GBA_SCREEN_WIDTH + i) = color;
            }

            break;
        case 4:
            for (std::size_t i = 0; i < GBA_SCREEN_WIDTH; i++) {
                std::size_t vram_address = (vcount * GBA_SCREEN_WIDTH) + i;
                if (dispcnt.flags.display_frame_select) {
                    vram_address += 0xA000;
                }

                const u16 palette_index = vram.at(vram_address) * sizeof(u16);
                const u16 color = ReadPRAM<u16>(palette_index);
                framebuffer.at((vcount * GBA_SCREEN_WIDTH) + i) = color;
            }

            break;
        default:
            LERROR("PPU: unimplemented BG mode {}", dispcnt.flags.bg_mode);
            break;
    }
}

template <u8 bg_no>
void PPU::RenderTiledBGScanline() {
    static_assert(bg_no < 4);

    const u32 tile_map_base = bgcnts[bg_no].flags.screen_base_block * 0x800;

    for (std::size_t i = 0; i < GBA_SCREEN_WIDTH / TILE_WIDTH; i++) {
        const u16 tile_index = ReadVRAM<u16>(tile_map_base + ((vcount / TILE_HEIGHT) * 64) + (i * sizeof(u16))) & 0x7FF;
        const Tile& tile = ConstructTile<bg_no>(tile_index);

        for (std::size_t tile_x = 0; tile_x < TILE_WIDTH; tile_x++) {
            const std::size_t framebuffer_pixel_position = (vcount * GBA_SCREEN_WIDTH) + (i * TILE_WIDTH) + tile_x;
            const std::size_t tile_y = vcount % 8;
            framebuffer.at(framebuffer_pixel_position) = ReadPRAM<u16>(tile[tile_y][tile_x] * sizeof(u16));
        }
    }
}

template <u8 bg_no>
PPU::Tile PPU::ConstructTile(const u16 tile_index) {
    static_assert(bg_no < 4);

    const u32 tile_data_base = bgcnts[bg_no].flags.character_base_block * 0x4000;
    const u32 tile_base = tile_data_base + (tile_index * TILE_SIZE_IN_BYTES);

    Tile tile;

    std::array<u8, TILE_SIZE_IN_BYTES> tile_data;
    std::copy(vram.data() + tile_base, vram.data() + tile_base + TILE_SIZE_IN_BYTES, tile_data.data());

    for (std::size_t y = 0; y < 8; y++) {
        for (std::size_t x = 0; x < 8; x += 2) {
            tile[y][x + 0] = tile_data.at((y * 4) + (x / 2)) & 0xF;
            tile[y][x + 1] = tile_data.at((y * 4) + (x / 2)) >> 4;
        }
    }

    return tile;
}
