#include "common/bits.h"
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

    const bool op2_is_immediate = Common::IsBitSet<25>(opcode);
    const std::unsigned_integral auto op = Common::GetBitRange<24, 21>(opcode);
    const bool set_condition_codes = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto op2 = Common::GetBitRange<11, 0>(opcode);

    if (op2_is_immediate) {
        const std::unsigned_integral auto rotate_amount = Common::GetBitRange<11, 8>(op2);
        const std::unsigned_integral auto imm = Common::GetBitRange<7, 0>(op2);
        const u32 rotated_operand = Shift_RotateRight(imm, rotate_amount << 1, set_condition_codes);

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
            case 0x3:
                SetRegister(rd, SUB(rotated_operand, GetRegister(rn), set_condition_codes));
                break;
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
                SetRegister(rd, SBC(rotated_operand, GetRegister(rn), set_condition_codes));
                break;
            case 0x8:
                TST(GetRegister(rn), rotated_operand);
                return;
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
        const std::unsigned_integral auto shift = Common::GetBitRange<11, 4>(op2);
        const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(op2);
        if (!Common::IsBitSet<0>(shift)) {
            std::unsigned_integral auto shift_amount = Common::GetBitRange<7, 3>(shift);
            auto shift_type = ShiftType(Common::GetBitRange<2, 1>(shift));

            if (shift_amount == 0 && shift_type != ShiftType::LSL) {
                if (shift_type == ShiftType::ROR) {
                    shift_type = ShiftType::RRX;
                } else {
                    shift_amount = 32;
                }
            }

            u32 shifted_operand = 0;
            if (shift_type == ShiftType::RRX) {
                shifted_operand = Shift_RRX(GetRegister(rm), set_condition_codes);
            } else {
                shifted_operand = Shift(GetRegister(rm), shift_type, shift_amount, set_condition_codes);
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
                case 0x3:
                    SetRegister(rd, SUB(shifted_operand, GetRegister(rn), set_condition_codes));
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
                    SetRegister(rd, SBC(shifted_operand, GetRegister(rn), set_condition_codes));
                    break;
                case 0x8:
                    TST(GetRegister(rn), shifted_operand);
                    return;
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
            const std::unsigned_integral auto rs = Common::GetBitRange<7, 4>(shift);
            const auto shift_type = ShiftType(Common::GetBitRange<2, 1>(shift));

            const u32 shifted_operand = Shift(GetRegister(rm), shift_type, GetRegister(rs), set_condition_codes);

            switch (op) {
                case 0x0:
                    SetRegister(rd, GetRegister(rn) | shifted_operand);
                    break;
                case 0x4:
                    SetRegister(rd, ADD(GetRegister(rn), shifted_operand, set_condition_codes));
                    break;
                case 0x5:
                    SetRegister(rd, ADC(GetRegister(rn), shifted_operand, set_condition_codes));
                    break;
                case 0xC:
                    SetRegister(rd, GetRegister(rn) | shifted_operand);
                    break;
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
        cpsr.flags.negative = Common::IsBitSet<31>(GetRegister(rd));
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
    const bool source_is_spsr = Common::IsBitSet<22>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);

    if (source_is_spsr) {
        SetRegister(rd, GetSPSR());
    } else {
        SetRegister(rd, cpsr.raw);
    }
}

template <bool flag_bits_only>
void ARM7::ARM_MSR(const u32 opcode) {
    const bool destination_is_spsr = Common::IsBitSet<22>(opcode);
    const bool operand_is_immediate = Common::IsBitSet<25>(opcode);
    const std::unsigned_integral auto source_operand = Common::GetBitRange<11, 0>(opcode);

    if (operand_is_immediate) {
        const std::unsigned_integral auto rotate_amount = Common::GetBitRange<11, 8>(source_operand);
        const std::unsigned_integral auto immediate = Common::GetBitRange<7, 0>(source_operand);

        if constexpr (flag_bits_only) {
            if (destination_is_spsr) {
                SetSPSR(GetSPSR() & ~0xFFFFFF00);
                SetSPSR(GetSPSR() | (Shift_RotateRight(immediate, rotate_amount << 1, false) & 0xFFFFFF00));
            } else {
                cpsr.raw &= ~0xFFFFFF00;
                cpsr.raw |= (Shift_RotateRight(immediate, rotate_amount << 1, false) & 0xFFFFFF00);
            }
        } else {
            if (destination_is_spsr) {
                SetSPSR(Shift_RotateRight(immediate, rotate_amount << 1, false));
            } else {
                cpsr.raw = Shift_RotateRight(immediate, rotate_amount << 1, false);
            }
        }
    } else {
        const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(source_operand);
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
