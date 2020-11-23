#include <bit>
#include <cstdarg>
#include <ranges>
#include "arm7.h"
#include "logging.h"

ARM7::ARM7(MMU& mmu, PPU& ppu)
    : mmu(mmu), ppu(ppu) {
    // Initialize the pipeline and registers
    pipeline.fill(0);
    gpr.fill(0);
    fiq_r.fill(0);
    svc_r.fill(0);
    abt_r.fill(0);
    irq_r.fill(0);
    und_r.fill(0);

    cpsr.flags.processor_mode = ProcessorMode::System;

    // taken from no$gba
    SetRegister(0, 0x00000CA5);
    SetSP(0x03007F00);
    SetLR(0x08000000);
    SetPC(0x08000000);
}

void ARM7::Step(bool dump_registers) {
    // FIXME: put the pipeline stuff in its own function
    if (cpsr.flags.thumb_mode) {
        const u16 opcode = pipeline[0];

        pipeline[0] = pipeline[1];
        gpr[15] += 2;
        pipeline[1] = mmu.Read16(gpr[15]);

        const Thumb_Instructions instr = DecodeThumbInstruction(opcode);
        if (dump_registers)
            DumpRegisters();
        DisassembleThumbInstruction(instr, opcode);
        ExecuteThumbInstruction(instr, opcode);
    } else {
        const u32 opcode = pipeline[0];

        pipeline[0] = pipeline[1];
        gpr[15] += 4;
        pipeline[1] = mmu.Read32(gpr[15]);

        const ARM_Instructions instr = DecodeARMInstruction(opcode);
        if (dump_registers)
            DumpRegisters();
        DisassembleARMInstruction(instr, opcode);
        ExecuteARMInstruction(instr, opcode);
    }

    // TODO: actual timing
    ppu.AdvanceCycles(1);
}

ARM7::ARM_Instructions ARM7::DecodeARMInstruction(const u32 opcode) const {
    if ((opcode & 0x0F000000) == 0x0F000000) return ARM_Instructions::SoftwareInterrupt;
    if ((opcode & 0x0F000010) == 0x0E000000) return ARM_Instructions::CoprocessorDataOperation;
    if ((opcode & 0x0F000010) == 0x0E000010) return ARM_Instructions::CoprocessorRegisterTransfer;
    if ((opcode & 0x0E000000) == 0x0C000000) return ARM_Instructions::CoprocessorDataTransfer;
    if ((opcode & 0x0E000000) == 0x0A000000) return ARM_Instructions::Branch;
    if ((opcode & 0x0E000000) == 0x08000000) return ARM_Instructions::BlockDataTransfer;
    if ((opcode & 0x0E000010) == 0x06000010) return ARM_Instructions::Undefined;
    if ((opcode & 0x0C000000) == 0x04000000) return ARM_Instructions::SingleDataTransfer;
    if ((opcode & 0x0E400090) == 0x00400090) return ARM_Instructions::HalfwordDataTransferImmediate;
    if ((opcode & 0x0E400F90) == 0x00000090) return ARM_Instructions::HalfwordDataTransferRegister;
    if ((opcode & 0x0FFFFFF0) == 0x012FFF10) return ARM_Instructions::BranchAndExchange;
    if ((opcode & 0x0FB00FF0) == 0x01000090) return ARM_Instructions::SingleDataSwap;
    if ((opcode & 0x0F8000F0) == 0x00800090) return ARM_Instructions::MultiplyLong;
    if ((opcode & 0x0FC000F0) == 0x00000090) return ARM_Instructions::Multiply;
    if ((opcode & 0x0C000000) == 0x00000000) return ARM_Instructions::DataProcessing;

    // The opcode did not meet any of the above conditions.
    return ARM_Instructions::Unknown;
}

