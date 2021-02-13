#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/reverse.hpp>
#include "arm7/arm7.h"

void ARM7::ARM_DataProcessing(const u32 opcode) {
    if ((opcode & 0x0FBF0FFF) == 0x010F0000) {
        ARM_MRS(opcode);
        return;
    }

    if ((opcode & 0x0FBFFFF0) == 0x0129F000) {
        ARM_MSR<false>(opcode);
        return;
    }

    if ((opcode & 0x0DBFF000) == 0x0128F000) {
        ARM_MSR<true>(opcode);
        return;
    }

    const bool op2_is_immediate = (opcode >> 25) & 0b1;
    const u8 op = (opcode >> 21) & 0xF;
    const bool set_condition_codes = (opcode >> 20) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u16 op2 = opcode & 0xFFF;

    if (op2_is_immediate) {
        u8 rotate_amount = (op2 >> 8) & 0xF;
        u8 imm = op2 & 0xFF;
        u32 rotated_operand = Shift_RotateRight(imm, rotate_amount << 1);

        switch (op) {
            case 0x0:
                SetRegister(rd, GetRegister(rn) & rotated_operand);
                break;
            case 0x1:
                SetRegister(rd, GetRegister(rn) ^ rotated_operand);
                break;
            case 0x2:
                SetRegister(rd, SUB(GetRegister(rn), rotated_operand, set_condition_codes));
                break;
            case 0x3: {
                u32 result = rotated_operand - GetRegister(rn);
                cpsr.flags.carry = (result < GetRegister(rn));
                if (set_condition_codes) {
                    cpsr.flags.overflow = ((GetRegister(rn) ^ result) & (rotated_operand ^ result)) >> 31;
                }
                SetRegister(rd, result);
                break;
            }
            case 0x4:
                SetRegister(rd, ADD(GetRegister(rn), rotated_operand, set_condition_codes));
                break;
            case 0x5:
                SetRegister(rd, ADC(GetRegister(rn), rotated_operand, set_condition_codes));
                break;
            case 0x6:
                SetRegister(rd, SBC(GetRegister(rn), rotated_operand, set_condition_codes));
                break;
            case 0x7:
                SetRegister(rd, RSC(GetRegister(rn), rotated_operand, set_condition_codes));
                break;
            case 0x8: {
                u32 result = GetRegister(rn) & rotated_operand;
                cpsr.flags.negative = (result & (1 << 31));
                cpsr.flags.carry = (result < GetRegister(rn));
                cpsr.flags.zero = (result == 0);

                return;
            }
            case 0x9:
                TEQ(GetRegister(rn), rotated_operand);
                return;
            case 0xA:
                CMP(GetRegister(rn), rotated_operand);
                return;
            case 0xB:
                CMN(GetRegister(rn), rotated_operand);
                return;
            case 0xC:
                SetRegister(rd, GetRegister(rn) | rotated_operand);
                break;
            case 0xD:
                if (set_condition_codes && rd == 15) {
                    cpsr.raw = GetSPSR();
                }

                SetRegister(rd, rotated_operand);
                break;
            case 0xE:
                SetRegister(rd, GetRegister(rn) & ~rotated_operand);
                break;
            case 0xF:
                SetRegister(rd, ~rotated_operand);
                break;
            default:
                UNIMPLEMENTED_MSG("unimplemented data processing op 0x{:X} w/ immediate", op);
        }
    } else {
        u8 shift = (op2 >> 4) & 0xFF;
        u8 rm = op2 & 0xF;
        if ((shift & 0b1) == 0) {
            u8 shift_amount = (shift >> 3) & 0x1F;
            ShiftType shift_type = static_cast<ShiftType>((shift >> 1) & 0b11);

            if (!shift_amount && shift_type != ShiftType::LSL) {
                shift_amount = 32;
            }

            u32 shifted_operand = Shift(GetRegister(rm), shift_type, shift_amount);

            switch (op) {
                case 0x0:
                    SetRegister(rd, GetRegister(rn) & shifted_operand);
                    break;
                case 0x1:
                    SetRegister(rd, GetRegister(rn) ^ shifted_operand);
                    break;
                case 0x2:
                    SetRegister(rd, SUB(GetRegister(rn), shifted_operand, set_condition_codes));
                    break;
                case 0x4:
                    SetRegister(rd, ADD(GetRegister(rn), shifted_operand, set_condition_codes));
                    break;
                case 0x5:
                    SetRegister(rd, ADC(GetRegister(rn), shifted_operand, set_condition_codes));
                    break;
                case 0x6:
                    SetRegister(rd, SBC(GetRegister(rn), shifted_operand, set_condition_codes));
                    break;
                case 0x7:
                    SetRegister(rd, RSC(GetRegister(rn), shifted_operand, set_condition_codes));
                    break;
                case 0x9:
                    TEQ(GetRegister(rn), shifted_operand);
                    return;
                case 0xA:
                    CMP(GetRegister(rn), shifted_operand);
                    return;
                case 0xB:
                    CMN(GetRegister(rn), shifted_operand);
                    return;
                case 0xC:
                    SetRegister(rd, GetRegister(rn) | shifted_operand);
                    break;
                case 0xD:
                    if (set_condition_codes && rd == 15) {
                        cpsr.raw = GetSPSR();
                    }

                    SetRegister(rd, shifted_operand);
                    break;
                case 0xE:
                    SetRegister(rd, GetRegister(rn) & ~shifted_operand);
                    break;
                case 0xF:
                    SetRegister(rd, ~shifted_operand);
                    break;
                default:
                    UNIMPLEMENTED_MSG("unimplemented data processing op 0x{:X} w/ register", op);
            }
        } else if ((shift & 0b1001) == 0b0001) {
            u8 rs = (shift >> 4) & 0xF;
            ShiftType shift_type = static_cast<ShiftType>((shift >> 1) & 0b11);

            u32 shifted_operand = Shift(GetRegister(rm), shift_type, GetRegister(rs));

            switch (op) {
                case 0xD:
                    if (set_condition_codes && rd == 15) {
                        cpsr.raw = GetSPSR();
                    }

                    SetRegister(rd, shifted_operand);
                    break;
                default:
                    UNIMPLEMENTED_MSG("unimplemented data processing op 0x{:X} w/ register and barrel shifter", op);
            }
        } else {
            ASSERT(false);
        }
    }

    if (set_condition_codes) {
        cpsr.flags.negative = (GetRegister(rd) & (1 << 31));
        cpsr.flags.zero = (GetRegister(rd) == 0);
    }
}

