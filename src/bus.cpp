#include "bus.h"
#include "common/logging.h"

Bus::Bus(BIOS& bios_, Cartridge& cartridge_, Keypad& keypad_, PPU& ppu_)
    : bios(bios_), cartridge(cartridge_), keypad(keypad_), ppu(ppu_) {
}

u8 Bus::Read8(u32 addr) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x0:
            return bios.Read<u8>(masked_addr & 0x3FFF);

        case 0x2: {
            u8 value = wram_onboard.at(masked_addr & 0x3FFFF);
            LDEBUG("read8 0x{:02X} from 0x{:08X} (WRAM onboard)", value, masked_addr);
            return value;
        }

        case 0x3: {
            u8 value = wram_onchip.at(masked_addr & 0x7FFF);
            LDEBUG("read8 0x{:02X} from 0x{:08X} (WRAM on-chip)", value, masked_addr);
            return value;
        }

        case 0x4:
            switch (masked_addr) {
                case 0x4000006:
                    return ppu.GetVCOUNT();
                default:
                    LERROR("unrecognized read8 from IO register 0x{:08X}", masked_addr);
                    return 0xFF;
            }

            return 0xFF;

        case 0x6: {
            u32 address = masked_addr & 0x1FFFF;
            if (address > 0x17FFF) {
                address -= 0x8000;
            }

            u8 value = ppu.ReadVRAM<u8>(address);
            LDEBUG("read8 0x{:02X} from 0x{:08X} (VRAM)", value, masked_addr);
            return value;
        }

        case 0x8:
            if ((masked_addr & 0x1FFFFFF) >= cartridge.GetSize()) {
                // TODO: open bus?
                return 0;
            }
            return cartridge.Read<u8>(masked_addr & 0x1FFFFFF);

        default:
            LERROR("unrecognized read8 from 0x{:08X}", addr);
            return 0xFF;
    }
}

void Bus::Write8(u32 addr, u8 value) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x2:
            LDEBUG("write8 0x{:02X} to 0x{:08X} (WRAM onboard)", value, masked_addr);
            wram_onboard[masked_addr & 0x3FFFF] = value;
            return;

        case 0x3:
            LDEBUG("write8 0x{:02X} to 0x{:08X} (WRAM on-chip)", value, masked_addr);
            wram_onchip[masked_addr & 0x7FFF] = value;
            return;

        case 0x6: {
            u32 address = masked_addr & 0x1FFFF;
            if (address > 0x17FFF) {
                address -= 0x8000;
            }

            LDEBUG("write8 0x{:02X} to 0x{:08X} (VRAM)", value, masked_addr);
            ppu.WriteVRAM<u8>(address, value);
            return;
        }

        default:
            LERROR("unrecognized write8 0x{:02X} to 0x{:08X}", value, masked_addr);
            return;
    }
}

u16 Bus::Read16(u32 addr) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x0:
            return bios.Read<u16>(masked_addr & 0x3FFF);

        case 0x2: {
            u16 value = 0;
            for (std::size_t i = 0; i < 2; i++) {
                // Mask off the last bit to keep halfword alignment.
                value |= ((wram_onboard.at(((masked_addr & ~0b1) & 0x3FFFF) + i)) & 0xFF) << (8 * i);
            }

            LDEBUG("read16 0x{:04X} from 0x{:08X} (WRAM onboard)", value, masked_addr);
            return value;
        }

        case 0x3: {
            u16 value = 0;
            for (std::size_t i = 0; i < 2; i++) {
                // Mask off the last bit to keep halfword alignment.
                value |= ((wram_onchip.at(((masked_addr & ~0b1) & 0x7FFF) + i)) & 0xFF) << (8 * i);
            }

            LDEBUG("read16 0x{:04X} from 0x{:08X} (WRAM on-chip)", value, masked_addr);
            return value;
        }

        case 0x4:
            switch (masked_addr) {
                case 0x4000000:
                    return ppu.GetDISPCNT();
                case 0x4000004:
                    return ppu.GetDISPSTAT();
                case 0x4000006:
                    return ppu.GetVCOUNT();
                case 0x4000130:
                    return keypad.GetState();
                default:
                    LERROR("unrecognized read16 from IO register 0x{:08X}", masked_addr);
                    return 0xFFFF;
            }

        case 0x6: {
            u32 address = masked_addr & 0x1FFFF;
            if (address > 0x17FFF) {
                address -= 0x8000;
            }

            u16 value = ppu.ReadVRAM<u16>(address);
            LDEBUG("read16 0x{:04X} from 0x{:08X} (VRAM)", value, masked_addr);
            return value;
        }

        case 0x8:
            if ((masked_addr & 0x1FFFFFF) >= cartridge.GetSize()) {
                // TODO: open bus?
                return 0;
            }
            return cartridge.Read<u16>(masked_addr & 0x1FFFFFF);

        default:
            LERROR("unrecognized read16 from 0x{:08X}", addr);
            return 0xFFFF;
    }
}