void ARM7::ExecuteARMInstruction(const ARM_Instructions instr, const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    if (!CheckConditionCode(cond)) {
        return;
    }

    switch (instr) {
        case ARM_Instructions::DataProcessing:
            ARM_DataProcessing(opcode);
            break;
        case ARM_Instructions::Multiply:
            ARM_Multiply(opcode);
            break;
        case ARM_Instructions::MultiplyLong:
            ARM_MultiplyLong(opcode);
            break;
        case ARM_Instructions::BranchAndExchange:
            ARM_BranchAndExchange(opcode);
            break;
        case ARM_Instructions::HalfwordDataTransferRegister:
            ARM_HalfwordDataTransferRegister(opcode);
            break;
        case ARM_Instructions::HalfwordDataTransferImmediate:
            ARM_HalfwordDataTransferImmediate(opcode);
            break;
        case ARM_Instructions::SingleDataTransfer:
            ARM_SingleDataTransfer(opcode);
            break;
        case ARM_Instructions::BlockDataTransfer:
            ARM_BlockDataTransfer(opcode);
            break;
        case ARM_Instructions::Branch:
            ARM_Branch(opcode);
            break;

        case ARM_Instructions::Unknown:
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented ARM instruction (%u)", static_cast<u8>(instr));
    }
}

ARM7::Thumb_Instructions ARM7::DecodeThumbInstruction(const u16 opcode) const {
    if ((opcode & 0xF000) == 0xF000) return Thumb_Instructions::LongBranchWithLink;
    if ((opcode & 0xF800) == 0xE000) return Thumb_Instructions::UnconditionalBranch;
    if ((opcode & 0xFF00) == 0xDF00) return Thumb_Instructions::SoftwareInterrupt;
    if ((opcode & 0xF000) == 0xD000) return Thumb_Instructions::ConditionalBranch;
    if ((opcode & 0xF000) == 0xC000) return Thumb_Instructions::MultipleLoadStore;
    if ((opcode & 0xF600) == 0xB400) return Thumb_Instructions::PushPopRegisters;
    if ((opcode & 0xFF00) == 0xB000) return Thumb_Instructions::AddOffsetToStackPointer;
    if ((opcode & 0xF000) == 0xA000) return Thumb_Instructions::LoadAddress;
    if ((opcode & 0xF000) == 0x9000) return Thumb_Instructions::SPRelativeLoadStore;
    if ((opcode & 0xF000) == 0x8000) return Thumb_Instructions::LoadStoreHalfword;
    if ((opcode & 0xE000) == 0x6000) return Thumb_Instructions::LoadStoreWithImmediateOffset;
    if ((opcode & 0xF200) == 0x5200) return Thumb_Instructions::LoadStoreSignExtendedByteHalfword;
    if ((opcode & 0xF200) == 0x5000) return Thumb_Instructions::LoadStoreWithRegisterOffset;
    if ((opcode & 0xF800) == 0x4800) return Thumb_Instructions::PCRelativeLoad;
    if ((opcode & 0xFC00) == 0x4400) return Thumb_Instructions::HiRegisterOperationsBranchExchange;
    if ((opcode & 0xFC00) == 0x4000) return Thumb_Instructions::ALUOperations;
    if ((opcode & 0xE000) == 0x2000) return Thumb_Instructions::MoveCompareAddSubtractImmediate;
    if ((opcode & 0xF800) == 0x1800) return Thumb_Instructions::AddSubtract;
    if ((opcode & 0xE000) == 0x0000) return Thumb_Instructions::MoveShiftedRegister;

    // The opcode did not meet any of the above conditions.
    return Thumb_Instructions::Unknown;
}

