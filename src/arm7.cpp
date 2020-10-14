#include <bit>
#include <cstdarg>
#include <ranges>
#include "arm7.h"
#include "logging.h"

ARM7::ARM7(MMU& mmu)
    : mmu(mmu) {
    // Initialize the pipeline and registers
    pipeline.fill(0);
    r.fill(0);

    r[0] = 0x00000CA5;
    // stack pointer
    r[13] = 0x03007F00;
    // link register
    r[14] = 0x08000000;
    // program counter
    r[15] = 0x08000000;

    // TODO: initialize CPSR to 0 once we start loading the BIOS.
    cpsr.raw = 0x0000001F;

    FillPipeline();
}

void ARM7::Step(bool disassemble) {
    // FIXME: put the piprline stuff in its own function
    if (cpsr.flags.thumb_mode) {
        const u16 opcode = pipeline[0];

        pipeline[0] = pipeline[1];
        r[15] += 2;
        pipeline[1] = mmu.Read16(r[15]);

        const Thumb_Instructions instr = DecodeThumbInstruction(opcode);
        if (disassemble)
            DisassembleThumbInstruction(instr, opcode);
        ExecuteThumbInstruction(instr, opcode);
    } else {
        const u32 opcode = pipeline[0];

        pipeline[0] = pipeline[1];
        r[15] += 4;
        pipeline[1] = mmu.Read32(r[15]);

        const ARM_Instructions instr = DecodeARMInstruction(opcode);
        if (disassemble)
            DisassembleARMInstruction(instr, opcode);
        ExecuteARMInstruction(instr, opcode);
    }
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
        case ARM_Instructions::BranchAndExchange:
            ARM_BranchAndExchange(opcode);
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
        case Thumb_Instructions::LoadStoreWithImmediateOffset:
            Thumb_LoadStoreWithImmediateOffset(opcode);
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
        case Thumb_Instructions::LongBranchWithLink:
            Thumb_LongBranchWithLink(opcode);
            break;

        case Thumb_Instructions::Unknown:
        default:
            UNIMPLEMENTED_MSG("interpreter: unknown THUMB instruction (%u)", static_cast<u8>(instr));
    }
}

void ARM7::DumpRegisters(const bool thumb) {
    printf("r0: %08X r1: %08X r2: %08X r3: %08X\n", r[0], r[1], r[2], r[3]);
    printf("r4: %08X r5: %08X r6: %08X r7: %08X\n", r[4], r[5], r[6], r[7]);
    printf("r8: %08X r9: %08X r10:%08X r11:%08X\n", r[8], r[9], r[10], r[11]);
    printf("r12:%08X r13:%08X lr: %08X pc: %08X\n", r[12], r[13], r[14], thumb ? r[15] - 4 : r[15] - 8);
    printf("cpsr:%08X\n", cpsr.raw);
}

void ARM7::FillPipeline() {
    if (cpsr.flags.thumb_mode) {
        pipeline[0] = mmu.Read16(r[15]);
        // LDEBUG("r15=%08X p0=%04X", r[15], pipeline[0]);
        r[15] += 2;
        pipeline[1] = mmu.Read16(r[15]);
        // LDEBUG("r15=%08X p1=%04X", r[15], pipeline[1]);
    } else {
        pipeline[0] = mmu.Read32(r[15]);
        // LDEBUG("r15=%08X p0=%08X", r[15], pipeline[0]);
        r[15] += 4;
        pipeline[1] = mmu.Read32(r[15]);
        // LDEBUG("r15=%08X p1=%08X", r[15], pipeline[1]);
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
    ASSERT(cond != 0xF);

    if (cond == 0xE) {
        return true;
    }

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
        default:
            UNREACHABLE();
    }
}

void ARM7::ARM_Branch(const u32 opcode) {
    const bool link = (opcode >> 24) & 0b1;
    s32 offset = (opcode & 0xFFFFFF) << 2;

    offset <<= 6;
    offset >>= 6;

    if (link) {
        r[14] = r[15] - 4;
    }

    r[15] += offset;
    FillPipeline();
}