void Bus::Write16(u32 addr, u16 value) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x2:
            LDEBUG("write16 0x{:04X} to 0x{:08X} (WRAM onboard)", value, masked_addr);
            for (size_t i = 0; i < 2; i++) {
                // Mask off the last bit to keep halfword alignment.
                wram_onboard[((masked_addr & ~0b1) & 0x3FFFF) + i] = (value >> (8 * i)) & 0xFF;
            }
            return;

        case 0x3:
            LDEBUG("write16 0x{:04X} to 0x{:08X} (WRAM on-chip)", value, masked_addr);
            for (size_t i = 0; i < 2; i++) {
                // Mask off the last bit to keep halfword alignment.
                wram_onchip[((masked_addr & ~0b1) & 0x7FFF) + i] = (value >> (8 * i)) & 0xFF;
            }
            return;

        case 0x4:
            switch (masked_addr) {
                case 0x4000000:
                    ppu.SetDISPCNT(value);
                    return;
                case 0x4000004:
                    ppu.SetDISPSTAT(value);
                    return;
                case 0x4000008:
                    ppu.SetBGCNT<0>(value);
                    return;
                case 0x400000A:
                    ppu.SetBGCNT<1>(value);
                    return;
                case 0x400000C:
                    ppu.SetBGCNT<2>(value);
                    return;
                case 0x40000DC:
                    dma_channels[3].word_count = value;
                    return;
                case 0x40000DE:
                    SetDMAControl<3>(value);
                    return;
                default:
                    LERROR("unrecognized write16 0x{:04X} to IO register 0x{:08X}", value, masked_addr);
                    return;
            }

        case 0x5:
            LDEBUG("write16 0x{:04X} to 0x{:08X} (PRAM)", value, masked_addr);
            for (size_t i = 0; i < 2; i++) {
                // Mask off the last bit to keep halfword alignment.
                ppu.WritePRAM<u8>(((masked_addr & ~0b1) & 0x3FF) + i, (value >> (8 * i)) & 0xFF);
            }
            return;

        case 0x6: {
            u32 address = masked_addr & 0x1FFFF;
            if (address > 0x17FFF) {
                address -= 0x8000;
            }

            LDEBUG("write16 0x{:04X} to 0x{:08X} (VRAM)", value, masked_addr);
            ppu.WriteVRAM<u16>(address, value);
            return;
        }

        default:
            LERROR("unrecognized write16 0x{:04X} to 0x{:08X}", value, addr);
            return;
    }
}

u32 Bus::Read32(u32 addr) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x0:
            return bios.Read<u32>(masked_addr & 0x3FFF);

        case 0x2: {
            u32 value = 0;
            for (std::size_t i = 0; i < 4; i++) {
                // Mask off the last 2 bits to keep word alignment.
                value |= ((wram_onboard.at(((masked_addr & ~0b11) & 0x3FFFF) + i)) & 0xFF) << (8 * i);
            }

            LDEBUG("read32 0x{:08X} from 0x{:08X} (WRAM onboard)", value, masked_addr);
            return value;
        }

        case 0x3: {
            u32 value = 0;
            for (std::size_t i = 0; i < 4; i++) {
                // Mask off the last 2 bits to keep word alignment.
                value |= ((wram_onchip.at(((masked_addr & ~0b11) & 0x7FFF) + i)) & 0xFF) << (8 * i);
            }

            LDEBUG("read32 0x{:08X} from 0x{:08X} (WRAM on-chip)", value, masked_addr);
            return value;
        }

        case 0x4:
            switch (masked_addr) {
                case 0x4000004:
                    return ppu.GetDISPSTAT();
                case 0x4000130:
                    return keypad.GetState();
                default:
                    LERROR("unrecognized read32 from IO register 0x{:08X}", masked_addr);
                    return 0xFFFFFFFF;
            }

        case 0x6: {
            u32 address = masked_addr & 0x1FFFF;
            if (address > 0x17FFF) {
                address -= 0x8000;
            }

            u32 value = ppu.ReadVRAM<u32>(address);
            LDEBUG("read32 0x{:08X} from 0x{:08X} (VRAM)", value, masked_addr);
            return value;
        }

        case 0x8:
            if ((masked_addr & 0x1FFFFFF) >= cartridge.GetSize()) {
                // TODO: open bus?
                return 0;
            }
            return cartridge.Read<u32>(masked_addr & 0x1FFFFFF);

        default:
            LERROR("unrecognized read32 from 0x{:08X}", addr);
            return 0xFFFFFFFF;
    }
}