void ARM7::ExecuteThumbInstruction(const Thumb_Instructions instr, const u16 opcode) {
    switch (instr) {
        case Thumb_Instructions::MoveShiftedRegister:
            Thumb_MoveShiftedRegister(opcode);
            break;
        case Thumb_Instructions::AddSubtract:
            Thumb_AddSubtract(opcode);
            break;
        case Thumb_Instructions::MoveCompareAddSubtractImmediate:
            Thumb_MoveCompareAddSubtractImmediate(opcode);
            break;
        case Thumb_Instructions::ALUOperations:
            Thumb_ALUOperations(opcode);
            break;
        case Thumb_Instructions::HiRegisterOperationsBranchExchange:
            Thumb_HiRegisterOperationsBranchExchange(opcode);
            break;
        case Thumb_Instructions::PCRelativeLoad:
            Thumb_PCRelativeLoad(opcode);
            break;
        case Thumb_Instructions::LoadStoreWithRegisterOffset:
            Thumb_LoadStoreWithRegisterOffset(opcode);
            break;
        case Thumb_Instructions::LoadStoreSignExtendedByteHalfword:
            Thumb_LoadStoreSignExtendedByteHalfword(opcode);
            break;
        case Thumb_Instructions::LoadStoreWithImmediateOffset:
            Thumb_LoadStoreWithImmediateOffset(opcode);
            break;
        case Thumb_Instructions::LoadStoreHalfword:
            Thumb_LoadStoreHalfword(opcode);
            break;
        case Thumb_Instructions::LoadAddress:
            Thumb_LoadAddress(opcode);
            break;
        case Thumb_Instructions::AddOffsetToStackPointer:
            Thumb_AddOffsetToStackPointer(opcode);
            break;
        case Thumb_Instructions::PushPopRegisters:
            Thumb_PushPopRegisters(opcode);
            break;
        case Thumb_Instructions::MultipleLoadStore:
            Thumb_MultipleLoadStore(opcode);
            break;
        case Thumb_Instructions::ConditionalBranch:
            Thumb_ConditionalBranch(opcode);
            break;
        case Thumb_Instructions::UnconditionalBranch:
            Thumb_UnconditionalBranch(opcode);
            break;
        case Thumb_Instructions::LongBranchWithLink:
            Thumb_LongBranchWithLink(opcode);
            break;

        case Thumb_Instructions::Unknown:
        default:
            UNIMPLEMENTED_MSG("interpreter: unknown THUMB instruction (%u)", static_cast<u8>(instr));
    }
}

void ARM7::DumpRegisters() {
    printf("r0: %08X r1: %08X r2: %08X r3: %08X\n", GetRegister(0), GetRegister(1), GetRegister(2), GetRegister(3));
    printf("r4: %08X r5: %08X r6: %08X r7: %08X\n", GetRegister(4), GetRegister(5), GetRegister(6), GetRegister(7));
    printf("r8: %08X r9: %08X r10:%08X r11:%08X\n", GetRegister(8), GetRegister(9), GetRegister(10), GetRegister(11));
    printf("r12:%08X sp: %08X lr: %08X pc: %08X\n", GetRegister(12), GetSP(), GetLR(), cpsr.flags.thumb_mode ? GetPC() - 4 : GetPC() - 8);
    printf("cpsr:%08X\n", cpsr.raw);
}

void ARM7::FillPipeline() {
    if (cpsr.flags.thumb_mode) {
        pipeline[0] = mmu.Read16(gpr[15]);
        // LDEBUG("r15=%08X p0=%04X", gpr[15], pipeline[0]);
        gpr[15] += 2;
        pipeline[1] = mmu.Read16(gpr[15]);
        // LDEBUG("r15=%08X p1=%04X", gpr[15], pipeline[1]);
    } else {
        pipeline[0] = mmu.Read32(gpr[15]);
        // LDEBUG("r15=%08X p0=%08X", gpr[15], pipeline[0]);
        gpr[15] += 4;
        pipeline[1] = mmu.Read32(gpr[15]);
        // LDEBUG("r15=%08X p1=%08X", gpr[15], pipeline[1]);
    }
}

