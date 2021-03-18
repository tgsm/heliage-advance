#pragma once

#include <deque>
#include <functional>
#include "types.h"

class Bus;

class PPU {
public:
    PPU(Bus& bus);

    void AdvanceCycles(u8 cycles);
    void Tick();

    void SetDISPCNT(u16 value) { dispcnt.raw = value; }

    u16 GetDISPSTAT() const { return dispstat.raw; }

    u16 GetVCOUNT() const { return vcount; }

    template <UnsignedIntegerMax32 T>
    T ReadVRAM(u32 addr) const {
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
            vram[addr] = value;
        }

        if constexpr (std::is_same_v<T, u16>) {
            addr &= ~0b1;
            vram[addr] = value & 0xFF;
            vram[addr + 1] = value >> 8;
        }

        if constexpr (std::is_same_v<T, u32>) {
            addr &= ~0b11;
            vram.at(addr) = value & 0xFF;
            vram.at(addr + 1) = value >> 8;
            vram.at(addr + 2) = value >> 16;
            vram.at(addr + 3) = value >> 24;
        }
    }

    void WritePRAM8(u32 addr, u8 value) { pram[addr] = value; }

    u16 ReadPRAM(u32 addr) const;

private:
    std::array<u8, 0x18000> vram;
    std::array<u8, 0x400> pram; // Palette RAM
    std::array<u16, 240 * 160> framebuffer;

    Bus& bus;

    u64 vcycles = 0;

    using SchedulerEntry = std::pair<u64, std::function<void()>>;
    std::deque<SchedulerEntry> scheduler_entries;

    void ScheduleNewFunction(u64 cycles_from_now, const std::function<void()>& function);

    void StartNewScanline();
    void StartHBlank();
    void EndHBlank();
    void StartVBlankLine();
    void RenderScanline();

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

    // Scanline counter, much like LY from the gameboy
    u8 vcount = 0;
};