void ARM7::ARM_DataProcessing(const u32 opcode) {
    if ((opcode & 0x0FBF0FFF) == 0x010F0000) {
        UNIMPLEMENTED_MSG("interpreter: implement MRS");
    }

    if ((opcode & 0x0FBFFFF0) == 0x0129F000) {
        ARM_MSR(opcode);
        return;
    }

    if ((opcode & 0x0DBFF000) == 0x0128F000) {
        UNIMPLEMENTED_MSG("interpreter: implement MSR");
    }

    const bool op2_is_immediate = (opcode >> 25) & 0b1;
    const u8 op = (opcode >> 21) & 0xF;
    const bool set_condition_codes = (opcode >> 20) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u16 op2 = opcode & 0xFFF;

    if (op != 0xD) {
        ASSERT(rd != 15);
    }

    switch (op) {
        case 0x0:
            if (op2_is_immediate) {
                u8 rotate_amount = (op2 >> 8) & 0xF;
                u8 imm = op2 & 0xFF;
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

                r[rd] = r[rn] & rotated_operand;
            } else {
                UNIMPLEMENTED_MSG("interpreter: unimplemented AND with register as op2");
            }

            break;
        case 0x2:
            if (op2_is_immediate) {
                u8 rotate_amount = (op2 >> 8) & 0xF;
                u8 imm = op2 & 0xFF;
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

                r[rd] = r[rn] - rotated_operand;
            } else {
                UNIMPLEMENTED_MSG("interpreter: unimplemented SUB with register as op2");
            }

            break;
        case 0x4:
            if (op2_is_immediate) {
                u8 rotate_amount = (op2 >> 8) & 0xF;
                u8 imm = op2 & 0xFF;
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

                r[rd] = r[rn] + rotated_operand;
            } else {
                u8 shift = (op2 >> 4) & 0xFF;
                if ((shift & 0b1) == 0) {
                    u8 shift_amount = (shift >> 3) & 0x1F;
                    u8 shift_type = (shift >> 1) & 0b11;
                    u8 rm = op2 & 0xF;

                    if (!shift_amount) {
                        r[rd] = r[rn] + r[rm];
                        if (rd == 15) {
                            FillPipeline();
                        }

                        break;
                    }

                    switch (shift_type) {
                        case 0b00:
                            r[rd] = r[rn] + (r[rm] << shift_amount);
                            break;
                        default:
                            UNIMPLEMENTED_MSG("unimplemented add shift type 0x%X", shift_type);
                    }
                } else if ((shift & 0b1001) == 0b0001) {
                    UNIMPLEMENTED();
                } else {
                    ASSERT(false);
                }
            }

            break;
        case 0xA:
            if (op2_is_immediate) {
                u8 rotate_amount = (op2 >> 8) & 0xF;
                u8 imm = op2 & 0xFF;
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

                u32 result = r[rn] - rotated_operand;
                cpsr.flags.negative = (result & (1 << 31));
                cpsr.flags.carry = (result < r[rn]);
                cpsr.flags.zero = (result == 0);
            } else {
                UNIMPLEMENTED_MSG("interpreter: unimplemented CMP with register as op2");
            }

            break;
        case 0xC:
            if (op2_is_immediate) {
                u8 rotate_amount = (op2 >> 8) & 0xF;
                u8 imm = op2 & 0xFF;
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

                r[rd] = r[rn] | rotated_operand;
            } else {
                u8 shift = (op2 >> 4) & 0xFF;
                if ((shift & 0b1) == 0) {
                    u8 shift_amount = (shift >> 3) & 0x1F;
                    u8 shift_type = (shift >> 1) & 0b11;
                    u8 rm = op2 & 0xF;

                    if (!shift_amount) {
                        r[rd] = r[rm];
                        if (rd == 15) {
                            FillPipeline();
                        }

                        break;
                    }

                    switch (shift_type) {
                        case 0b00:
                            r[rd] = r[rn] | (r[rm] << shift_amount);
                            break;
                        case 0b01:
                            r[rd] = r[rn] | (r[rm] >> shift_amount);
                            break;
                        default:
                            UNIMPLEMENTED_MSG("unimplemented mov shift type 0x%X", shift_type);
                    }
                } else if ((shift & 0b1001) == 0b0001) {
                    UNIMPLEMENTED();
                } else {
                    ASSERT(false);
                }
            }

            break;
        case 0xD:
            if (op2_is_immediate) {
                u8 rotate_amount = (op2 >> 8) & 0xF;
                u8 imm = op2 & 0xFF;
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

                r[rd] = rotated_operand;
                ASSERT(rd != 15);
            } else {
                u8 shift = (op2 >> 4) & 0xFF;
                if ((shift & 0b1) == 0) {
                    u8 shift_amount = (shift >> 3) & 0x1F;
                    u8 shift_type = (shift >> 1) & 0b11;
                    u8 rm = op2 & 0xF;

                    if (!shift_amount) {
                        r[rd] = r[rm];
                        if (rd == 15) {
                            FillPipeline();
                        }

                        break;
                    }

                    switch (shift_type) {
                        case 0b00:
                            r[rd] = r[rm] << shift_amount;
                            break;
                        case 0b01:
                            r[rd] = r[rm] >> shift_amount;
                            break;
                        default:
                            UNIMPLEMENTED_MSG("unimplemented mov shift type 0x%X", shift_type);
                    }
                } else if ((shift & 0b1001) == 0b0001) {
                    UNIMPLEMENTED();
                } else {
                    ASSERT(false);
                }
            }

            break;
        case 0xF:
            if (op2_is_immediate) {
                u8 rotate_amount = (op2 >> 8) & 0xF;
                u8 imm = op2 & 0xFF;
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

                r[rd] = ~rotated_operand;
            } else {
                u8 shift = (op2 >> 4) & 0xFF;
                if ((shift & 0b1) == 0) {
                    u8 shift_amount = (shift >> 3) & 0x1F;
                    u8 shift_type = (shift >> 1) & 0b11;
                    u8 rm = op2 & 0xF;

                    if (!shift_amount) {
                        r[rd] = ~r[rm];
                        if (rd == 15) {
                            FillPipeline();
                        }

                        break;
                    }

                    switch (shift_type) {
                        case 0b00:
                            r[rd] = ~(r[rm] << shift_amount);
                            break;
                        case 0b01:
                            r[rd] = ~(r[rm] >> shift_amount);
                            break;
                        default:
                            UNIMPLEMENTED_MSG("unimplemented mov shift type 0x%X", shift_type);
                    }
                } else if ((shift & 0b1001) == 0b0001) {
                    UNIMPLEMENTED();
                } else {
                    ASSERT(false);
                }
            }

            break;
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented data processing op 0x%X", op);
    }

    if (set_condition_codes) {
        cpsr.flags.negative = (r[rd] & (1 << 31));
        cpsr.flags.zero = (r[rd] == 0);
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
            UNIMPLEMENTED_MSG("interpreter: unimplemented store byte");
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
    u16 offset = 0;
    if (offset_is_register) {
        LERROR("interpreter: unimplemented load word with register offset");
    } else {
        offset = opcode & 0xFFF;
    }

    u32 address = r[rn];
    if (add_before_transfer) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        address &= ~0x3; // Word align
        r[rd] = mmu.Read32(address);

        if (write_back) {
            LERROR("interpreter: unimplemented load word write-back");
        }
    } else {
        UNIMPLEMENTED_MSG("interpreter: unimplemented load word post-index");
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
        LERROR("interpreter: unimplemented store word with register offset");
    } else {
        offset = opcode & 0xFFF;
    }

    u32 address = r[rn];
    if (add_before_transfer) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        address &= ~0x3; // Word align
        mmu.Write32(address, r[rd]);

        if (write_back) {
            LERROR("interpreter: unimplemented store word write-back");
        }
    } else {
        address &= ~0x3; // Word align
        mmu.Write32(address, r[rd]);

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        r[rn] = address;
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
        LERROR("interpreter: unimplemented load byte with register offset");
    } else {
        offset = opcode & 0xFFF;
    }

    u32 address = r[rn];
    if (add_before_transfer) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        r[rd] = mmu.Read8(address);

        if (write_back) {
            LERROR("interpreter: unimplemented load byte write-back");
        }
    } else {
        r[rd] = mmu.Read8(address);

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        r[rn] = address;
    }
}

