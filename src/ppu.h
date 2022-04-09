#pragma once

#include <deque>
#include <functional>
#include "common/bits.h"
#include "common/types.h"

constexpr u32 GBA_SCREEN_WIDTH = 240;
constexpr u32 GBA_SCREEN_HEIGHT = 160;

class Bus;
class Interrupts;

class PPU {
public:
    PPU(Bus& bus_, Interrupts& interrupts_);

    void AdvanceCycles(u8 cycles);
    void Tick();

    u16 GetDISPCNT() const { return dispcnt.raw; }
    void SetDISPCNT(u16 value) { dispcnt.raw = value; }

    [[nodiscard]] u16 GetDISPSTAT() const { return dispstat.raw; }
    void SetDISPSTAT(const u16 value) {
        dispstat.raw = (value & ~0x7) | (dispstat.raw & 0x7);
    }

    [[nodiscard]] u16 GetVCOUNT() const { return vcount; }

    template <u8 bg_no>
    [[nodiscard]] u16 GetBGCNT() const {
        static_assert(bg_no < 4);
        return bgcnts[bg_no].raw;
    }

    template <u8 bg_no>
    void SetBGCNT(const u16 value) {
        static_assert(bg_no < 4);
        bgcnts[bg_no].raw = value;
    }

    template <UnsignedIntegerMax32 T>
    [[nodiscard]] T ReadVRAM(u32 addr) const {
        if constexpr (std::is_same_v<T, u8>) {
            return vram.at(addr);
        }

        if constexpr (std::is_same_v<T, u16>) {
            addr &= ~0b1;
            return (vram.at(addr) |
                   (vram.at(addr + 1) << 8));
        }

        if constexpr (std::is_same_v<T, u32>) {
            addr &= ~0b11;
            return (vram.at(addr) |
                   (vram.at(addr + 1) << 8) |
                   (vram.at(addr + 2) << 16) |
                   (vram.at(addr + 3) << 24));
        }
    }

    template <UnsignedIntegerMax32 T>
    void WriteVRAM(u32 addr, T value) {
        if constexpr (std::is_same_v<T, u8>) {
            vram.at(addr) = value;
        }

        if constexpr (std::is_same_v<T, u16>) {
            addr &= ~0b1;
            vram.at(addr + 0) = Common::GetBitRange<7, 0>(value);
            vram.at(addr + 1) = Common::GetBitRange<15, 8>(value);
        }

        if constexpr (std::is_same_v<T, u32>) {
            addr &= ~0b11;
            vram.at(addr + 0) = Common::GetBitRange<7, 0>(value);
            vram.at(addr + 1) = Common::GetBitRange<15, 8>(value);
            vram.at(addr + 2) = Common::GetBitRange<23, 16>(value);
            vram.at(addr + 3) = Common::GetBitRange<31, 24>(value);
        }
    }

    template <UnsignedIntegerMax32 T>
    [[nodiscard]] T ReadPRAM(u32 addr) const {
        if constexpr (std::is_same_v<T, u8>) {
            return pram.at(addr);
        }

        if constexpr (std::is_same_v<T, u16>) {
            addr &= ~0b1;
            return (pram.at(addr) |
                   (pram.at(addr + 1) << 8));
        }

        if constexpr (std::is_same_v<T, u32>) {
            addr &= ~0b11;
            return (pram.at(addr) |
                   (pram.at(addr + 1) << 8) |
                   (pram.at(addr + 2) << 16) |
                   (pram.at(addr + 3) << 24));
        }
    }

    template <UnsignedIntegerMax32 T>
    void WritePRAM(u32 addr, T value) {
        if constexpr (std::is_same_v<T, u8>) {
            pram.at(addr) = value;
        }

        if constexpr (std::is_same_v<T, u16>) {
            addr &= ~0b1;
            pram.at(addr + 0) = Common::GetBitRange<7, 0>(value);
            pram.at(addr + 1) = Common::GetBitRange<15, 8>(value);
        }

        if constexpr (std::is_same_v<T, u32>) {
            addr &= ~0b11;
            pram.at(addr + 0) = Common::GetBitRange<7, 0>(value);
            pram.at(addr + 1) = Common::GetBitRange<15, 8>(value);
            pram.at(addr + 2) = Common::GetBitRange<23, 16>(value);
            pram.at(addr + 3) = Common::GetBitRange<31, 24>(value);
        }
    }

