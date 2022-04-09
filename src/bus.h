#pragma once

#include <array>
#include "common/types.h"
#include "bios.h"
#include "cartridge.h"
#include "interrupts.h"
#include "keypad.h"
#include "ppu.h"

class ARM7;

class Bus {
public:
    Bus(BIOS& bios_, Cartridge& cartridge_, Keypad& keypad_, PPU& ppu_, Interrupts& interrupts_, ARM7& arm7_);

    [[nodiscard]] u8 Read8(u32 addr);
    void Write8(u32 addr, u8 value);
    [[nodiscard]] u16 Read16(u32 addr);
    void Write16(u32 addr, u16 value);
    [[nodiscard]] u32 Read32(u32 addr);
    void Write32(u32 addr, u32 value);

    [[nodiscard]] Keypad& GetKeypad() { return keypad; }
    [[nodiscard]] const Keypad& GetKeypad() const { return keypad; }

    [[nodiscard]] Interrupts& GetInterrupts() { return interrupts; }
    [[nodiscard]] const Interrupts& GetInterrupts() const { return interrupts; }
    
private:
    BIOS& bios;
    Cartridge& cartridge;
    Keypad& keypad;
    PPU& ppu;
    Interrupts& interrupts;
    ARM7& arm7;

    std::array<u8, 0x40000> wram_onboard {};
    std::array<u8, 0x8000> wram_onchip {};

    union DMACNT {
        u16 raw;
        struct {
            u16 : 5;
            u16 dest_addr_control : 2;
            u16 src_addr_control : 2;
            bool repeat : 1;
            bool transfer_type_is_32bit : 1;
            bool gamepak_dma3_drq : 1;
            u16 start_timing : 2;
            bool irq_at_end_of_word_count : 1;
            bool enable : 1;
        } flags;
    };

    struct DMAChannel {
        u32 source_address;
        u32 destination_address;
        u16 word_count;
        DMACNT control;
    };

    std::array<DMAChannel, 4> dma_channels {};

    template <u8 dma_channel_no>
    void SetDMAControl(u16 value);

    template <u8 dma_channel_no>
    void RunDMATransfer();

    bool post_flg = false;
};