void ARM7::ARM_MRS(const u32 opcode) {
    const bool source_is_spsr = (opcode >> 22) & 0b1;
    const u8 rd = (opcode >> 12) & 0xF;

    if (source_is_spsr) {
        SetRegister(rd, GetSPSR());
    } else {
        SetRegister(rd, cpsr.raw);
    }
}

template <bool flag_bits_only>
void ARM7::ARM_MSR(const u32 opcode) {
    // TODO: from starbreeze:
    // "you might have to add some things to your msr later. (e.g. you can't change
    // control bits in User mode, there's a mask field that controls access to the
    // individual PSR fields that the ARM7TDMI reference doesn't tell you about, ...)"
    //
    // "msr won't be the only way to set a PSR. There's mode changes and PSR transfers
    // in block/single data transfers, CPU exceptions and some data processing instructions."

    const bool destination_is_spsr = (opcode >> 22) & 0b1;

    if constexpr (flag_bits_only) {
        const bool operand_is_immediate = (opcode >> 25) & 0b1;
        const u16 source_operand = opcode & 0xFFF;

        if (operand_is_immediate) {
            const u8 rotate_amount = (source_operand >> 8) & 0xF;
            const u8 immediate = source_operand & 0xFF;

            if (destination_is_spsr) {
                SetSPSR(GetSPSR() & ~0xFFFFFF00);
                SetSPSR(GetSPSR() | (Shift_RotateRight(immediate, rotate_amount << 1) & 0xFFFFFF00));
            } else {
                cpsr.raw &= ~0xFFFFFF00;
                cpsr.raw |= (Shift_RotateRight(immediate, rotate_amount << 1) & 0xFFFFFF00);
            }
        } else {
            const u8 rm = source_operand & 0xF;
            if (destination_is_spsr) {
                SetSPSR(GetSPSR() & ~0xFFFFFF00);
                SetSPSR(GetSPSR() | (GetRegister(rm) & 0xFFFFFF00));
            } else {
                cpsr.raw &= ~0xFFFFFF00;
                cpsr.raw |= (GetRegister(rm) & 0xFFFFFF00);
            }
        }
    } else {
        const u8 rm = opcode & 0xF;
        if (destination_is_spsr) {
            SetSPSR(GetRegister(rm));
        } else {
            cpsr.raw = GetRegister(rm);
        }
    }
}

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
        SetRegister(rd, mmu.Read8(GetRegister(rn)));
        mmu.Write8(GetRegister(rn), GetRegister(rm));
    } else {
        SetRegister(rd, mmu.Read32(GetRegister(rn)));
        mmu.Write32(GetRegister(rn), GetRegister(rm));
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

        mmu.Write16(address, sign ? static_cast<s16>(GetRegister(rd)) : GetRegister(rd));

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        mmu.Write16(address, sign ? static_cast<s16>(GetRegister(rd)) : GetRegister(rd));
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
            SetRegister(rd, static_cast<s16>(mmu.Read16(address)));
        } else {
            SetRegister(rd, mmu.Read16(address));
        }

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        if (sign) {
            SetRegister(rd, static_cast<s16>(mmu.Read16(address)));
        } else {
            SetRegister(rd, mmu.Read16(address));
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

        mmu.Write16(address, sign ? static_cast<s16>(value) : value);

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        mmu.Write16(address, sign ? static_cast<s16>(value) : value);
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

        SetRegister(rd, static_cast<s8>(mmu.Read8(address)));

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        SetRegister(rd, static_cast<s8>(mmu.Read8(address)));

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
                SetRegister(rd, mmu.Read8(address));
            } else {
                SetRegister(rd, mmu.Read32(address));
            }
        } else {
            if constexpr (transfer_byte) {
                mmu.Write8(address, GetRegister(rd) & 0xFF);
            } else {
                mmu.Write32(address, GetRegister(rd));
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
                SetRegister(rd, mmu.Read8(address));
            } else {
                SetRegister(rd, mmu.Read32(address));
            }
        } else {
            if constexpr (transfer_byte) {
                mmu.Write8(address, GetRegister(rd) & 0xFF);
            } else {
                mmu.Write32(address, GetRegister(rd));
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
                    SetRegister(reg, mmu.Read32(address));
                } else {
                    SetRegister(reg, mmu.Read32(address));
                    address += 4;
                }
            }
        } else {
            for (u8 reg : set_bits | ranges::views::reverse) {
                if (pre_indexing) {
                    address -= 4;
                    SetRegister(reg, mmu.Read32(address));
                } else {
                    SetRegister(reg, mmu.Read32(address));
                    address -= 4;
                }
            }
        }
    } else {
        if (add_offset_to_base) {
            for (u8 reg : set_bits) {
                if (pre_indexing) {
                    address += 4;
                    mmu.Write32(address, GetRegister(reg));
                } else {
                    mmu.Write32(address, GetRegister(reg));
                    address += 4;
                }
            }
        } else {
            for (u8 reg : set_bits | ranges::views::reverse) {
                if (pre_indexing) {
                    address -= 4;
                    mmu.Write32(address, GetRegister(reg));
                } else {
                    mmu.Write32(address, GetRegister(reg));
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