void Bus::Write32(u32 addr, u32 value) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x2:
            // LDEBUG("write32 0x{:08X} to 0x{:08X} (WRAM onboard)", value, masked_addr);
            for (size_t i = 0; i < 4; i++) {
                // Mask off the last 2 bits to keep word alignment.
                wram_onboard[((masked_addr & ~0b11) & 0x3FFFF) + i] = (value >> (8 * i)) & 0xFF;
            }
            return;

        case 0x3:
            LDEBUG("write32 0x{:08X} to 0x{:08X} (WRAM on-chip)", value, masked_addr);
            for (size_t i = 0; i < 4; i++) {
                // Mask off the last 2 bits to keep word alignment.
                wram_onchip[((masked_addr & ~0b11) & 0x7FFF) + i] = (value >> (8 * i)) & 0xFF;
            }
            return;

        case 0x4:
            switch (masked_addr) {
                case 0x4000000:
                    ppu.SetDISPCNT(value);
                    return;
                case 0x40000B0:
                    dma_channels[0].source_address = value;
                    return;
                case 0x40000B4:
                    dma_channels[0].destination_address = value;
                    return;
                case 0x40000B8:
                    dma_channels[0].word_count = value & 0xFFFF;
                    SetDMAControl<0>(value >> 16);
                    return;
                case 0x40000BC:
                    dma_channels[1].source_address = value;
                    return;
                case 0x40000C0:
                    dma_channels[1].destination_address = value;
                    return;
                case 0x40000C4:
                    dma_channels[1].word_count = value & 0xFFFF;
                    SetDMAControl<1>(value >> 16);
                    return;
                case 0x40000C8:
                    dma_channels[2].source_address = value;
                    return;
                case 0x40000CC:
                    dma_channels[2].destination_address = value;
                    return;
                case 0x40000D0:
                    dma_channels[2].word_count = value & 0xFFFF;
                    SetDMAControl<2>(value >> 16);
                    return;
                case 0x40000D4:
                    dma_channels[3].source_address = value;
                    return;
                case 0x40000D8:
                    dma_channels[3].destination_address = value;
                    return;
                case 0x40000DC:
                    dma_channels[3].word_count = value & 0xFFFF;
                    SetDMAControl<3>(value >> 16);
                    return;
                default:
                    LERROR("unrecognized write32 0x{:08X} to IO register 0x{:08X}", value, masked_addr);
                    return;
            }

        case 0x5:
            LDEBUG("write32 0x{:08X} to 0x{:08X} (PRAM)", value, masked_addr);
            ppu.WritePRAM<u32>(masked_addr & 0x3FF, value);
            return;

        case 0x6: {
            u32 address = masked_addr & 0x1FFFF;
            if (address > 0x17FFF) {
                address -= 0x8000;
            }

            LDEBUG("write32 0x{:08X} to 0x{:08X} (VRAM)", value, masked_addr);
            ppu.WriteVRAM<u32>(address, value);
            return;
        }

        case 0x8:
            LWARN("tried to write32 0x{:08X} to 0x{:08X} (cartridge space)", value, masked_addr);
            return;

        default:
            LERROR("unrecognized write32 0x{:08X} to 0x{:08X}", value, addr);
    }
}

template <u8 dma_channel_no>
void Bus::SetDMAControl(const u16 value) {
    static_assert(dma_channel_no < 4);
    DMAChannel& channel = dma_channels[dma_channel_no];

    channel.control.raw = value;
    if (channel.control.flags.enable) {
        RunDMATransfer<dma_channel_no>();
    }
}

template <u8 dma_channel_no>
void Bus::RunDMATransfer() {
    DMAChannel& channel = dma_channels[dma_channel_no];

    const bool transfer_32bit = channel.control.flags.transfer_type_is_32bit;
    LINFO("Running {}bit DMA{} transfer (source={:08X}, destination={:08X}, words={})", transfer_32bit ? 32 : 16,
                                                                                        dma_channel_no,
                                                                                        channel.source_address,
                                                                                        channel.destination_address,
                                                                                        channel.word_count);

    if (transfer_32bit) {
        for (std::size_t i = 0; i < channel.word_count; i++) {
            const u32 offset = i * sizeof(u32);
            Write32(channel.destination_address + offset, Read32(channel.source_address + offset));
        }
    } else {
        for (std::size_t i = 0; i < channel.word_count; i++) {
            const u32 offset = i * sizeof(u16);
            Write16(channel.destination_address + offset, Read16(channel.source_address + offset));
        }
    }

    // channel.control.flags.enable = false;
}
