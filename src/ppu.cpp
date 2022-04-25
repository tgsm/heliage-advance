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

void PPU::AdvanceCycles(u16 cycles) {
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
    } else if (vcount == GBA_SCREEN_HEIGHT) {
        if (dispstat.flags.vblank_irq) {
            interrupts.RequestInterrupt(Interrupts::Bits::VBlank);
        }

        dispstat.flags.vblank = true;
        StartVBlankLine();

        DisplayFramebuffer(framebuffer);
        framebuffer = {};

        HandleFrontendEvents(&bus.GetKeypad());
    } else if (vcount > GBA_SCREEN_HEIGHT) {
        StartVBlankLine();
    } else {
        StartNewScanline();
    }
}

void PPU::StartVBlankLine() {
    StartNewScanline();
}

void PPU::RenderScanline() {
    if (vcount >= GBA_SCREEN_HEIGHT) {
        return;
    }

    switch (dispcnt.flags.bg_mode) {
        case 0:
        case 1:
        case 2:
            // Render the backdrop color before anything else.
            for (std::size_t i = 0; i < GBA_SCREEN_WIDTH; i++) {
                framebuffer.at(vcount * GBA_SCREEN_WIDTH + i) = ReadPRAM<u16>(0);
            }

            RenderTiledBGScanlineByPriority(3);
            RenderTiledSpriteScanlineByPriority(3);
            RenderTiledBGScanlineByPriority(2);
            RenderTiledSpriteScanlineByPriority(2);
            RenderTiledBGScanlineByPriority(1);
            RenderTiledSpriteScanlineByPriority(1);
            RenderTiledBGScanlineByPriority(0);
            RenderTiledSpriteScanlineByPriority(0);

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

            RenderTiledSpriteScanlineByPriority(3);
            RenderTiledSpriteScanlineByPriority(2);
            RenderTiledSpriteScanlineByPriority(1);
            RenderTiledSpriteScanlineByPriority(0);

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
        const Tile& tile = ConstructBGTile(bg, tile_index);

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

        u16 palette_index = Common::GetBitRange<12, 15>(tile_entry);
        if (bg.control.flags.use_256_colors) {
            palette_index = 0;
        }
        const auto pram_addr = ((palette_index << 4) | tile[real_tile_y][real_tile_x]) * sizeof(u16);
        const std::size_t framebuffer_pixel_position = (vcount * GBA_SCREEN_WIDTH) + screen_x;
        framebuffer.at(framebuffer_pixel_position) = ReadPRAM<u16>(pram_addr);
    }
}

void PPU::RenderTiledSpriteScanlineByPriority(const std::size_t priority) {
    if (!dispcnt.flags.screen_display_obj) {
        return;
    }

    for (int sprite_no = 127; sprite_no >= 0; sprite_no--) {
        const Sprite sprite = {
            .attributes = {
                ReadOAM<u16>((sprite_no * 8) + 0),
                ReadOAM<u16>((sprite_no * 8) + 2),
                ReadOAM<u16>((sprite_no * 8) + 4),
            },
        };

        // Skip to the next sprite if the rotation/scaling flag is disabled and the OBJ disabled flag is enabled.
        if (!Common::IsBitSet<8>(sprite.attributes[0])) {
            if (Common::IsBitSet<9>(sprite.attributes[0])) {
                continue;
            }
        }

        const u16 sprite_priority = Common::GetBitRange<10, 11>(sprite.attributes[2]);
        if (priority == sprite_priority) {
            RenderTiledSpriteScanline(sprite);
        }
    }
}

std::size_t PPU::DetermineTileInSprite(const Sprite& sprite, const u16 screen_x, const u16 screen_y, const u16 sprite_x, const u16 sprite_y, const u16 width, const u16 height) {
    const auto width_in_tiles = width / TILE_WIDTH;
    const auto height_in_tiles = height / TILE_HEIGHT;

    auto sprite_map_x = (screen_x - sprite_x) / TILE_WIDTH;
    ASSERT(sprite_map_x < width_in_tiles);
    auto sprite_map_y = (screen_y - sprite_y) / TILE_HEIGHT;
    ASSERT(sprite_map_y < height_in_tiles);

    if (width_in_tiles == 1 && height_in_tiles == 1) {
        return 0;
    }

    const bool horizontal_flip = Common::IsBitSet<12>(sprite.attributes[1]);
    if (horizontal_flip && width_in_tiles != 1) {
        sprite_map_x = width_in_tiles - sprite_map_x - 1;
    }

    const bool vertical_flip = Common::IsBitSet<13>(sprite.attributes[1]);
    if (vertical_flip && height_in_tiles != 1) {
        sprite_map_y = height_in_tiles - sprite_map_y - 1;
    }

    return sprite_map_y * width_in_tiles + sprite_map_x;
}

