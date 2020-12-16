#include "logging.h"
#include "mmu.h"

MMU::MMU(Cartridge& cartridge, Keypad& keypad, PPU& ppu)
    : cartridge(cartridge), keypad(keypad), ppu(ppu) {
    wram.fill(0x00);
}

u8 MMU::Read8(u32 addr) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x3: {
            u8 value = wram.at(masked_addr & 0x7FFF);
            LDEBUG("read8 0x{:02X} from 0x{:08X} (WRAM)", value, masked_addr);
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

void MMU::Write8(u32 addr, u8 value) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x3:
            LDEBUG("write8 0x{:02X} to 0x{:08X} (VRAM)", value, masked_addr);
            wram[masked_addr & 0x3FFFF] = value;
            return;
        default:
            LERROR("unrecognized write8 0x{:02X} to 0x{:08X}", value, masked_addr);
            return;
    }
}

u16 MMU::Read16(u32 addr) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x3: {
            u16 value = 0;
            for (std::size_t i = 0; i < 2; i++) {
                // Mask off the last bit to keep halfword alignment.
                value |= ((wram.at(((masked_addr & ~0b1) & 0x7FFF) + i)) & 0xFF) << (8 * i);
            }

            LDEBUG("read16 0x{:04X} from 0x{:08X} (WRAM)", value, masked_addr);

            return value;
        }

        case 0x4:
            switch (masked_addr) {
                case 0x4000004:
                    return ppu.GetDISPSTAT();
                case 0x4000006:
                    return ppu.GetVCOUNT();
                case 0x4000130: // Keypad input
                    return keypad.GetState();
                default:
                    LERROR("unrecognized read16 from IO register 0x{:08X}", masked_addr);
                    return 0xFFFF;
            }

            return 0xFFFF;

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

void MMU::Write16(u32 addr, u16 value) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x3:
            LDEBUG("write16 0x{:04X} to 0x{:08X} (WRAM)", value, masked_addr);
            for (size_t i = 0; i < 2; i++) {
                // Mask off the last bit to keep halfword alignment.
                wram[((masked_addr & ~0b1) & 0x7FFF) + i] = (value >> (8 * i)) & 0xFF;
            }
            return;

        case 0x4:
            switch (masked_addr) {
                case 0x4000000:
                    ppu.SetDISPCNT(value);
                    return;
                default:
                    LERROR("unrecognized write16 0x{:04X} to 0x{:08X} (IO)", value, masked_addr);
                    return;
            }

        case 0x5:
            LDEBUG("write16 0x{:04X} to 0x{:08X} (PRAM)", value, masked_addr);
            for (size_t i = 0; i < 2; i++) {
                // Mask off the last bit to keep halfword alignment.
                ppu.WritePRAM8(((masked_addr & ~0b1) & 0x3FF) + i, (value >> (8 * i)) & 0xFF);
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

u32 MMU::Read32(u32 addr) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x3: {
            u32 value = 0;
            for (std::size_t i = 0; i < 4; i++) {
                // Mask off the last 2 bits to keep word alignment.
                value |= ((wram.at(((masked_addr & ~0b11) & 0x7FFF) + i)) & 0xFF) << (8 * i);
            }

            LDEBUG("read32 0x{:08X} from 0x{:08X} (WRAM)", value, masked_addr);

            return value;
        }

        case 0x4:
            switch (masked_addr) {
                case 0x4000004:
                    return ppu.GetDISPSTAT();
                case 0x4000130:
                    return keypad.GetState();
                default:
                    LERROR("unrecognized read32 from 0x{:08X} (IO)", masked_addr);
                    return 0xFFFFFFFF;
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

void MMU::Write32(u32 addr, u32 value) {
    const u32 masked_addr = addr & 0x0FFFFFFF;
    switch ((masked_addr >> 24) & 0xF) {
        case 0x3:
            LDEBUG("write32 0x{:08X} to 0x{:08X} (WRAM)", value, masked_addr);
            for (size_t i = 0; i < 4; i++) {
                // Mask off the last 2 bits to keep word alignment.
                wram[((masked_addr & ~0b11) & 0x7FFF) + i] = (value >> (8 * i)) & 0xFF;
            }
            return;

        case 0x4:
            switch (masked_addr) {
                case 0x4000000:
                    ppu.SetDISPCNT(value);
                    return;
                default:
                    LERROR("unrecognized write32 0x{:08X} to 0x{:08X} (IO)", value, masked_addr);
                    return;
            }

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