void ARM7::ARM_MSR(const u32 opcode) {
    // TODO: from starbreeze:
    // "you might have to add some things to your msr later. (e.g. you can't change
    // control bits in User mode, there's a mask field that controls access to the
    // individual PSR fields that the ARM7TDMI reference doesn't tell you about, ...)"
    //
    // "msr won't be the only way to set a PSR. There's mode changes and PSR transfers
    // in block/single data transfers, CPU exceptions and some data processing instructions."
    LWARN("MSR was skipped... fix me!!!!");
}

void ARM7::ARM_BranchAndExchange(const u32 opcode) {
    const u8 rn = opcode & 0xF;

    // If bit 0 of Rn is set, we switch to THUMB mode. Else, we switch to ARM mode.
    cpsr.flags.thumb_mode = r[rn] & 0b1;
    LDEBUG("thumb: %u", cpsr.flags.thumb_mode);

    r[15] = r[rn] & ~0x1;
    FillPipeline();
}

void ARM7::ARM_HalfwordDataTransferImmediate(const u32 opcode) {
    const bool load_from_memory = (opcode >> 20) & 0b1;
    const bool sign = (opcode >> 6) & 0b1;
    const bool halfword = (opcode >> 5) & 0b1;

    const u8 conditions = (load_from_memory << 1) | halfword;
    switch (conditions) {
        case 0b01:
            ARM_StoreHalfword(opcode, sign);
            break;
        case 0b11:
            ARM_LoadHalfword(opcode, sign);
            break;
        default:
            UNIMPLEMENTED_MSG("unimplemented halfword data transfer immediate conditions 0x%X", conditions);
    }
}

