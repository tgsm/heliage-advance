#pragma once

#include <array>
#include "cartridge.h"
#include "ppu.h"
#include "types.h"

class MMU {
public:
    MMU(Cartridge& cartridge, PPU& ppu);

    u8 Read8(u32 addr);
    void Write8(u32 addr, u8 value);
    u16 Read16(u32 addr);
    void Write16(u32 addr, u16 value);
    u32 Read32(u32 addr);
    void Write32(u32 addr, u32 value);
private:
    Cartridge& cartridge;
    PPU& ppu;

    std::array<u8, 0x8000> wram;
};
