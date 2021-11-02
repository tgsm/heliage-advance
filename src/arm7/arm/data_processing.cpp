#include "arm7/arm7.h"

void ARM7::ARM_DataProcessing(const u32 opcode) {
    if ((opcode & 0x0FBF0FFF) == 0x010F0000) {
        ARM_MRS(opcode);
        return;
    }

    if ((opcode & 0x0DBFF000) == 0x0129F000) {
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

            if (shift_amount == 0 && shift_type != ShiftType::LSL) {
                if (shift_type == ShiftType::ROR) {
                    shift_type = ShiftType::RRX;
                } else {
                    shift_amount = 32;
                }
            }

            u32 shifted_operand = 0;
            if (shift_type == ShiftType::RRX) {
                shifted_operand = Shift_RRX(GetRegister(rm));
            } else {
                shifted_operand = Shift(GetRegister(rm), shift_type, shift_amount);
            }

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

// TODO: from starbreeze:
// "you might have to add some things to your msr later. (e.g. you can't change
// control bits in User mode, there's a mask field that controls access to the
// individual PSR fields that the ARM7TDMI reference doesn't tell you about, ...)"
//
// "msr won't be the only way to set a PSR. There's mode changes and PSR transfers
// in block/single data transfers, CPU exceptions and some data processing instructions."

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
    const bool destination_is_spsr = (opcode >> 22) & 0b1;
    const bool operand_is_immediate = (opcode >> 25) & 0b1;
    const u16 source_operand = opcode & 0xFFF;

    if (operand_is_immediate) {
        const u8 rotate_amount = (source_operand >> 8) & 0xF;
        const u8 immediate = source_operand & 0xFF;

        if constexpr (flag_bits_only) {
            if (destination_is_spsr) {
                SetSPSR(GetSPSR() & ~0xFFFFFF00);
                SetSPSR(GetSPSR() | (Shift_RotateRight(immediate, rotate_amount << 1) & 0xFFFFFF00));
            } else {
                cpsr.raw &= ~0xFFFFFF00;
                cpsr.raw |= (Shift_RotateRight(immediate, rotate_amount << 1) & 0xFFFFFF00);
            }
        } else {
            if (destination_is_spsr) {
                SetSPSR(Shift_RotateRight(immediate, rotate_amount << 1));
            } else {
                cpsr.raw = Shift_RotateRight(immediate, rotate_amount << 1);
            }
        }
    } else {
        const u8 rm = source_operand & 0xF;
        if constexpr (flag_bits_only) {
            if (destination_is_spsr) {
                SetSPSR(GetSPSR() & ~0xFFFFFF00);
                SetSPSR(GetSPSR() | (GetRegister(rm) & 0xFFFFFF00));
            } else {
                cpsr.raw &= ~0xFFFFFF00;
                cpsr.raw |= (GetRegister(rm) & 0xFFFFFF00);
            }
        } else {
            if (destination_is_spsr) {
                SetSPSR(GetRegister(rm));
            } else {
                cpsr.raw = GetRegister(rm);
            }
        }
    }
}