void ARM7::ARM_StoreHalfword(const u32 opcode, const bool sign) {
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u8 offset_high = (opcode >> 8) & 0xF;
    const u8 offset_low = opcode & 0xF;
    const u8 offset = (offset_high << 8) | offset_low;

    u32 address = r[rn];
    u16 value = r[rd] & 0xFFFF;
    if (pre_indexing) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        mmu.Write16(address, sign ? static_cast<s16>(value) : value);

        if (write_back) {
            r[rn] = address;
        }
    } else {
        mmu.Write16(address, sign ? static_cast<s16>(value) : value);
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        r[rn] = address;
    }

    ASSERT_MSG(!write_back, "unimplemented store halfword write-back");
}

void ARM7::ARM_LoadHalfword(const u32 opcode, const bool sign) {
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u8 offset_high = (opcode >> 8) & 0xF;
    const u8 offset_low = opcode & 0xF;
    const u8 offset = (offset_high << 8) | offset_low;

    u32 address = r[rn];
    if (pre_indexing) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        if (sign) {
            r[rd] = static_cast<s16>(mmu.Read16(address));
        } else {
            r[rd] = mmu.Read16(address);
        }

        if (write_back) {
            r[rn] = address;
        }
    } else {
        if (sign) {
            r[rd] = static_cast<s16>(mmu.Read16(address));
        } else {
            r[rd] = mmu.Read16(address);
        }

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        r[rn] = address;
    }

    ASSERT_MSG(!write_back, "unimplemented store halfword write-back");
}

void ARM7::ARM_BlockDataTransfer(const u32 opcode) {
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool load_psr = (opcode >> 22) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const bool load_from_memory = (opcode >> 20) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u16 rlist = opcode & 0xFFFF;

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

    u32 address = r[rn];
    if (load_from_memory) {
        if (add_offset_to_base) {
            for (u8 reg : set_bits) {
                if (pre_indexing) {
                    address += 4;
                    r[reg] = mmu.Read32(address);
                } else {
                    r[reg] = mmu.Read32(address);
                    address += 4;
                }

                if (reg == 15) {
                    FillPipeline();
                }
            }
        } else {
            for (u8 reg : set_bits | std::views::reverse) {
                if (pre_indexing) {
                    address -= 4;
                    r[reg] = mmu.Read32(address);
                } else {
                    r[reg] = mmu.Read32(address);
                    address -= 4;
                }

                if (reg == 15) {
                    FillPipeline();
                }
            }
        }
    } else {
        if (add_offset_to_base) {
            for (u8 reg : set_bits) {
                if (pre_indexing) {
                    address += 4;
                    mmu.Write32(address, r[reg]);
                } else {
                    mmu.Write32(address, r[reg]);
                    address += 4;
                }
            }
        } else {
            for (u8 reg : set_bits | std::views::reverse) {
                if (pre_indexing) {
                    address -= 4;
                    mmu.Write32(address, r[reg]);
                } else {
                    mmu.Write32(address, r[reg]);
                    address -= 4;
                }
            }
        }
    }

    if (write_back) {
        r[rn] = address;
    }
}

