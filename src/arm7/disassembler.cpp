#include <fmt/core.h>
#include "arm7.h"
#include "logging.h"

void ARM7::DisassembleARMInstruction(const ARM_Instructions instr, const u32 opcode) {
    switch (instr) {
        case ARM_Instructions::DataProcessing:
            ARM_DisassembleDataProcessing(opcode);
            break;
        case ARM_Instructions::Multiply:
            ARM_DisassembleMultiply(opcode);
            break;
        case ARM_Instructions::MultiplyLong:
            ARM_DisassembleMultiplyLong(opcode);
            break;
        case ARM_Instructions::SingleDataSwap:
            ARM_DisassembleSingleDataSwap(opcode);
            break;
        case ARM_Instructions::BranchAndExchange:
            ARM_DisassembleBranchAndExchange(opcode);
            break;
        case ARM_Instructions::HalfwordDataTransferRegister:
            ARM_DisassembleHalfwordDataTransferRegister(opcode);
            break;
        case ARM_Instructions::HalfwordDataTransferImmediate:
            ARM_DisassembleHalfwordDataTransferImmediate(opcode);
            break;
        case ARM_Instructions::SingleDataTransfer:
            ARM_DisassembleSingleDataTransfer(opcode);
            break;
        case ARM_Instructions::BlockDataTransfer:
            ARM_DisassembleBlockDataTransfer(opcode);
            break;
        case ARM_Instructions::Branch:
            ARM_DisassembleBranch(opcode);
            break;
        case ARM_Instructions::SoftwareInterrupt:
            ARM_DisassembleSoftwareInterrupt(opcode);
            break;

        case ARM_Instructions::Unknown:
        default:
            UNIMPLEMENTED_MSG("disassembler: unhandled ARM instruction {} (opcode: {:08X}, pc: {:08X})", instr, opcode, GetPC() - 8);
    }
}

void ARM7::DisassembleThumbInstruction(const Thumb_Instructions instr, const u16 opcode) {
    switch (instr) {
        case Thumb_Instructions::MoveShiftedRegister:
            Thumb_DisassembleMoveShiftedRegister(opcode);
            break;
        case Thumb_Instructions::AddSubtract:
            Thumb_DisassembleAddSubtract(opcode);
            break;
        case Thumb_Instructions::MoveCompareAddSubtractImmediate:
            Thumb_DisassembleMoveCompareAddSubtractImmediate(opcode);
            break;
        case Thumb_Instructions::ALUOperations:
            Thumb_DisassembleALUOperations(opcode);
            break;
        case Thumb_Instructions::HiRegisterOperationsBranchExchange:
            Thumb_DisassembleHiRegisterOperationsBranchExchange(opcode);
            break;
        case Thumb_Instructions::PCRelativeLoad:
            Thumb_DisassemblePCRelativeLoad(opcode);
            break;
        case Thumb_Instructions::LoadStoreWithRegisterOffset:
            Thumb_DisassembleLoadStoreWithRegisterOffset(opcode);
            break;
        case Thumb_Instructions::LoadStoreSignExtendedByteHalfword:
            Thumb_DisassembleLoadStoreSignExtendedByteHalfword(opcode);
            break;
        case Thumb_Instructions::LoadStoreWithImmediateOffset:
            Thumb_DisassembleLoadStoreWithImmediateOffset(opcode);
            break;
        case Thumb_Instructions::LoadStoreHalfword:
            Thumb_DisassembleLoadStoreHalfword(opcode);
            break;
        case Thumb_Instructions::LoadAddress:
            Thumb_DisassembleLoadAddress(opcode);
            break;
        case Thumb_Instructions::AddOffsetToStackPointer:
            Thumb_DisassembleAddOffsetToStackPointer(opcode);
            break;
        case Thumb_Instructions::PushPopRegisters:
            Thumb_DisassemblePushPopRegisters(opcode);
            break;
        case Thumb_Instructions::MultipleLoadStore:
            Thumb_DisassembleMultipleLoadStore(opcode);
            break;
        case Thumb_Instructions::ConditionalBranch:
            Thumb_DisassembleConditionalBranch(opcode);
            break;
        case Thumb_Instructions::UnconditionalBranch:
            Thumb_DisassembleUnconditionalBranch(opcode);
            break;
        case Thumb_Instructions::LongBranchWithLink:
            Thumb_DisassembleLongBranchWithLink(opcode);
            break;

        case Thumb_Instructions::Unknown:
        default:
            UNIMPLEMENTED_MSG("disassembler: unhandled THUMB instruction {} (opcode: {:04X}, pc: {:08X})", instr, opcode, GetPC() - 4);
    }
}