u32 ARM7::Shift(const u8 operand_to_shift, const u8 shift_type, const u8 shift_amount) {
    if (!shift_amount) { // shift by 0 digits
        return operand_to_shift;
    }

    switch (shift_type) {
        default:
            UNIMPLEMENTED_MSG("unimplemented shift type %u", shift_type);
    }
}

u32 ARM7::Shift_RotateRight(const u32 operand_to_rotate, const u8 rotate_amount) {
    if (!rotate_amount) { // rotate by 0 digits
        return operand_to_rotate;
    }

    // 32 is the number of bits in the number we are rotating
    // const u8 r = rotate_amount % 32;
    // return (operand_to_rotate >> rotate_amount) | (operand_to_rotate << (32 - r));

    return std::rotr(operand_to_rotate, rotate_amount);
}

bool ARM7::CheckConditionCode(const u8 cond) {
    switch (cond) {
        case 0x0:
            return cpsr.flags.zero;
        case 0x1:
            return !cpsr.flags.zero;
        case 0x2:
            return cpsr.flags.carry;
        case 0x3:
            return !cpsr.flags.carry;
        case 0x4:
            return cpsr.flags.negative;
        case 0x5:
            return !cpsr.flags.negative;
        case 0x6:
            return cpsr.flags.overflow;
        case 0x7:
            return !cpsr.flags.overflow;
        case 0x8:
            return (cpsr.flags.carry && !cpsr.flags.zero);
        case 0x9:
            return (!cpsr.flags.carry || cpsr.flags.zero);
        case 0xA:
            return (cpsr.flags.negative == cpsr.flags.overflow);
        case 0xB:
            return (cpsr.flags.negative != cpsr.flags.overflow);
        case 0xC:
            return (!cpsr.flags.zero && (cpsr.flags.negative == cpsr.flags.overflow));
        case 0xD:
            return (cpsr.flags.zero || (cpsr.flags.negative != cpsr.flags.overflow));
        case 0xE:
            return true;
        default: // cond is 0xF
            UNREACHABLE();
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

void ARM7::ARM_BranchAndExchange(const u32 opcode) {
    const u8 rn = opcode & 0xF;

    // If bit 0 of Rn is set, we switch to THUMB mode. Else, we switch to ARM mode.
    cpsr.flags.thumb_mode = GetRegister(rn) & 0b1;
    LDEBUG("thumb: %u", cpsr.flags.thumb_mode);

    SetPC(GetRegister(rn) & ~0b1);
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

void ARM7::Thumb_PCRelativeLoad(const u16 opcode) {
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 imm = opcode & 0xFF;

    SetRegister(rd, mmu.Read32((GetPC() + (imm << 2)) & ~0x3));
}

void ARM7::Thumb_MoveShiftedRegister(const u16 opcode) {
    const u8 op = (opcode >> 11) & 0x3;
    switch (op) {
        case 0:
            Thumb_LSL(opcode);
            break;
        case 1:
            Thumb_LSR(opcode);
            break;
        case 2:
            Thumb_ASR(opcode);
            break;
        default:
            UNREACHABLE();
    }
}

void ARM7::Thumb_LSL(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    const u32 source = GetRegister(rs);

    SetRegister(rd, source << offset);

    cpsr.flags.negative = GetRegister(rd) & (1 << 31);
    cpsr.flags.zero = (GetRegister(rd) == 0);
    cpsr.flags.carry = source & (1 << (32 - offset));
    LWARN("make LSL's carry flag more accurate");
}

void ARM7::Thumb_LSR(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    const u32 source = GetRegister(rs);

    cpsr.flags.carry = (GetRegister(rd) >> (source - 1)) & 0b1;

    SetRegister(rd, source >> offset);

    cpsr.flags.negative = GetRegister(rd) & (1 << 31);
    cpsr.flags.zero = (GetRegister(rd) == 0);
    LWARN("verify LSR's flags");
}

void ARM7::Thumb_ASR(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    const u32 source = GetRegister(rs);

    SetRegister(rd, source >> offset);

    cpsr.flags.negative = GetRegister(rd) & (1 << 31);
    cpsr.flags.zero = (GetRegister(rd) == 0);
    cpsr.flags.carry = source & (1 << (offset - 1));
    LWARN("verify ASR's carry flag");
}

void ARM7::Thumb_ConditionalBranch(const u16 opcode) {
    const u8 cond = (opcode >> 8) & 0xF;
    const s8 offset = opcode & 0xFF;

    bool condition = false;
    switch (cond) {
        case 0x0:
            condition = cpsr.flags.zero;
            break;
        case 0x1:
            condition = !cpsr.flags.zero;
            break;
        case 0x2:
            condition = cpsr.flags.carry;
            break;
        case 0x3:
            condition = !cpsr.flags.carry;
            break;
        case 0x4:
            condition = cpsr.flags.negative;
            break;
        case 0x5:
            condition = !cpsr.flags.negative;
            break;
        case 0xB:
            condition = (cpsr.flags.negative != cpsr.flags.overflow);
            break;
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb conditional branch condition 0x%X", cond);
    }

    if (condition) {
        SetPC(GetPC() + (offset << 1));
    }
}

void ARM7::Thumb_MoveCompareAddSubtractImmediate(const u16 opcode) {
    const u8 op = (opcode >> 11) & 0x3;
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 offset = opcode & 0xFF;

    switch (op) {
        case 0x0:
            SetRegister(rd, offset);
            break;
        case 0x1:
            cpsr.flags.zero = (GetRegister(rd) == offset);
            return;
        case 0x2:
            SetRegister(rd, GetRegister(rd) + offset);
            break;
        case 0x3:
            SetRegister(rd, GetRegister(rd) - offset);
            break;

        default:
            UNREACHABLE_MSG("interpreter: illegal thumb MCASI op 0x%X", op);
    }

    cpsr.flags.negative = (GetRegister(rd) & (1 << 31));
    cpsr.flags.zero = (GetRegister(rd) == 0);
}

void ARM7::Thumb_LongBranchWithLink(const u16 opcode) {
    const u16 next_opcode = mmu.Read16(GetPC() - 2);
    const u16 offset = opcode & 0x7FF;
    const u16 next_offset = next_opcode & 0x7FF;

    u32 lr = GetLR();
    u32 pc = GetPC();

    lr = pc + (offset << 12);

    pc = lr + (next_offset << 1);
    lr = GetPC() | 0b1;

    SetLR(lr);

    // FIXME: something is going wrong with this calculation.
    // For example, sometimes we should be jumping to 0x08000210,
    // but we're actually jumping to 0x08800210, which is way out
    // of bounds, in terms of cartridge size. No good.
    if (pc & (1 << 23)) {
        LERROR("thumb long branch with link reset bit 23 hack");
        pc &= ~(1 << 23);
    }

    SetPC(pc);
}

void ARM7::Thumb_AddSubtract(const u16 opcode) {
    const bool operand_is_immediate = (opcode >> 10) & 0b1;
    const bool subtracting = (opcode >> 9) & 0b1;
    const u8 rn_or_immediate = (opcode >> 6) & 0x7;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    if (subtracting) {
        if (operand_is_immediate) {
            cpsr.flags.carry = GetRegister(rs) < rn_or_immediate;
            SetRegister(rd, GetRegister(rs) - rn_or_immediate);
        } else {
            cpsr.flags.carry = GetRegister(rs) < GetRegister(rn_or_immediate);
            SetRegister(rd, GetRegister(rs) - GetRegister(rn_or_immediate));
        }
    } else {
        LWARN("need to implement the carry flag for thumb add");
        if (operand_is_immediate) {
            SetRegister(rd, GetRegister(rs) + rn_or_immediate);
        } else {
            SetRegister(rd, GetRegister(rs) + GetRegister(rn_or_immediate));
        }
    }
}

void ARM7::Thumb_ALUOperations(const u16 opcode) {
    const u8 op = (opcode >> 6) & 0xF;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    switch (op) {
        case 0x0: // AND
            SetRegister(rd, GetRegister(rd) & GetRegister(rs));
            break;
        case 0x1: // EOR
            SetRegister(rd, GetRegister(rd) ^ GetRegister(rs));
            break;
        case 0x2: // LSL
            if (GetRegister(rs) == 32) {
                cpsr.flags.carry = (GetRegister(rd) & 0b1);
            } else if (GetRegister(rs) > 32) {
                cpsr.flags.carry = false;
            } else {
                cpsr.flags.carry = ((GetRegister(rd) >> (32 - GetRegister(rs))) & 0b1);
            }

            SetRegister(rd, static_cast<u64>(GetRegister(rd)) << GetRegister(rs));
            break;
        case 0x3: // LSR
            if (GetRegister(rs) == 32) {
                cpsr.flags.carry = (GetRegister(rd) >> 31);
            } else if (GetRegister(rs) > 32) {
                cpsr.flags.carry = false;
            } else {
                cpsr.flags.carry = ((GetRegister(rd) >> (GetRegister(rs) - 1)) & 0b1);
            }

            SetRegister(rd, GetRegister(rd) >> GetRegister(rs));
            break;
        case 0x7: // ROR
            SetRegister(rd, Shift_RotateRight(GetRegister(rd), GetRegister(rs)));
            break;
        case 0x8: { // TST
            u32 result = GetRegister(rd) & GetRegister(rs);
            cpsr.flags.zero = (result == 0);
            cpsr.flags.negative = (result & (1 << 31));
            return;
        }
        case 0x9: // NEG
            SetRegister(rd, -GetRegister(rs));
            break;
        case 0xA: { // CMP
            u32 result = GetRegister(rd) - GetRegister(rs);
            cpsr.flags.zero = (result == 0);
            return;
        }
        case 0xB: { // CMN
            u32 result = GetRegister(rd) + GetRegister(rs);
            cpsr.flags.zero = (result == 0);
            return;
        }
        case 0xC: // ORR
            SetRegister(rd, GetRegister(rd) | GetRegister(rs));
            break;
        case 0xD: // MUL
            SetRegister(rd, GetRegister(rd) * GetRegister(rs));
            break;
        case 0xE: // BIC
            SetRegister(rd, GetRegister(rd) & ~GetRegister(rs));
            break;
        case 0xF: // MVN
            SetRegister(rd, ~GetRegister(rs));
            break;
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb alu op 0x%X", op);
    }

    cpsr.flags.negative = (GetRegister(rd) & (1 << 31));
    cpsr.flags.zero = (GetRegister(rd) == 0);
}

void ARM7::Thumb_MultipleLoadStore(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const u8 rb = (opcode >> 8) & 0x7;
    const u8 rlist = opcode & 0xFF;

    // Go through `rlist`'s 8 bits, and write down which bits are set. This tells
    // us which registers we need to load/store.
    // If bit 0 is set, that corresponds to R0. If bit 1 is set, that corresponds
    // to R1, and so on.
    std::vector<u8> set_bits;
    for (u8 i = 0; i < 8; i++) {
        if (rlist & (1 << i)) {
            set_bits.push_back(i);
        }
    }

    if (set_bits.empty()) {
        return;
    }

    if (load_from_memory) {
        for (u8 i : set_bits) {
            SetRegister(i, mmu.Read32(GetRegister(rb)));
            SetRegister(rb, GetRegister(rb) + 4);
        }
    } else {
        for (u8 reg : set_bits) {
            mmu.Write32(GetRegister(rb), GetRegister(reg));
            SetRegister(rb, GetRegister(rb) + 4);
        }
    }
}

void ARM7::Thumb_HiRegisterOperationsBranchExchange(const u16 opcode) {
    const u8 op = (opcode >> 8) & 0x3;
    const bool h1 = (opcode >> 7) & 0b1;
    const bool h2 = (opcode >> 6) & 0b1;
    u8 rs_hs = (opcode >> 3) & 0x7;
    u8 rd_hd = opcode & 0x7;

    ASSERT(!(op == 0x3 && h1));

    if (h1) {
        rd_hd += 8;
    }

    if (h2) {
        rs_hs += 8;
    }

    switch (op) {
        case 0x0:
            SetRegister(rd_hd, GetRegister(rd_hd) + GetRegister(rs_hs));
            // If we're setting R15 through here, we need to halfword align it
            // and refill the pipeline.
            if (rd_hd == 15) {
                SetPC(GetPC() & ~0b1);
            }
            break;
        case 0x2:
            SetRegister(rd_hd, GetRegister(rs_hs));
            // If we're setting R15 through here, we need to halfword align it
            // and refill the pipeline.
            if (rd_hd == 15) {
                SetPC(GetPC() & ~0b1);
            }
            break;
        case 0x3:
            cpsr.flags.thumb_mode = GetRegister(rs_hs) & 0b1;
            SetPC(GetRegister(rs_hs) & ~0b1);
            return;
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb hi reg op 0x%X", op);
    }

    cpsr.flags.negative = (GetRegister(rd_hd) & (1 << 31));
    cpsr.flags.zero = (GetRegister(rd_hd) == 0);
}

void ARM7::Thumb_LoadStoreWithImmediateOffset(const u16 opcode) {
    const bool transfer_byte = (opcode >> 12) & 0b1;
    const bool load_from_memory = (opcode >> 11) & 0b1;

    if (transfer_byte) {
        if (load_from_memory) {
            Thumb_LoadByteWithImmediateOffset(opcode);
        } else {
            Thumb_StoreByteWithImmediateOffset(opcode);
        }
    } else {
        if (load_from_memory) {
            Thumb_LoadWordWithImmediateOffset(opcode);
        } else {
            Thumb_StoreWordWithImmediateOffset(opcode);
        }
    }
}

void ARM7::Thumb_StoreByteWithImmediateOffset(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    mmu.Write8(GetRegister(rd), GetRegister(rb) + offset);
}

void ARM7::Thumb_LoadByteWithImmediateOffset(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    SetRegister(rd, mmu.Read8(GetRegister(rb) + offset));
}

void ARM7::Thumb_StoreWordWithImmediateOffset(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    mmu.Write32(GetRegister(rd), GetRegister(rb) + offset);
}

void ARM7::Thumb_LoadWordWithImmediateOffset(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    SetRegister(rd, mmu.Read32(GetRegister(rb) + offset));
}

void ARM7::Thumb_PushPopRegisters(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const bool store_lr_load_pc = (opcode >> 8) & 0b1;
    const u8 rlist = opcode & 0xFF;

    // Go through `rlist`'s 8 bits, and write down which bits are set. This tells
    // us which registers we need to load/store.
    // If bit 0 is set, that corresponds to R0. If bit 1 is set, that corresponds
    // to R1, and so on.
    std::vector<u8> set_bits;
    for (u8 i = 0; i < 8; i++) {
        if (rlist & (1 << i)) {
            set_bits.push_back(i);
        }
    }

    if (set_bits.empty()) {
        if (!store_lr_load_pc) {
            return;
        }

        if (load_from_memory) {
            SetPC(mmu.Read32(GetSP()) & ~0b1);
            SetSP(GetSP() + 4);
        } else {
            mmu.Write32(GetSP(), GetLR());
            SetSP(GetSP() - 4);
        }

        return;
    }

    if (load_from_memory) {
        for (u8 reg : set_bits) {
            SetRegister(reg, mmu.Read32(GetSP()));
            SetSP(GetSP() + 4);
        }

        if (store_lr_load_pc) {
            SetPC(mmu.Read32(GetSP()) & ~0b1);
            SetSP(GetSP() + 4);
        }
    } else {
        if (store_lr_load_pc) {
            SetSP(GetSP() - 4);
            mmu.Write32(GetSP(), GetLR());
        }

        for (u8 reg : set_bits | std::views::reverse) {
            SetSP(GetSP() - 4);
            mmu.Write32(GetSP(), GetRegister(reg));
        }
    }
}

void ARM7::Thumb_UnconditionalBranch(const u16 opcode) {
    s16 offset = (opcode & 0x7FF) << 1;

    // Sign-extend to 16 bits
    offset <<= 4;
    offset >>= 4;

    SetPC(GetPC() + offset);
}

void ARM7::Thumb_LoadAddress(const u16 opcode) {
    const bool load_from_sp = (opcode >> 11) & 0b1;
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 imm = opcode & 0xFF;

    SetRegister(rd, (imm << 2) + (load_from_sp ? GetSP() : GetPC()));
}

void ARM7::Thumb_AddOffsetToStackPointer(const u16 opcode) {
    const bool offset_is_negative = (opcode >> 7) & 0b1;
    const u8 imm = opcode & 0x7F;

    if (offset_is_negative) {
        SetSP(GetSP() - imm);
    } else {
        SetSP(GetSP() + imm);
    }
}

void ARM7::Thumb_LoadStoreHalfword(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const u8 imm = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    if (load_from_memory) {
        SetRegister(rd, mmu.Read16(GetRegister(rb) + imm));
    } else {
        mmu.Write16(GetRegister(rb) + imm, static_cast<u16>(GetRegister(rd)));
    }
}

void ARM7::Thumb_LoadStoreWithRegisterOffset(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const bool transfer_byte = (opcode >> 10) & 0b1;
    const u8 ro = (opcode >> 6) & 0x7;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    if (load_from_memory) {
        if (transfer_byte) {
            SetRegister(rd, mmu.Read8(GetRegister(rb) + GetRegister(ro)));
        } else {
            SetRegister(rd, mmu.Read32(GetRegister(rb) + GetRegister(ro)));
        }
    } else {
        if (transfer_byte) {
            mmu.Write8(GetRegister(rb) + GetRegister(ro), GetRegister(rd) & 0xFF);
        } else {
            mmu.Write32(GetRegister(rb) + GetRegister(ro), GetRegister(rd));
        }
    }
}

void ARM7::Thumb_LoadStoreSignExtendedByteHalfword(const u16 opcode) {
    const bool h_flag = (opcode >> 11) & 0b1;
    const bool sign_extend = (opcode >> 10) & 0b1;
    const u8 ro = (opcode >> 6) & 0x7;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    if (sign_extend) {
        if (h_flag) {
            SetRegister(rd, mmu.Read16(GetRegister(rb) + GetRegister(ro)));

            // Set bits 31-16 of Rd to what's in bit 15.
            if (!(GetRegister(rd) & (1 << 15))) {
                return;
            }

            for (u8 i = 16; i < 31; i++) {
                SetRegister(rd, GetRegister(rd) | (1 << i));
            }
        } else {
            SetRegister(rd, mmu.Read8(GetRegister(rb) + GetRegister(ro)));

            // Set bits 31-8 of Rd to what's in bit 15.
            if (!(GetRegister(rd) & (1 << 7))) {
                return;
            }

            for (u8 i = 8; i < 31; i++) {
                SetRegister(rd, GetRegister(rd) | (1 << i));
            }
        }
    } else {
        if (h_flag) {
            SetRegister(rd, mmu.Read16(GetRegister(rb) + GetRegister(ro)));
        } else {
            UNIMPLEMENTED_MSG("unimplemented thumb store (not sign-extended) halfword");
        }
    }
}