void ARM7::ARM_Multiply(const u32 opcode) {
    const bool accumulate = (opcode >> 21) & 0b1;
    const bool set_condition_codes = (opcode >> 20) & 0b1;
    const u8 rd = (opcode >> 16) & 0xF;
    const u8 rn = (opcode >> 12) & 0xF;
    const u8 rs = (opcode >> 8) & 0xF;
    const u8 rm = opcode & 0xF;

    r[rd] = r[rm] * r[rs];

    if (accumulate) {
        r[rd] += r[rn];
    }

    if (set_condition_codes) {
        cpsr.flags.negative = (r[rd] & (1 << 31));
        cpsr.flags.zero = (r[rd] == 0);
    }
}

void ARM7::Thumb_PCRelativeLoad(const u16 opcode) {
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 imm = opcode & 0xFF;

    r[rd] = mmu.Read32((r[15] + (imm << 2)) & ~0x3);
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
    const u32 source = r[rs];

    r[rd] = source << offset;

    cpsr.flags.negative = r[rd] & (1 << 31);
    cpsr.flags.zero = (r[rd] == 0);
    cpsr.flags.carry = source & (1 << (32 - offset));
    LWARN("make LSL's carry flag more accurate");
}

void ARM7::Thumb_LSR(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    const u32 source = r[rs];

    r[rd] = source >> offset;

    cpsr.flags.negative = r[rd] & (1 << 31);
    cpsr.flags.zero = (r[rd] == 0);
    LWARN("verify LSR's flags");
}

void ARM7::Thumb_ASR(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    const u32 source = r[rs];

    r[rd] = source >> offset;

    cpsr.flags.negative = r[rd] & (1 << 31);
    cpsr.flags.zero = (r[rd] == 0);
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
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb conditional branch condition 0x%X", cond);
    }

    if (condition) {
        r[15] += (offset * 2);
        FillPipeline();
    }
}

void ARM7::Thumb_MoveCompareAddSubtractImmediate(const u16 opcode) {
    const u8 op = (opcode >> 11) & 0x3;
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 offset = opcode & 0xFF;

    switch (op) {
        case 0x0:
            r[rd] = offset;
            break;
        case 0x1:
            cpsr.flags.zero = (r[rd] == offset);
            return;
        case 0x2:
            r[rd] += offset;
            break;
        case 0x3:
            r[rd] -= offset;
            break;

        default:
            UNREACHABLE_MSG("interpreter: illegal thumb MCASI op 0x%X", op);
    }

    cpsr.flags.zero = (r[rd] == 0);
}

void ARM7::Thumb_LongBranchWithLink(const u16 opcode) {
    const u16 next_opcode = mmu.Read16(r[15] - 2);
    const u16 offset = opcode & 0x7FF;
    const u16 next_offset = next_opcode & 0x7FF;

    u32 lr = r[14];
    u32 pc = r[15];

    lr = pc + (offset << 12);

    pc = lr + (next_offset << 1);
    lr = r[15] | 0b1;

    r[14] = lr;
    r[15] = pc;

    // FIXME: something is going wrong with this calculation.
    // For example, sometimes we should be jumping to 0x08000210,
    // but we're actually jumping to 0x08800210, which is way out
    // of bounds, in terms of cartridge size. No good.
    if (r[15] & (1 << 23)) {
        LERROR("thumb long branch with link reset bit 23 hack");
        r[15] &= ~(1 << 23);
    }
    FillPipeline();
}

