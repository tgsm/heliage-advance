#include "arm7/arm7.h"

void ARM7::ARM_DataProcessing(const u32 opcode) {
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
