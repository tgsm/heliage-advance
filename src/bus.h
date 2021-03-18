#pragma once

#include <array>
#include "bios.h"
#include "cartridge.h"
#include "keypad.h"
#include "ppu.h"
#include "types.h"

class Bus {
public:
    Bus(BIOS& bios_, Cartridge& cartridge_, Keypad& keypad_, PPU& ppu_);

    [[nodiscard]] u8 Read8(u32 addr);
    void Write8(u32 addr, u8 value);
    [[nodiscard]] u16 Read16(u32 addr);
    void Write16(u32 addr, u16 value);
    [[nodiscard]] u32 Read32(u32 addr);
    void Write32(u32 addr, u32 value);

    [[nodiscard]] Keypad& GetKeypad() { return keypad; }
    [[nodiscard]] const Keypad& GetKeypad() const { return keypad; }
private:
    BIOS& bios;
    Cartridge& cartridge;
    Keypad& keypad;
    PPU& ppu;

    std::array<u8, 0x40000> wram_onboard {};
    std::array<u8, 0x8000> wram_onchip {};
};