void ARM7::Thumb_AddSubtract(const u16 opcode) {
    const bool operand_is_immediate = (opcode >> 10) & 0b1;
    const bool subtracting = (opcode >> 9) & 0b1;
    const u8 rn_or_immediate = (opcode >> 6) & 0x7;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    if (subtracting) {
        if (operand_is_immediate) {
            cpsr.flags.carry = r[rs] < rn_or_immediate;
            r[rd] = r[rs] - rn_or_immediate;
        } else {
            cpsr.flags.carry = r[rs] < r[rn_or_immediate];
            r[rd] = r[rs] - r[rn_or_immediate];
        }
    } else {
        LWARN("need to implement the carry flag for thumb add");
        if (operand_is_immediate) {
            r[rd] = r[rs] + rn_or_immediate;
        } else {
            r[rd] = r[rs] + r[rn_or_immediate];
        }
    }
}

void ARM7::Thumb_ALUOperations(const u16 opcode) {
    const u8 op = (opcode >> 6) & 0xF;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    switch (op) {
        case 0xA:
            cpsr.flags.zero = (r[rd] == r[rs]);
            return;
        case 0xE:
            r[rd] &= ~r[rs];
            break;
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb alu op 0x%X", op);
    }

    cpsr.flags.zero = (r[rd] == 0);
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
            r[i] = mmu.Read32(r[rb]);
            r[rb] += 4;
        }
    } else {
        for (u8 reg : set_bits) {
            mmu.Write32(r[rb], r[reg]);
            r[rb] += 4;
        }
    }
}

void ARM7::Thumb_HiRegisterOperationsBranchExchange(const u16 opcode) {
    const u8 op = (opcode >> 8) & 0x3;
    const bool h1 = (opcode >> 7) & 0b1;
    const bool h2 = (opcode >> 6) & 0b1;
    const u8 rs_hs = (opcode >> 3) & 0x7;
    const u8 rd_hd = opcode & 0x7;

    ASSERT(!(op == 0x3 && h1));

    switch (op) {
        case 0x2:
            r[h1 ? rd_hd + 8 : rd_hd] = r[h2 ? rs_hs + 8 : rs_hs];
            break;
        case 0x3:
            if (h2) {
                cpsr.flags.thumb_mode = r[rs_hs + 8] & 0b1;
                r[15] = r[rs_hs + 8] & ~0x1;
            } else {
                cpsr.flags.thumb_mode = r[rs_hs] & 0b1;
                r[15] = r[rs_hs] & ~0x1;
            }
            FillPipeline();
            break;
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb hi reg op 0x%X", op);
    }
}

void ARM7::Thumb_LoadStoreWithImmediateOffset(const u16 opcode) {
    const bool transfer_byte = (opcode >> 12) & 0b1;
    const bool load_from_memory = (opcode >> 11) & 0b1;

    if (transfer_byte) {
        if (load_from_memory) {
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb load byte with immediate offset");
        } else {
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb store byte with immediate offset");
        }
    } else {
        if (load_from_memory) {
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb load word with immediate offset");
        } else {
            Thumb_StoreWordWithImmediateOffset(opcode);
        }
    }
}

void ARM7::Thumb_StoreWordWithImmediateOffset(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    mmu.Write32(r[rd], r[rb] + offset);
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

    ASSERT_MSG(!set_bits.empty(), "unimplemented pop/push with no registers");

    if (set_bits.empty()) {
        if (!store_lr_load_pc) {
            return;
        }

        if (load_from_memory) {
            UNIMPLEMENTED_MSG("unimplemented pop pc from stack with no registers");
        } else {
            mmu.Write32(r[13], r[14]);
            r[13] -= 4;
        }
    }

    if (load_from_memory) {
        if (store_lr_load_pc) {
            UNIMPLEMENTED_MSG("unimplemented pop pc from stack");
        }

        for (u8 reg : set_bits) {
            r[reg] = mmu.Read32(r[13]);
            r[13] += 4;
        }
    } else {
        if (store_lr_load_pc) {
            r[13] -= 4;
            mmu.Write32(r[13], r[14]);
        }

        for (u8 reg : set_bits | std::views::reverse) {
            r[13] -= 4;
            mmu.Write32(r[13], r[reg]);
        }
    }
}
