#include "bus.h"
#include "frontend/frontend.h"
#include "common/logging.h"
#include "ppu.h"

constexpr u32 TILE_WIDTH = 8;
constexpr u32 TILE_HEIGHT = 8;

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
        case 1:
            // Render the backdrop color before anything else.
            for (std::size_t i = 0; i < GBA_SCREEN_WIDTH; i++) {
                framebuffer.at(vcount * GBA_SCREEN_WIDTH + i) = ReadPRAM<u16>(0);
            }

            RenderTiledBGScanlineByPriority(3);
            RenderTiledBGScanlineByPriority(2);
            RenderTiledBGScanlineByPriority(1);
            RenderTiledBGScanlineByPriority(0);
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

bool PPU::IsBGScreenDisplayEnabled(const std::size_t bg_no) const {
    switch (bg_no) {
        case 0:
            return dispcnt.flags.screen_display0;
        case 1:
            return dispcnt.flags.screen_display1;
        case 2:
            return dispcnt.flags.screen_display2;
        case 3:
            return dispcnt.flags.screen_display3;
        default:
            UNREACHABLE();
    }
}

void PPU::RenderTiledBGScanlineByPriority(const std::size_t priority) {
    for (std::size_t bg = 0; bg < bgs.size(); bg++) {
        if (!IsBGScreenDisplayEnabled(bg)) {
            continue;
        }

        if (bgs[bg].control.flags.bg_priority == priority) {
            RenderTiledBGScanline(bg);
        }
    }
}

void PPU::RenderTiledBGScanline(const std::size_t bg_no) {
    const BG& bg = bgs.at(bg_no);

    const u32 tile_map_base = bg.control.flags.screen_base_block * 0x800;

    for (u16 screen_x = 0; screen_x < GBA_SCREEN_WIDTH; screen_x++) {
        const u16 map_x = (screen_x + bg.x_offset) % (Common::IsBitSet<0>(bg.control.flags.screen_size) ? 512 : 256);
        const u16 map_y = (vcount + bg.y_offset) % (Common::IsBitSet<1>(bg.control.flags.screen_size) ? 512 : 256);

        auto tile_address = tile_map_base + ((map_x / TILE_WIDTH) + (map_y / TILE_HEIGHT * 32)) * sizeof(u16);
        if (map_x >= 256) {
            tile_address = (tile_map_base + (((map_x - 256) / TILE_WIDTH) + (map_y / TILE_HEIGHT * 32)) * sizeof(u16)) + 0x800;
        }
        // TODO: map_y >= 256

        u16 tile_entry = ReadVRAM<u16>(tile_address);
        const u16 tile_index = Common::GetBitRange<0, 9>(tile_entry);
        const Tile& tile = ConstructTile(bg, tile_index);

        const std::size_t tile_x = map_x % TILE_WIDTH;
        const std::size_t tile_y = map_y % TILE_HEIGHT;

        const bool vertical_flip = Common::IsBitSet<11>(tile_entry);
        const std::size_t real_tile_y = vertical_flip ? ((TILE_HEIGHT - 1) - tile_y) : tile_y;

        const bool horizontal_flip = Common::IsBitSet<10>(tile_entry);
        const std::size_t real_tile_x = horizontal_flip ? ((TILE_WIDTH - 1) - tile_x) : tile_x;

        // Color 0 is used for transparency.
        if (tile[real_tile_y][real_tile_x] == 0) {
            continue;
        }

        const u16 palette_index = Common::GetBitRange<12, 15>(tile_entry);
        const auto pram_addr = ((palette_index << 4) | tile[real_tile_y][real_tile_x]) * sizeof(u16);
        const std::size_t framebuffer_pixel_position = (vcount * GBA_SCREEN_WIDTH) + screen_x;
        framebuffer.at(framebuffer_pixel_position) = ReadPRAM<u16>(pram_addr);
    }
}

PPU::Tile PPU::ConstructTile(const BG& bg, const u16 tile_index) {
    const u32 tile_data_base = bg.control.flags.character_base_block * 0x4000;
    const std::size_t tile_size = bg.control.flags.use_256_colors ? 64 : 32;
    const u32 tile_base = tile_data_base + (tile_index * tile_size);

    Tile tile {};

    std::vector<u8> tile_data(tile_size);
    std::copy(vram.data() + tile_base, vram.data() + tile_base + tile_size, tile_data.data());

    if (bg.control.flags.use_256_colors) {
        for (std::size_t y = 0; y < TILE_HEIGHT; y++) {
            for (std::size_t x = 0; x < TILE_WIDTH; x++) {
                tile[y][x] = tile_data.at((y * TILE_WIDTH) + x);
            }
        }
    } else {
        for (std::size_t y = 0; y < TILE_HEIGHT; y++) {
            for (std::size_t x = 0; x < TILE_WIDTH; x += 2) {
                const auto tile_data_index = (y * (TILE_WIDTH / 2)) + (x / 2);
                tile[y][x + 0] = tile_data.at(tile_data_index) & 0xF;
                tile[y][x + 1] = tile_data.at(tile_data_index) >> 4;
            }
        }
    }

    return tile;
}
