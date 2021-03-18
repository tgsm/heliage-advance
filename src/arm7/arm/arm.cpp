#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/reverse.hpp>
#include "arm7/arm7.h"

void ARM7::ARM_Multiply(const u32 opcode) {
    const bool accumulate = (opcode >> 21) & 0b1;
    const bool set_condition_codes = (opcode >> 20) & 0b1;
    const u8 rd = (opcode >> 16) & 0xF;
    const u8 rn = (opcode >> 12) & 0xF;
    const u8 rs = (opcode >> 8) & 0xF;
    const u8 rm = opcode & 0xF;

    SetRegister(rd, GetRegister(rm) * GetRegister(rs));

    if (accumulate) {
        SetRegister(rd, GetRegister(rd) + GetRegister(rn));
    }

    if (set_condition_codes) {
        cpsr.flags.negative = (GetRegister(rd) & (1 << 31));
        cpsr.flags.zero = (GetRegister(rd) == 0);
    }
}

void ARM7::ARM_MultiplyLong(const u32 opcode) {
    const bool sign = (opcode >> 22) & 0b1;
    const bool accumulate = (opcode >> 21) & 0b1;
    const bool set_condition_codes = (opcode >> 20) & 0b1;
    const u8 rdhi = (opcode >> 16) & 0xF;
    const u8 rdlo = (opcode >> 12) & 0xF;
    const u8 rs = (opcode >> 8) & 0xF;
    const u8 rm = opcode & 0xF;

    if (sign) {
        s64 result = static_cast<s64>(GetRegister(rm)) * static_cast<s64>(GetRegister(rs));
        if (accumulate) {
            result += ((static_cast<s64>(GetRegister(rdhi)) << 32) | GetRegister(rdlo));
        }
        SetRegister(rdlo, result & 0xFFFFFFFF);
        SetRegister(rdhi, result >> 32);

        if (set_condition_codes) {
            cpsr.flags.negative = (result >> 63);
            cpsr.flags.zero = (result == 0);
        }
    } else {
        u64 result = static_cast<u64>(GetRegister(rm)) * static_cast<u64>(GetRegister(rs));
        if (accumulate) {
            result += ((static_cast<u64>(GetRegister(rdhi)) << 32) | GetRegister(rdlo));
        }
        SetRegister(rdlo, result & 0xFFFFFFFF);
        SetRegister(rdhi, result >> 32);

        if (set_condition_codes) {
            cpsr.flags.negative = (result >> 63);
            cpsr.flags.zero = (result == 0);
        }
    }
}

void ARM7::ARM_SingleDataSwap(const u32 opcode) {
    const bool swap_byte = (opcode >> 22) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u8 rm = opcode & 0xF;

    if (swap_byte) {
        SetRegister(rd, bus.Read8(GetRegister(rn)));
        bus.Write8(GetRegister(rn), GetRegister(rm));
    } else {
        SetRegister(rd, bus.Read32(GetRegister(rn)));
        bus.Write32(GetRegister(rn), GetRegister(rm));
    }
}

void ARM7::ARM_HalfwordDataTransferRegister(const u32 opcode) {
    const bool load_from_memory = (opcode >> 20) & 0b1;
    const bool sign = (opcode >> 6) & 0b1;
    const bool halfword = (opcode >> 5) & 0b1;

    const u8 conditions = (load_from_memory << 1) | halfword;
    switch (conditions) {
        case 0b00:
            ARM_SingleDataSwap(opcode);
            break;
        case 0b01:
            ARM_StoreHalfwordRegister(opcode, sign);
            break;
        default:
            UNIMPLEMENTED_MSG("unimplemented halfword data transfer register conditions 0x{:X}", conditions);
    }
}