std::string ARM7::GetRegisterAsString(u8 reg) const {
    ASSERT(reg <= 15);

    if (reg == 13) {
        return "SP";
    }

    if (reg == 14) {
        return "LR";
    }

    if (reg == 15) {
        return "PC";
    }

    return std::to_string(reg);
}

std::string ARM7::GetConditionCode(const u8 cond) {
    ASSERT(cond != 0xF);

    if (cond == 0xE) {
        return "";
    }

    constexpr std::array<const char*, 14> codes = {
        "EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC", "HI", "LS", "GE", "LT", "GT", "LE"
    };

    return std::string(codes[cond]);
}

void ARM7::ARM_DisassembleDataProcessing(const u32 opcode) {
    if ((opcode & 0x0FBF0FFF) == 0x010F0000) {
        ARM_DisassembleMRS(opcode);
        return;
    }

    if ((opcode & 0x0FBFFFF0) == 0x0129F000) {
        ARM_DisassembleMSR(opcode, false);
        return;
    }

    if ((opcode & 0x0DBFF000) == 0x0128F000) {
        ARM_DisassembleMSR(opcode, true);
        return;
    }

    const u8 cond = (opcode >> 28) & 0xF;
    const bool op2_is_immediate = (opcode >> 25) & 0b1;
    const u8 op = (opcode >> 21) & 0xF;
    const bool set_condition_codes = (opcode >> 20) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u16 op2 = opcode & 0xFFF;
    std::string disasm;

    constexpr std::array<const char*, 16> mnemonics = {
        "AND", "EOR", "SUB", "RSB", "ADD", "ADC", "SBC", "RSC", "TST", "TEQ", "CMP", "CMN", "ORR", "MOV", "BIC", "MVN"
    };

    disasm += std::string(mnemonics.at(op));
    disasm += GetConditionCode(cond);

    switch (op) {
        case 0xD: // MOV
        case 0xF: // MVN
            if (set_condition_codes) {
                disasm += "S";
            }

            disasm += fmt::format(" R{}, ", rd);

            if (op2_is_immediate) {
                u8 rotate_amount = (op2 >> 8) & 0xF;
                u8 imm = op2 & 0xFF;
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

                disasm += fmt::format("#0x{:08X}", rotated_operand);
            } else {
                u8 shift = (op2 >> 4) & 0xFF;
                u8 rm = op2 & 0xF;
                if ((shift & 0b1) == 0) {
                    u16 shift_amount = (shift >> 3) & 0x1F;
                    u8 shift_type = (shift >> 1) & 0b11;

                    if (!shift_amount) {
                        disasm += fmt::format("R{}", rm);
                    } else {
                        disasm += fmt::format("R{}, ", rm);

                        constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                        disasm += std::string(shift_types[shift_type]);

                        disasm += fmt::format(" #{}", shift_amount);
                    }
                } else if ((shift & 0b1001) == 0b0001) {
                    u8 rs = (shift >> 4) & 0xF;
                    u8 shift_type = (shift >> 1) & 0b11;

                    disasm += fmt::format("R{}, ", rm);

                    constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                    disasm += std::string(shift_types[shift_type]);

                    disasm += fmt::format(" R{}", rs);
                } else {
                    ASSERT(false);
                }
            }

            break;
        case 0x8 ... 0xB: // TST, TEQ, CMP, CMN
            disasm += fmt::format(" R{}, ", rn);

            if (op2_is_immediate) {
                u8 rotate_amount = (op2 >> 8) & 0xF;
                u8 imm = op2 & 0xFF;
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

                disasm += fmt::format("#0x{:08X}", rotated_operand);
            } else {
                u8 shift = (op2 >> 4) & 0xFF;
                if ((shift & 0b1) == 0) {
                    u16 shift_amount = (shift >> 3) & 0x1F;
                    u8 shift_type = (shift >> 1) & 0b11;
                    u8 rm = op2 & 0xF;

                    if (!shift_amount) {
                        disasm += fmt::format("R{}", rm);
                    } else {
                        disasm += fmt::format("R{}, ", rm);

                        constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                        disasm += std::string(shift_types[shift_type]);

                        disasm += fmt::format(" #{}", shift_amount);
                    }
                } else if ((shift & 0b1001) == 0b0001) {
                    UNIMPLEMENTED();
                } else {
                    ASSERT(false);
                }
            }

            break;
        case 0x0 ... 0x7: // AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC
        case 0xC: // ORR
        case 0xE: // BIC
            if (set_condition_codes) {
                disasm += "S";
            }

            disasm += fmt::format(" R{}, R{}, ", rd, rn);

            if (op2_is_immediate) {
                u8 rotate_amount = (op2 >> 8) & 0xF;
                u8 imm = op2 & 0xFF;
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount * 2);

                disasm += fmt::format("#0x{:08X}", rotated_operand);
            } else {
                u8 shift = (op2 >> 4) & 0xFF;
                if ((shift & 0b1) == 0) {
                    u16 shift_amount = (shift >> 3) & 0x1F;
                    u8 shift_type = (shift >> 1) & 0b11;
                    u8 rm = op2 & 0xF;

                    if (!shift_amount) {
                        disasm += fmt::format("R{}", rm);
                    } else {
                        disasm += fmt::format("R{}, ", rm);

                        constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                        disasm += std::string(shift_types[shift_type]);

                        disasm += fmt::format(" #{}", shift_amount);
                    }
                } else if ((shift & 0b1001) == 0b0001) {
                    UNIMPLEMENTED();
                } else {
                    ASSERT(false);
                }
            }
            break;
        default:
            UNIMPLEMENTED_MSG("disassembler: unimplemented data processing op 0x{:X}", op);
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleMRS(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const bool source_is_spsr = (opcode >> 22) & 0b1;
    const u8 rd = (opcode >> 12) & 0xF;
    std::string disasm;

    disasm += fmt::format("MRS{} ", GetConditionCode(cond));

    disasm += fmt::format("R{}, ", rd);

    if (source_is_spsr) {
        disasm += "SPSR";
    } else {
        disasm += "CPSR";
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleMSR(const u32 opcode, const bool flag_bits_only) {
    const u8 cond = (opcode >> 28) & 0xF;
    const bool destination_is_spsr = (opcode >> 22) & 0b1;
    std::string disasm;

    disasm += fmt::format("MSR{} ", GetConditionCode(cond));

    if (flag_bits_only) {
        const bool operand_is_immediate = (opcode >> 25) & 0b1;
        const u16 source_operand = opcode & 0xFFF;

        if (destination_is_spsr) {
            disasm += "SPSR_flg, ";
        } else {
            disasm += "CPSR_flg, ";
        }

        if (operand_is_immediate) {
            const u8 rotate_amount = (source_operand >> 8) & 0xF;
            const u8 immediate = source_operand & 0xFF;

            disasm += fmt::format("#0x{:08X}", Shift_RotateRight(immediate, rotate_amount * 2));
        } else {
            const u8 rm = source_operand & 0xF;
            disasm += fmt::format("R{}", rm);
        }
    } else {
        const u8 rm = opcode & 0xF;

        disasm += fmt::format("{}_all, R{}", destination_is_spsr ? "SPSR" : "CPSR", rm);
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleMultiply(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const bool accumulate = (opcode >> 21) & 0b1;
    const bool set_condition_codes = (opcode >> 20) & 0b1;
    const u8 rd = (opcode >> 16) & 0xF;
    const u8 rn = (opcode >> 12) & 0xF;
    const u8 rs = (opcode >> 8) & 0xF;
    const u8 rm = opcode & 0xF;
    std::string disasm;

    if (accumulate) {
        disasm += "MLA";
    } else {
        disasm += "MUL";
    }

    disasm += GetConditionCode(cond);

    if (set_condition_codes) {
        disasm += "S";
    }

    if (accumulate) {
        disasm += fmt::format(" R{}, R{}, R{}, R{}", rd, rm, rs, rn);
    } else {
        disasm += fmt::format(" R{}, R{}, R{}", rd, rm, rs);
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleMultiplyLong(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const bool sign = (opcode >> 22) & 0b1;
    const bool accumulate = (opcode >> 21) & 0b1;
    const bool set_condition_codes = (opcode >> 20) & 0b1;
    const u8 rdhi = (opcode >> 16) & 0xF;
    const u8 rdlo = (opcode >> 12) & 0xF;
    const u8 rs = (opcode >> 8) & 0xF;
    const u8 rm = opcode & 0xF;
    std::string disasm;

    if (sign) {
        disasm += "S";
    } else {
        disasm += "U";
    }

    if (accumulate) {
        disasm += "MLAL";
    } else {
        disasm += "MULL";
    }

    disasm += GetConditionCode(cond);

    if (set_condition_codes) {
        disasm += "S";
    }

    disasm += fmt::format(" R{}, R{}, R{}, R{}", rdlo, rdhi, rm, rs);

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleSingleDataSwap(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const bool swap_byte = (opcode >> 22) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u8 rm = opcode & 0xF;
    std::string disasm;

    disasm += fmt::format("SWP{}", GetConditionCode(cond));

    if (swap_byte) {
        disasm += "B";
    }

    disasm += fmt::format(" R{}, R{}, [R{}]", rd, rm, rn);

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleBranchAndExchange(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const u8 rn = opcode & 0xF;
    std::string disasm;

    disasm += fmt::format("BX{} R{}", GetConditionCode(cond), rn);

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleHalfwordDataTransferRegister(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const bool load_from_memory = (opcode >> 20) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const bool sign = (opcode >> 6) & 0b1;
    const bool halfword = (opcode >> 5) & 0b1;
    const u8 rm = opcode & 0xF;
    std::string disasm;

    if (!sign && !halfword) {
        ARM_DisassembleSingleDataSwap(opcode);
        return;
    }

    if (load_from_memory) {
        disasm += "LDR";
    } else {
        disasm += "STR";
    }

    disasm += GetConditionCode(cond);

    if (sign) {
        disasm += "S";
    }

    if (halfword) {
        disasm += "H";
    } else {
        disasm += "B";
    }

    disasm += fmt::format(" R{}, ", rd);

    if (pre_indexing) {
        disasm += fmt::format("[R{}, ", rn);
        if (!add_offset_to_base) {
            disasm += "-";
        }
        disasm += fmt::format("R{}]", rm);

        if (write_back) {
            disasm += "!";
        }
    } else {
        disasm += fmt::format("[R{}], ", rn);
        if (!add_offset_to_base) {
            disasm += "-";
        }
        disasm += fmt::format("R{}", rm);
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleHalfwordDataTransferImmediate(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const bool load_from_memory = (opcode >> 20) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u8 offset_high = (opcode >> 8) & 0xF;
    const bool sign = (opcode >> 6) & 0b1;
    const bool halfword = (opcode >> 5) & 0b1;
    const u8 offset_low = opcode & 0xF;
    const u8 offset = (offset_high << 8) | offset_low;
    std::string disasm;

    if (!sign && !halfword) {
        ARM_DisassembleSingleDataSwap(opcode);
        return;
    }

    if (load_from_memory) {
        disasm += "LDR";
    } else {
        disasm += "STR";
    }

    disasm += GetConditionCode(cond);

    if (sign) {
        disasm += "S";
    }

    if (halfword) {
        disasm += "H";
    } else {
        disasm += "B";
    }

    disasm += fmt::format(" R{}, ", rd);

    if (pre_indexing) {
        if (offset == 0) {
            disasm += fmt::format("[R{}]", rn);
        } else {
            disasm += fmt::format("[R{}, #", rn);
            if (!add_offset_to_base) {
                disasm += "-";
            }
            disasm += fmt::format("0x{:02X}]", offset);

            if (write_back) {
                disasm += "!";
            }
        }
    } else {
        disasm += fmt::format("[R{}], #", rn);
        if (!add_offset_to_base) {
            disasm += "-";
        }
        disasm += fmt::format("0x{:02X}", offset);
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleSingleDataTransfer(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const bool offset_is_register = (opcode >> 25) & 0b1;
    const bool add_before_transfer = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool transfer_byte = (opcode >> 22) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const bool load_from_memory = (opcode >> 20) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u8 rd = (opcode >> 12) & 0xF;
    const u16 offset = opcode & 0xFFF;
    std::string disasm;

    if (load_from_memory) {
        disasm += "LDR";
    } else {
        disasm += "STR";
    }

    disasm += GetConditionCode(cond);

    if (transfer_byte) {
        disasm += "B";
    }

    // TODO: {T} - if T is present the W bit will be set in a post-indexed instruction, forcing
    //             non-privileged mode for the transfer cycle. T is not allowed when 
    //             pre-indexed addressing mode is specified or implied.

    disasm += fmt::format(" R{}, ", rd);

    if (add_before_transfer) {
        if (offset == 0) {
            disasm += fmt::format("[R{}]", rn);
        } else {
            if (offset_is_register) {
                u8 shift = (offset >> 4) & 0xFF;
                u8 rm = offset & 0xF;

                disasm += fmt::format("[R{}, ", rn);
                if (!add_offset_to_base) {
                    disasm += "-";
                }
                disasm += fmt::format("R{}", rm);

                if ((shift & 0b1) == 0) {
                    u16 shift_amount = (shift >> 3) & 0x1F;
                    u8 shift_type = (shift >> 1) & 0b11;

                    if (!shift_amount) {
                        disasm += "]";
                    } else {
                        constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                        disasm += fmt::format(", {} #{}]", shift_types[shift_type], shift_amount);
                    }
                } else if ((shift & 0b1001) == 0b0001) {
                    UNIMPLEMENTED();
                } else {
                    ASSERT(false);
                }
            } else {
                disasm += fmt::format("[R{}, #0x{:08X}]", rn, offset);
            }
        }
    } else {
        if (offset_is_register) {
            u8 shift = (offset >> 4) & 0xFF;
            u8 rm = offset & 0xF;

            disasm += fmt::format("[R{}], ", rn);
            if (!add_offset_to_base) {
                disasm += "-";
            }
            disasm += fmt::format("R{}", rm);

            if ((shift & 0b1) == 0) {
                u16 shift_amount = (shift >> 3) & 0x1F;
                u8 shift_type = (shift >> 1) & 0b11;

                if (shift_amount) {
                    constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                    disasm += fmt::format(", {} #{}", shift_types[shift_type], shift_amount);
                }
            } else if ((shift & 0b1001) == 0b0001) {
                UNIMPLEMENTED();
            } else {
                ASSERT(false);
            }
        } else {
            disasm += fmt::format("[R{}], #0x{:08X}", rn, offset);
        }
    }

    if (write_back) {
        disasm += "!";
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleBlockDataTransfer(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const bool pre_indexing = (opcode >> 24) & 0b1;
    const bool add_offset_to_base = (opcode >> 23) & 0b1;
    const bool load_psr = (opcode >> 22) & 0b1;
    const bool write_back = (opcode >> 21) & 0b1;
    const bool load_from_memory = (opcode >> 20) & 0b1;
    const u8 rn = (opcode >> 16) & 0xF;
    const u16 rlist = opcode & 0xFFFF;
    std::string disasm;

    if (!load_from_memory && rn == 13 && !add_offset_to_base && pre_indexing && write_back) {
        disasm += fmt::format("PUSH{} {{", GetConditionCode(cond));
    } else if (load_from_memory && rn == 13 && add_offset_to_base && !pre_indexing && write_back) {
        disasm += fmt::format("POP{} {{", GetConditionCode(cond));
    } else {
        if (load_from_memory) {
            disasm += "LDM";
        } else {
            disasm += "STM";
        }

        disasm += GetConditionCode(cond);

        if (add_offset_to_base) {
            disasm += "I";
        } else {
            disasm += "D";
        }

        if (pre_indexing) {
            disasm += "B";
        } else {
            disasm += "A";
        }

        disasm += fmt::format(" R{}", rn);

        if (write_back) {
            disasm += "!";
        }

        disasm += ", {";
    }

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
        disasm += "}";

        if (load_psr) {
            disasm += "^";
        }

        return;
    }

    for (u8 i = 0; i < set_bits.size(); i++) {
        disasm += fmt::format("R{}", set_bits[i]);
        if (i != set_bits.size() - 1) {
            disasm += ", ";
        }
    }

    disasm += "}";

    if (load_psr) {
        disasm += "^";
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleBranch(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const bool link = (opcode >> 24) & 0b1;
    s32 offset = (opcode & 0xFFFFFF) << 2;
    std::string disasm;

    disasm += "B";
    if (link) {
        disasm += "L";
    }

    disasm += GetConditionCode(cond);

    // sign extend
    offset <<= 6;
    offset >>= 6;

    disasm += fmt::format(" 0x{:08X}", GetPC() + offset);

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleSoftwareInterrupt(const u32 opcode) {
    const u8 cond = (opcode >> 28) & 0xF;
    const u32 comment = opcode & 0xFFFFFF;

    std::string disasm = fmt::format("SWI{} 0x{:X}", GetConditionCode(cond), comment);
    LTRACE_ARM("{}", disasm);
}

void ARM7::Thumb_DisassembleMoveShiftedRegister(const u16 opcode) {
    const u8 op = (opcode >> 11) & 0x3;
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    std::string disasm;

    const std::array<std::string, 3> mnemonics = { "LSL", "LSR", "ASR" };
    disasm += mnemonics.at(op);

    disasm += fmt::format(" R{}, R{}, #{}", rd, rs, offset);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleAddSubtract(const u16 opcode) {
    const bool operand_is_immediate = (opcode >> 10) & 0b1;
    const bool subtracting = (opcode >> 9) & 0b1;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    std::string disasm;

    if (subtracting) {
        disasm += "SUB";
    } else {
        disasm += "ADD";
    }

    disasm += fmt::format(" R{}, R{}, ", rd, rs);

    if (operand_is_immediate) {
        const u8 immediate = (opcode >> 6) & 0x7;
        disasm += fmt::format("#0x{:02X}", immediate);
    } else {
        const u8 rn = (opcode >> 6) & 0x7;
        disasm += fmt::format("R{}", rn);
    }

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleMoveCompareAddSubtractImmediate(const u16 opcode) {
    const u8 op = (opcode >> 11) & 0x3;
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 offset = opcode & 0xFF;
    std::string disasm;

    constexpr std::array<const char*, 4> mnemonics = { "MOV", "CMP", "ADD", "SUB" };
    disasm += std::string(mnemonics.at(op));

    disasm += fmt::format(" R{}, #0x{:02X}", rd, offset);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleALUOperations(const u16 opcode) {
    const u8 op = (opcode >> 6) & 0xF;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    std::string disasm;

    const std::array<std::string, 16> mnemonics = {
        "AND", "EOR", "LSL", "LSR", "ASR", "ADC", "SBC", "ROR", "TST", "NEG", "CMP", "CMN", "ORR", "MUL", "BIC", "MVN"
    };
    disasm += mnemonics.at(op);

    disasm += fmt::format(" R{}, R{}", rd, rs);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleHiRegisterOperationsBranchExchange(const u16 opcode) {
    const u8 op = (opcode >> 8) & 0x3;
    const bool h1 = (opcode >> 7) & 0b1;
    const bool h2 = (opcode >> 6) & 0b1;
    const u8 rs_hs = (opcode >> 3) & 0x7;
    const u8 rd_hd = opcode & 0x7;
    std::string disasm;

    const std::array<std::string, 4> mnemonics = { "ADD", "CMP", "MOV", "BX" };
    disasm += mnemonics.at(op);

    ASSERT(!(op == 0x3 && h1));

    // BX
    if (op == 0x3) {
        disasm += fmt::format(" R{}", h2 ? rs_hs + 8 : rs_hs);
    } else {
        disasm += fmt::format(" R{}, ", h1 ? rd_hd + 8 : rd_hd);
        disasm += fmt::format("R{}", h2 ? rs_hs + 8 : rs_hs);
    }

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassemblePCRelativeLoad(const u16 opcode) {
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 imm = opcode & 0xFF;

    LTRACE_THUMB("LDR R{}, [R15, #0x{:08X}]", rd, imm);
}

void ARM7::Thumb_DisassembleLoadStoreWithRegisterOffset(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const bool transfer_byte = (opcode >> 10) & 0b1;
    const u8 ro = (opcode >> 6) & 0x7;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    std::string disasm;

    if (load_from_memory) {
        disasm += "LDR";
    } else {
        disasm += "STR";
    }

    if (transfer_byte) {
        disasm += "B";
    }

    disasm += fmt::format(" R{}, [R{}, R{}]", rd, rb, ro);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleLoadStoreSignExtendedByteHalfword(const u16 opcode) {
    const bool h_flag = (opcode >> 11) & 0b1;
    const bool sign_extend = (opcode >> 10) & 0b1;
    const u8 ro = (opcode >> 6) & 0x7;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    std::string disasm;

    if (sign_extend) {
        if (h_flag) {
            disasm += "LDSH";
        } else {
            disasm += "LDSB";
        }
    } else {
        if (h_flag) {
            disasm += "LDRH";
        } else {
            disasm += "STRH";
        }
    }

    disasm += fmt::format(" R{}, [R{}, R{}]", rd, rb, ro);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleLoadStoreWithImmediateOffset(const u16 opcode) {
    const bool transfer_byte = (opcode >> 12) & 0b1;
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    std::string disasm;

    if (load_from_memory) {
        disasm += "LDR";
    } else {
        disasm += "STR";
    }

    if (transfer_byte) {
        disasm += "B";
    }

    disasm += fmt::format(" R{}, [R{}, #0x{:02X}]", rd, rb, offset);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleLoadStoreHalfword(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const u8 imm = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    std::string disasm;

    if (load_from_memory) {
        disasm += "LDRH";
    } else {
        disasm += "STRH";
    }

    disasm += fmt::format(" R{}, [R{}, #0x{:X}]", rd, rb, imm);

    LTRACE_THUMB("{}", disasm);
}

// TODO: Thumb SP-relative load/store

void ARM7::Thumb_DisassembleLoadAddress(const u16 opcode) {
    const bool load_from_sp = (opcode >> 11) & 0b1;
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 imm = opcode & 0xFF;
    std::string disasm;

    disasm += fmt::format("ADD R{}, ", rd);

    if (load_from_sp) {
        disasm += "SP";
    } else {
        disasm += "PC";
    }

    disasm += fmt::format(", #0x{:X}", imm << 2);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleAddOffsetToStackPointer(const u16 opcode) {
    const bool offset_is_negative = (opcode >> 7) & 0b1;
    const u8 imm = opcode & 0x7F;
    std::string disasm;

    disasm += "ADD SP, #";
    if (offset_is_negative) {
        disasm += "-";
    }
    disasm += fmt::format("0x{:02X}", imm);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassemblePushPopRegisters(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const bool store_lr_load_pc = (opcode >> 8) & 0b1;
    const u8 rlist = opcode & 0xFF;
    std::string disasm;

    if (load_from_memory) {
        disasm += "POP {";
    } else {
        disasm += "PUSH {";
    }

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
            disasm += "}";
            LTRACE_THUMB("{}", disasm);
            return;
        }

        if (load_from_memory) {
            disasm += "PC}";
        } else {
            disasm += "LR}";
        }

        LTRACE_THUMB("{}", disasm);
        return;
    }

    for (u8 i = 0; i < set_bits.size(); i++) {
        disasm += fmt::format("R{}", set_bits[i]);
        if (i != set_bits.size() - 1) {
            disasm += ", ";
        }
    }

    if (store_lr_load_pc) {
        if (load_from_memory) {
            disasm += ", PC}";
        } else {
            disasm += ", LR}";
        }
    } else {
        disasm += "}";
    }

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleMultipleLoadStore(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const u8 rb = (opcode >> 8) & 0x7;
    const u8 rlist = opcode & 0xFF;
    std::string disasm;

    if (load_from_memory) {
        disasm += "LDMIA";
    } else {
        disasm += "STMIA";
    }

    disasm += fmt::format(" R{}!, {{", rb);

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

    for (u8 i = 0; i < set_bits.size(); i++) {
        disasm += fmt::format("R{}", set_bits[i]);
        if (i != set_bits.size() - 1) {
            disasm += ", ";
        }
    }

    disasm += "}";

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleConditionalBranch(const u16 opcode) {
    const u8 cond = (opcode >> 8) & 0xF;
    const s8 offset = opcode & 0xFF;
    std::string disasm;

    ASSERT(cond != 0xF);

    constexpr std::array<const char*, 14> mnemonics = {
        "BEQ", "BNE", "BCS", "BCC", "BMI", "BPL", "BVS", "BVC", "BHI", "BLS", "BGE", "BLT", "BGT", "BLE" 
    };
    disasm += std::string(mnemonics.at(cond));

    disasm += fmt::format(" 0x{:08X}", GetPC() + (offset * 2));

    LTRACE_THUMB("{}", disasm);
}

// TODO: Thumb-mode software interrupt

void ARM7::Thumb_DisassembleUnconditionalBranch(const u16 opcode) {
    s16 offset = (opcode & 0x7FF) << 1;

    // Sign-extend to 16 bits
    offset <<= 4;
    offset >>= 4;

    LTRACE_THUMB("B 0x{:08X}", GetPC() + offset);
}

void ARM7::Thumb_DisassembleLongBranchWithLink(const u16 opcode) {
    const u16 next_opcode = mmu.Read16(GetPC() - 2);
    // Used for LTRACE_DOUBLETHUMB
    const u32 double_opcode = (static_cast<u32>(opcode) << 16) | next_opcode;
    const u16 offset_high = opcode & 0x7FF;
    const u16 offset_low = next_opcode & 0x7FF;

    u32 lr = GetLR();
    u32 pc = GetPC();

    const bool first_instruction = ((opcode >> 11) & 0b1) == 0b0;
    ASSERT(first_instruction);

    lr = pc + (offset_high << 12);

    const bool second_instruction = ((next_opcode >> 11) & 0b1) == 0b1;
    ASSERT(second_instruction);

    pc = lr + (offset_low << 1);
    lr = GetPC() | 0b1;

    LDEBUG("LR={:08X}", lr);
    LTRACE_DOUBLETHUMB("BX 0x{:08X}", pc);
}
