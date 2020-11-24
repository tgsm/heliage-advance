#include <ranges>
#include "arm7/arm7.h"

void ARM7::ARM_DataProcessing(const u32 opcode) {
    if ((opcode & 0x0FBF0FFF) == 0x010F0000) {
        ARM_MRS(opcode);
        return;
    }

    if ((opcode & 0x0FBFFFF0) == 0x0129F000) {
        ARM_MSR(opcode, false);
        return;
    }

    if ((opcode & 0x0DBFF000) == 0x0128F000) {
        ARM_MSR(opcode, true);
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
        u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

        switch (op) {
            case 0x0:
                SetRegister(rd, GetRegister(rn) & rotated_operand);
                break;
            case 0x1:
                SetRegister(rd, GetRegister(rn) ^ rotated_operand);
                break;
            case 0x2: {
                u32 result = GetRegister(rn) - rotated_operand;
                cpsr.flags.carry = (result <= GetRegister(rn));
                if (set_condition_codes) {
                    cpsr.flags.overflow = ((GetRegister(rn) ^ result) & (~rotated_operand ^ result)) >> 31;
                }
                SetRegister(rd, result);
                break;
            }
            case 0x4: {
                u32 result = GetRegister(rn) + rotated_operand;
                cpsr.flags.carry = (result < GetRegister(rn));
                if (set_condition_codes) {
                    cpsr.flags.overflow = ((GetRegister(rn) ^ result) & (rotated_operand ^ result)) >> 31;
                }
                SetRegister(rd, result);
                break;
            }
            case 0x5: {
                u32 result = GetRegister(rn) + rotated_operand + cpsr.flags.carry;
                cpsr.flags.carry = (result < GetRegister(rn));
                if (set_condition_codes) {
                    cpsr.flags.overflow = ((GetRegister(rn) ^ result) & (rotated_operand ^ result)) >> 31;
                }
                SetRegister(rd, result);
                break;
            }
            case 0x6: {
                u32 result = GetRegister(rn) - rotated_operand + cpsr.flags.carry - 1;
                cpsr.flags.carry = (result <= GetRegister(rn));
                if (set_condition_codes) {
                    cpsr.flags.overflow = ((GetRegister(rn) ^ result) & (~rotated_operand ^ result)) >> 31;
                }
                SetRegister(rd, result);
                break;
            }
            case 0x8: {
                u32 result = GetRegister(rn) & rotated_operand;
                cpsr.flags.negative = (result & (1 << 31));
                cpsr.flags.carry = (result < GetRegister(rn));
                cpsr.flags.zero = (result == 0);

                return;
            }
            case 0xA: {
                u32 result = GetRegister(rn) - rotated_operand;
                cpsr.flags.negative = (result & (1 << 31));
                cpsr.flags.carry = (result < GetRegister(rn));
                cpsr.flags.zero = (result == 0);

                return;
            }
            case 0xC:
                SetRegister(rd, GetRegister(rn) | rotated_operand);
                break;
            case 0xD:
                SetRegister(rd, rotated_operand);
                break;
            case 0xF:
                SetRegister(rd, ~rotated_operand);
                break;
            default:
                UNIMPLEMENTED_MSG("unimplemented data processing op 0x%X", op);
        }
    } else {
        u8 shift = (op2 >> 4) & 0xFF;
        u8 rm = op2 & 0xF;
        if ((shift & 0b1) == 0) {
            u8 shift_amount = (shift >> 3) & 0x1F;
            u8 shift_type = (shift >> 1) & 0b11;

            if (!shift_amount) {
                switch (op) {
                    case 0x0:
                        SetRegister(rd, GetRegister(rn) & GetRegister(rm));
                        break;
                    case 0x1:
                        SetRegister(rd, GetRegister(rn) ^ GetRegister(rm));
                        break;
                    case 0x2:
                        SetRegister(rd, GetRegister(rn) - GetRegister(rm));
                        break;
                    case 0x4:
                        SetRegister(rd, GetRegister(rn) + GetRegister(rm));
                        break;
                    case 0x5:
                        SetRegister(rd, GetRegister(rn) + GetRegister(rm) + cpsr.flags.carry);
                        break;
                    case 0x6:
                        SetRegister(rd, GetRegister(rn) - GetRegister(rm) + cpsr.flags.carry - 1);
                        break;
                    case 0x7:
                        SetRegister(rd, GetRegister(rm) - GetRegister(rn) + cpsr.flags.carry - 1);
                        break;
                    case 0xA: {
                        u32 result = GetRegister(rn) - GetRegister(rm);
                        cpsr.flags.negative = (result & (1 << 31));
                        cpsr.flags.carry = (result < GetRegister(rn));
                        cpsr.flags.zero = (result == 0);

                        return;
                    }
                    case 0xB: {
                        u32 result = GetRegister(rn) + GetRegister(rm);
                        cpsr.flags.negative = (result & (1 << 31));
                        cpsr.flags.carry = (result < GetRegister(rn));
                        cpsr.flags.zero = (result == 0);

                        return;
                    }
                    case 0xC:
                        SetRegister(rd, GetRegister(rn) | GetRegister(rm));
                        break;
                    case 0xD:
                        SetRegister(rd, GetRegister(rm));
                        break;
                    case 0xF:
                        SetRegister(rd, ~GetRegister(rm));
                        break;
                    default:
                        UNIMPLEMENTED_MSG("unimplemented data processing op 0x%X with zero shift", op);
                }

                return;
            }

            switch ((op << 2) | shift_type) {
                // Case format: XXXXYY
                // X: op, Y: shift type
                case 0b000001: // AND w/ LSR
                    SetRegister(rd, GetRegister(rn) & (GetRegister(rm) >> shift_amount));
                    break;
                case 0b000100: // EOR w/ LSL
                    SetRegister(rd, GetRegister(rn) ^ (GetRegister(rm) << shift_amount));
                    break;
                case 0b010000: // ADD w/ LSL
                    SetRegister(rd, GetRegister(rn) + (GetRegister(rm) << shift_amount));
                    break;
                case 0b010101: // ADC w/ LSR
                    SetRegister(rd, GetRegister(rn) + (GetRegister(rm) >> shift_amount) + cpsr.flags.carry);
                    break;
                case 0b110100: // MOV w/ LSL
                    SetRegister(rd, (GetRegister(rm) << shift_amount));
                    break;
                case 0b110000: // ORR w/ LSL
                    SetRegister(rd, GetRegister(rn) | (GetRegister(rm) << shift_amount));
                    break;
                case 0b110011: // ORR w/ ROR
                    SetRegister(rd, GetRegister(rn) | Shift_RotateRight(GetRegister(rm), shift_amount));
                    break;
                case 0b110101: // MOV w/ LSR
                    SetRegister(rd, (GetRegister(rm) >> shift_amount));
                    break;
                case 0b110111: // MOV w/ ROR
                    SetRegister(rd, Shift_RotateRight(GetRegister(rm), shift_amount));
                    break;
                case 0b111010: // BIC w/ ASR:
                    // TODO: carry flag
                    SetRegister(rd, (GetRegister(rm) >> shift_amount));
                    break;
                default:
                    UNIMPLEMENTED_MSG("unimplemented data processing op 0x%X with shift type 0x%X", op, shift_type);
            }
        } else if ((shift & 0b1001) == 0b0001) {
            u8 rs = (shift >> 4) & 0xF;
            u8 shift_type = (shift >> 1) & 0b11;

            // Case format: XXXXYY
            // X: op, Y: shift type
            switch ((op << 2) | shift_type) {
                case 0b110100: // MOV w/ LSL
                    SetRegister(rd, GetRegister(rm) << GetRegister(rs));
                    break;
                case 0b110110: // MOV w/ ASR
                    // TODO: carry flag
                    SetRegister(rd, GetRegister(rm) >> GetRegister(rs));
                    break;
                default:
                    UNIMPLEMENTED_MSG("unimplemented data processing op 0x%X with shift type 0x%X", op, shift_type);
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
        SetRegister(rd, spsr.raw);
    } else {
        SetRegister(rd, cpsr.raw);
    }
}

void ARM7::ARM_MSR(const u32 opcode, const bool flag_bits_only) {
    // TODO: from starbreeze:
    // "you might have to add some things to your msr later. (e.g. you can't change
    // control bits in User mode, there's a mask field that controls access to the
    // individual PSR fields that the ARM7TDMI reference doesn't tell you about, ...)"
    //
    // "msr won't be the only way to set a PSR. There's mode changes and PSR transfers
    // in block/single data transfers, CPU exceptions and some data processing instructions."

    const bool destination_is_spsr = (opcode >> 22) & 0b1;

    if (flag_bits_only) {
        const bool operand_is_immediate = (opcode >> 25) & 0b1;
        const u16 source_operand = opcode & 0xFFF;

        if (operand_is_immediate) {
            const u8 rotate_amount = (source_operand >> 8) & 0xF;
            const u8 immediate = source_operand & 0xFF;

            if (destination_is_spsr) {
                spsr.raw &= ~0xFFFFFF00;
                spsr.raw |= (Shift_RotateRight(immediate, rotate_amount * 2) & 0xFFFFFF00);
            } else {
                cpsr.raw &= ~0xFFFFFF00;
                cpsr.raw |= (Shift_RotateRight(immediate, rotate_amount * 2) & 0xFFFFFF00);
            }
        } else {
            const u8 rm = source_operand & 0xF;
            if (destination_is_spsr) {
                spsr.raw &= ~0xFFFFFF00;
                spsr.raw = (GetRegister(rm) & 0xFFFFFF00);
            } else {
                cpsr.raw &= ~0xFFFFFF00;
                cpsr.raw = (GetRegister(rm) & 0xFFFFFF00);
            }
        }
    } else {
        const u8 rm = opcode & 0xF;
        if (destination_is_spsr) {
            spsr.raw = GetRegister(rm);
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

    ASSERT_MSG(!sign, "unimplemented signed multiply long");

    u64 result = GetRegister(rm) * GetRegister(rs);
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

void ARM7::ARM_BranchAndExchange(const u32 opcode) {
    const u8 rn = opcode & 0xF;

    // If bit 0 of Rn is set, we switch to THUMB mode. Else, we switch to ARM mode.
    cpsr.flags.thumb_mode = GetRegister(rn) & 0b1;
    LDEBUG("thumb: %u", cpsr.flags.thumb_mode);

    SetPC(GetRegister(rn) & ~0b1);
}

void ARM7::ARM_HalfwordDataTransferRegister(const u32 opcode) {
    const bool load_from_memory = (opcode >> 20) & 0b1;
    const bool sign = (opcode >> 6) & 0b1;
    const bool halfword = (opcode >> 5) & 0b1;

    const u8 conditions = (load_from_memory << 1) | halfword;
    switch (conditions) {
        case 0b01:
            ARM_StoreHalfwordRegister(opcode, sign);
            break;
        default:
            UNIMPLEMENTED_MSG("unimplemented halfword data transfer register conditions 0x%X", conditions);
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
            UNIMPLEMENTED_MSG("unimplemented SWP");
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
    const u8 offset = (offset_high << 8) | offset_low;

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
    const u8 offset = (offset_high << 8) | offset_low;

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
    const s8 offset = (offset_high << 8) | offset_low;

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
            ARM_StoreWord(opcode);
            break;
        case 0b001:
            ARM_LoadWord(opcode);
            break;
        case 0b100:
            ARM_StoreByte(opcode);
            break;
        case 0b101:
            ARM_LoadByte(opcode);
            break;
        default:
            UNREACHABLE();
    }
}

void ARM7::ARM_LoadWord(const u32 opcode) {
    const bool offset_is_register = (opcode >> 25) & 0b1;
    const bool add_before_transfer = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    s16 offset = 0;
    if (offset_is_register) {
        u8 shift = (opcode >> 4) & 0xFF;
        u8 rm = opcode & 0xF;

        if ((shift & 0b1) == 0) {
            u8 shift_amount = (shift >> 3) & 0x1F;
            u8 shift_type = (shift >> 1) & 0b11;

            if (!shift_amount) {
                offset = (add_offset_to_base) ? GetRegister(rm) : -GetRegister(rm);
            } else {
                switch (shift_type) {
                    case 0b00:
                        offset = (GetRegister(rm) << shift_amount);
                        break;
                    case 0b01:
                        offset = (GetRegister(rm) >> shift_amount);
                        break;
                    default:
                        UNIMPLEMENTED();
                }
            }

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

        address &= ~0x3; // Word align
        SetRegister(rd, mmu.Read32(address));

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        address &= ~0x3; // Word align
        SetRegister(rd, mmu.Read32(address));

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_StoreWord(const u32 opcode) {
    const bool offset_is_register = (opcode >> 25) & 0b1;
    const bool add_before_transfer = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    u16 offset = 0;
    if (offset_is_register) {
        UNIMPLEMENTED_MSG("unimplemented store word with register offset");
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

        address &= ~0x3; // Word align
        mmu.Write32(address, GetRegister(rd));

        if (write_back) {
            UNIMPLEMENTED_MSG("unimplemented store word write-back");
        }
    } else {
        address &= ~0x3; // Word align
        mmu.Write32(address, GetRegister(rd));

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_LoadByte(const u32 opcode) {
    const bool offset_is_register = (opcode >> 25) & 0b1;
    const bool add_before_transfer = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    u16 offset = 0;
    if (offset_is_register) {
        UNIMPLEMENTED_MSG("unimplemented load byte with register offset");
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

        SetRegister(rd, mmu.Read8(address));

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        SetRegister(rd, mmu.Read8(address));

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_StoreByte(const u32 opcode) {
    const bool offset_is_register = (opcode >> 25) & 0b1;
    const bool add_before_transfer = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    u16 offset = 0;
    if (offset_is_register) {
        UNIMPLEMENTED_MSG("unimplemented store byte with register offset");
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

        mmu.Write8(address, GetRegister(rd));

        if (write_back) {
            UNIMPLEMENTED_MSG("unimplemented store byte write-back");
        }
    } else {
        mmu.Write8(address, GetRegister(rd));

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
    std::vector<u8> set_bits;
    for (u8 i = 0; i < 16; i++) {
        if (rlist & (1 << i)) {
            set_bits.push_back(i);
        }
    }

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
            for (u8 reg : set_bits | std::views::reverse) {
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
            for (u8 reg : set_bits | std::views::reverse) {
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

void ARM7::ARM_Branch(const u32 opcode) {
    const bool link = (opcode >> 24) & 0b1;
    s32 offset = (opcode & 0xFFFFFF) << 2;

    offset <<= 6;
    offset >>= 6;

    if (link) {
        SetLR(GetPC() - 4);
    }

    SetPC(GetPC() + offset);
}

// TODO: ARM coprocessor data transfer

// TODO: ARM coprocessor data operation

// TODO: ARM coprocessor register transfer