void ARM7::ARM_StoreHalfwordRegister(const u32 opcode, const bool sign) {
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u8 rm = opcode & 0xF;

    u32 address = GetRegister(rn);
    if (pre_indexing) {
        if (add_offset_to_base) {
            address += GetRegister(rm);
        } else {
            address -= GetRegister(rm);
        }

        bus.Write16(address, sign ? static_cast<s16>(GetRegister(rd)) : GetRegister(rd));

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        bus.Write16(address, sign ? static_cast<s16>(GetRegister(rd)) : GetRegister(rd));
        if (add_offset_to_base) {
            address += GetRegister(rm);
        } else {
            address -= GetRegister(rm);
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_HalfwordDataTransferImmediate(const u32 opcode) {
    const bool load_from_memory = (opcode >> 20) & 0b1;
    const bool sign = (opcode >> 6) & 0b1;
    const bool halfword = (opcode >> 5) & 0b1;

    const u8 conditions = (load_from_memory << 1) | halfword;
    switch (conditions) {
        case 0b00:
            ARM_SingleDataSwap(opcode);
            break;
        case 0b01:
            ARM_StoreHalfwordImmediate(opcode, sign);
            break;
        case 0b10:
            // Not to be confused with ARM_LoadByte
            ARM_LoadSignedByte(opcode);
            break;
        case 0b11:
            ARM_LoadHalfwordImmediate(opcode, sign);
            break;
        default:
            UNREACHABLE();
    }
}

void ARM7::ARM_LoadHalfwordImmediate(const u32 opcode, const bool sign) {
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u8 offset_high = (opcode >> 8) & 0xF;
    const u8 offset_low = opcode & 0xF;
    const u8 offset = (offset_high << 4) | offset_low;

    u32 address = GetRegister(rn);
    if (pre_indexing) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        if (sign) {
            SetRegister(rd, static_cast<s16>(bus.Read16(address)));
        } else {
            SetRegister(rd, bus.Read16(address));
        }

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        if (sign) {
            SetRegister(rd, static_cast<s16>(bus.Read16(address)));
        } else {
            SetRegister(rd, bus.Read16(address));
        }

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_StoreHalfwordImmediate(const u32 opcode, const bool sign) {
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u8 offset_high = (opcode >> 8) & 0xF;
    const u8 offset_low = opcode & 0xF;
    const u8 offset = (offset_high << 4) | offset_low;

    u32 address = GetRegister(rn);
    u16 value = GetRegister(rd) & 0xFFFF;
    if (pre_indexing) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        bus.Write16(address, sign ? static_cast<s16>(value) : value);

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        bus.Write16(address, sign ? static_cast<s16>(value) : value);
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_LoadSignedByte(const u32 opcode) {
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const s8 offset_high = (opcode >> 8) & 0xF;
    const s8 offset_low = opcode & 0xF;
    const s8 offset = (offset_high << 4) | offset_low;

    u32 address = GetRegister(rn);
    if (pre_indexing) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rd, static_cast<s8>(bus.Read8(address)));

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        SetRegister(rd, static_cast<s8>(bus.Read8(address)));

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_SingleDataTransfer(const u32 opcode) {
    const u16 operation = (opcode >> 20) & 0b101;
    switch (operation) {
        case 0b000:
            // Store word
            ARM_SingleDataTransfer_Impl<false, false>(opcode);
            break;
        case 0b001:
            // Load word
            ARM_SingleDataTransfer_Impl<true, false>(opcode);
            break;
        case 0b100:
            // Store byte
            ARM_SingleDataTransfer_Impl<false, true>(opcode);
            break;
        case 0b101:
            // Load byte
            ARM_SingleDataTransfer_Impl<true, true>(opcode);
            break;
        default:
            UNREACHABLE();
    }
}

template <bool load_from_memory, bool transfer_byte>
void ARM7::ARM_SingleDataTransfer_Impl(const u32 opcode) {
    const bool offset_is_register = (opcode >> 25) & 0b1;
    const bool add_before_transfer = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    s16 offset = 0;
    if (offset_is_register) {
        const u8 shift = (opcode >> 4) & 0xFF;
        const u8 rm = opcode & 0xF;

        if ((shift & 0b1) == 0) {
            const u8 shift_amount = (shift >> 3) & 0x1F;
            const ShiftType shift_type = static_cast<ShiftType>((shift >> 1) & 0b11);

            offset = Shift(GetRegister(rm), shift_type, shift_amount);
            if (!add_offset_to_base) {
                offset = -offset;
            }
        } else if ((shift & 0b1001) == 0b0001) {
            UNIMPLEMENTED();
        } else {
            ASSERT(false);
        }
    } else {
        offset = opcode & 0xFFF;
    }

    u32 address = GetRegister(rn);
    if (add_before_transfer) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        if constexpr (!transfer_byte) {
            address &= ~0x3; // Word align
        }

        if constexpr (load_from_memory) {
            if constexpr (transfer_byte) {
                SetRegister(rd, bus.Read8(address));
            } else {
                SetRegister(rd, bus.Read32(address));
            }
        } else {
            if constexpr (transfer_byte) {
                bus.Write8(address, GetRegister(rd) & 0xFF);
            } else {
                bus.Write32(address, GetRegister(rd));
            }
        }

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        if constexpr (!transfer_byte) {
            address &= ~0x3; // Word align
        }

        if constexpr (load_from_memory) {
            if constexpr (transfer_byte) {
                SetRegister(rd, bus.Read8(address));
            } else {
                SetRegister(rd, bus.Read32(address));
            }
        } else {
            if constexpr (transfer_byte) {
                bus.Write8(address, GetRegister(rd) & 0xFF);
            } else {
                bus.Write32(address, GetRegister(rd));
            }
        }

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_BlockDataTransfer(const u32 opcode) {
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool load_psr = (opcode >> 22) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const bool load_from_memory = (opcode >> 20) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u16 rlist = opcode & 0xFFFF;

    ASSERT_MSG(!load_psr, "unimplemented load PSR in block data transfer");

    // Go through `rlist`'s 16 bits, and write down which bits are set. This tells
    // us which registers we need to load/store.
    // If bit 0 is set, that corresponds to R0. If bit 1 is set, that corresponds
    // to R1, and so on.
    const auto bit_is_set = [rlist](const u8 i) { return (rlist & (1 << i)) != 0; };
    const auto set_bits = ranges::views::iota(0, 16)
                        | ranges::views::filter(bit_is_set)
                        | ranges::to<std::vector>;

    if (set_bits.empty()) {
        return;
    }

    u32 address = GetRegister(rn);
    if (load_from_memory) {
        if (add_offset_to_base) {
            for (u8 reg : set_bits) {
                if (pre_indexing) {
                    address += 4;
                    SetRegister(reg, bus.Read32(address));
                } else {
                    SetRegister(reg, bus.Read32(address));
                    address += 4;
                }
            }
        } else {
            for (u8 reg : set_bits | ranges::views::reverse) {
                if (pre_indexing) {
                    address -= 4;
                    SetRegister(reg, bus.Read32(address));
                } else {
                    SetRegister(reg, bus.Read32(address));
                    address -= 4;
                }
            }
        }
    } else {
        if (add_offset_to_base) {
            for (u8 reg : set_bits) {
                if (pre_indexing) {
                    address += 4;
                    bus.Write32(address, GetRegister(reg));
                } else {
                    bus.Write32(address, GetRegister(reg));
                    address += 4;
                }
            }
        } else {
            for (u8 reg : set_bits | ranges::views::reverse) {
                if (pre_indexing) {
                    address -= 4;
                    bus.Write32(address, GetRegister(reg));
                } else {
                    bus.Write32(address, GetRegister(reg));
                    address -= 4;
                }
            }
        }
    }

    if (write_back) {
        SetRegister(rn, address);
    }
}

// TODO: ARM coprocessor data transfer

// TODO: ARM coprocessor data operation

// TODO: ARM coprocessor register transfer