void PPU::RenderTiledSpriteScanline(const Sprite& sprite) {
    enum class SpriteShape {
        Square,
        Horizontal,
        Vertical,
        Forbidden,
    };

    const std::unsigned_integral auto x = Common::GetBitRange<0, 8>(sprite.attributes[1]);
    const std::unsigned_integral auto y = Common::GetBitRange<0, 7>(sprite.attributes[0]);

    if (x >= GBA_SCREEN_WIDTH || y >= GBA_SCREEN_HEIGHT) {
        return;
    }

    const auto shape = SpriteShape(Common::GetBitRange<14, 15>(sprite.attributes[0]));
    const std::unsigned_integral auto size = Common::GetBitRange<14, 15>(sprite.attributes[1]);

    u8 width = 0;
    u8 height = 0;

    switch (shape) {
        case SpriteShape::Square:
            switch (size) {
                case 0: width = 8; height = 8; break;
                case 1: width = 16; height = 16; break;
                case 2: width = 32; height = 32; break;
                case 3: width = 64; height = 64; break;
                default: UNREACHABLE();
            }
            break;
        case SpriteShape::Horizontal:
            switch (size) {
                case 0: width = 16; height = 8; break;
                case 1: width = 32; height = 8; break;
                case 2: width = 32; height = 16; break;
                case 3: width = 64; height = 32; break;
                default: UNREACHABLE();
            }
            break;
        case SpriteShape::Vertical:
            switch (size) {
                case 0: width = 8; height = 16; break;
                case 1: width = 8; height = 32; break;
                case 2: width = 16; height = 32; break;
                case 3: width = 32; height = 64; break;
                default: UNREACHABLE();
            }
            break;
        case SpriteShape::Forbidden:
        default:
            UNIMPLEMENTED_MSG("Unimplemented sprite shape {}", shape);
    }

    if (vcount < y || vcount >= y + height) {
        return;
    }

    for (std::size_t screen_x = 0; screen_x < GBA_SCREEN_WIDTH; screen_x++) {
        if (screen_x < x || screen_x >= x + width) {
            continue;
        }

        const bool use_256_colors = Common::IsBitSet<13>(sprite.attributes[0]);

        std::unsigned_integral auto tile_index = Common::GetBitRange<0, 9>(sprite.attributes[2]);
        if (use_256_colors) {
            tile_index /= 2;
        }
        const std::size_t which_tile = DetermineTileInSprite(sprite, screen_x, vcount, x, y, width, height);
        const Tile& tile = ConstructSpriteTile(sprite, tile_index + which_tile);

        const auto tile_x = (screen_x - x) % TILE_WIDTH;
        const auto tile_y = (vcount - y) % TILE_HEIGHT;

        const auto real_tile_x = Common::IsBitSet<12>(sprite.attributes[1]) ? (7 - tile_x) : tile_x;
        const auto real_tile_y = Common::IsBitSet<13>(sprite.attributes[1]) ? (7 - tile_y) : tile_y;

        if (tile[real_tile_y][real_tile_x] == 0) {
            continue;
        }

        u16 palette_index = 0;
        if (!use_256_colors) {
            palette_index = Common::GetBitRange<12, 15>(sprite.attributes[2]);
        }
        const auto pram_addr = ((palette_index << 4) | tile[real_tile_y][real_tile_x]) * sizeof(u16);
        framebuffer.at((vcount * GBA_SCREEN_WIDTH) + screen_x) = ReadPRAM<u16>(0x200 + pram_addr);
    }
}

PPU::Tile PPU::ConstructBGTile(const BG& bg, const u16 tile_index) {
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

PPU::Tile PPU::ConstructSpriteTile(const Sprite& sprite, const u16 tile_index) {
    const u32 tile_data_base = 0x10000;

    const bool use_256_colors = Common::IsBitSet<13>(sprite.attributes[0]);
    const std::size_t tile_size = use_256_colors ? 64 : 32;
    const u32 tile_base = tile_data_base + (tile_index * tile_size);

    Tile tile {};

    std::vector<u8> tile_data(tile_size);
    std::copy(vram.data() + tile_base, vram.data() + tile_base + tile_size, tile_data.data());

    if (use_256_colors) {
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