    template <UnsignedIntegerMax32 T>
    [[nodiscard]] T ReadOAM(u32 addr) const {
        if constexpr (std::is_same_v<T, u8>) {
            return oam.at(addr);
        }

        if constexpr (std::is_same_v<T, u16>) {
            addr &= ~0b1;
            return (oam.at(addr) |
                   (oam.at(addr + 1) << 8));
        }

        if constexpr (std::is_same_v<T, u32>) {
            addr &= ~0b11;
            return (oam.at(addr) |
                   (oam.at(addr + 1) << 8) |
                   (oam.at(addr + 2) << 16) |
                   (oam.at(addr + 3) << 24));
        }
    }

    template <UnsignedIntegerMax32 T>
    void WriteOAM(u32 addr, const T value) {
        if constexpr (std::is_same_v<T, u8>) {
            oam.at(addr) = value;
        }

        if constexpr (std::is_same_v<T, u16>) {
            addr &= ~0b1;
            oam.at(addr + 0) = Common::GetBitRange<7, 0>(value);
            oam.at(addr + 1) = Common::GetBitRange<15, 8>(value);
        }

        if constexpr (std::is_same_v<T, u32>) {
            addr &= ~0b11;
            oam.at(addr + 0) = Common::GetBitRange<7, 0>(value);
            oam.at(addr + 1) = Common::GetBitRange<15, 8>(value);
            oam.at(addr + 2) = Common::GetBitRange<23, 16>(value);
            oam.at(addr + 3) = Common::GetBitRange<31, 24>(value);
        }
    }

private:
    std::array<u8, 0x18000> vram {};
    std::array<u8, 0x400> pram {};
    std::array<u8, 0x400> oam {};
    std::array<u16, GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT> framebuffer {};

    Bus& bus;
    Interrupts& interrupts;

    u64 vcycles = 0;

    using SchedulerEntry = std::pair<u64, std::function<void()>>;
    std::deque<SchedulerEntry> scheduler_entries;

    void ScheduleNewFunction(u64 cycles_from_now, const std::function<void()>& function);

    void StartNewScanline();
    void StartHBlank();
    void EndHBlank();
    void StartVBlankLine();

    void RenderScanline();

    template <u8 bg_no>
    void RenderTiledBGScanline();

    union {
        u16 raw = 0x0000;
        struct {
            u8 bg_mode : 3;
            bool cgb_mode : 1;
            bool display_frame_select : 1;
            bool hblank_interval_free : 1;
            bool obj_character_vram_mapping : 1;
            bool forced_blank : 1;
            bool screen_display0 : 1;
            bool screen_display1 : 1;
            bool screen_display2 : 1;
            bool screen_display3 : 1;
            bool screen_display_obj : 1;
            bool window0_display : 1;
            bool window1_display : 1;
            bool obj_window_display : 1;
        } flags;
    } dispcnt;

    union {
        u16 raw = 0x0000;
        struct {
            bool vblank : 1;
            bool hblank : 1;
            bool vcounter : 1;
            bool vblank_irq : 1;
            bool hblank_irq : 1;
            bool vcounter_irq : 1;
            u8 : 2; // used in NDS/DSi, but not the GBA
            u8 vcount_setting : 8; // aka LYC
        } flags;
    } dispstat;

    union BGCNT {
        u16 raw;
        struct {
            u8 bg_priority : 2;
            u8 character_base_block : 2;
            u8 : 2;
            bool mosaic : 1;
            bool use_256_colors : 1;
            u8 screen_base_block : 5;
            bool display_area_overflow : 1;
            u8 screen_size : 2;
        } flags;
    };

    std::array<BGCNT, 4> bgcnts {};

    // Scanline counter, much like LY from the gameboy
    u8 vcount = 0;

    using Tile = std::array<std::array<u8, 8>, 8>;

    template <u8 bg_no>
    Tile ConstructTile(u16 tile_index);
};
