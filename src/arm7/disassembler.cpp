#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/reverse.hpp>
#include "common/bits.h"
#include "common/logging.h"
#include "arm7.h"

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
        case Thumb_Instructions::SPRelativeLoadStore:
            Thumb_DisassembleSPRelativeLoadStore(opcode);
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
        case Thumb_Instructions::SoftwareInterrupt:
            Thumb_DisassembleSoftwareInterrupt(opcode);
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

std::string ARM7::GetRegAsStr(const u8 reg) const {
    ASSERT(reg <= 15);

    switch (reg) {
        case 13:
            return "SP";
        case 14:
            return "LR";
        case 15:
            return "PC";
        default:
            return fmt::format("R{}", reg);
    }
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

    if ((opcode & 0x0DBFF000) == 0x0129F000) {
        ARM_DisassembleMSR<false>(opcode);
        return;
    }

    if ((opcode & 0x0DBFF000) == 0x0128F000) {
        ARM_DisassembleMSR<true>(opcode);
        return;
    }

    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool op2_is_immediate = Common::IsBitSet<25>(opcode);
    const std::unsigned_integral auto op = Common::GetBitRange<24, 21>(opcode);
    const bool set_condition_codes = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto op2 = Common::GetBitRange<11, 0>(opcode);
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

            disasm += fmt::format(" {}, ", GetRegAsStr(rd));

            if (op2_is_immediate) {
                const std::unsigned_integral auto rotate_amount = Common::GetBitRange<11, 8>(op2);
                const std::unsigned_integral auto imm = Common::GetBitRange<7, 0>(op2);
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount << 1, false);

                disasm += fmt::format("#0x{:08X}", rotated_operand);
            } else {
                const std::unsigned_integral auto shift = Common::GetBitRange<11, 4>(op2);
                const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(op2);
                if (!Common::IsBitSet<0>(shift)) {
                    std::unsigned_integral auto shift_amount = Common::GetBitRange<7, 3>(shift);
                    std::unsigned_integral auto shift_type = Common::GetBitRange<2, 1>(shift);

                    if (shift_amount == 0 && shift_type == static_cast<u8>(ShiftType::LSL)) {
                        disasm += GetRegAsStr(rm);
                    } else {
                        if (shift_amount == 0) {
                            if (shift_type == static_cast<u8>(ShiftType::ROR)) {
                                shift_type = static_cast<u8>(ShiftType::RRX);
                            } else {
                                shift_amount = 32;
                            }
                        }

                        disasm += fmt::format("{}, ", GetRegAsStr(rm));

                        constexpr std::array<const char*, 5> shift_types = { "LSL", "LSR", "ASR", "ROR", "RRX" };
                        disasm += std::string(shift_types[shift_type]);

                        if (shift_type != static_cast<u8>(ShiftType::RRX)) {
                            disasm += fmt::format(" #{}", shift_amount);
                        }
                    }
                } else if (!Common::IsBitSet<3>(shift) && Common::IsBitSet<0>(shift)) {
                    const std::unsigned_integral auto rs = Common::GetBitRange<7, 4>(shift);
                    const std::unsigned_integral auto shift_type = Common::GetBitRange<2, 1>(shift);

                    disasm += fmt::format("{}, ", GetRegAsStr(rm));

                    constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                    disasm += std::string(shift_types[shift_type]);

                    disasm += fmt::format(" {}", GetRegAsStr(rs));
                } else {
                    ASSERT(false);
                }
            }

            break;
        case 0x8: // TST
        case 0x9: // TEQ
        case 0xA: // CMP
        case 0xB: // CMN
            disasm += fmt::format(" {}, ", GetRegAsStr(rn));

            if (op2_is_immediate) {
                const std::unsigned_integral auto rotate_amount = Common::GetBitRange<11, 8>(op2);
                const std::unsigned_integral auto imm = Common::GetBitRange<7, 0>(op2);
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount << 1, false);

                disasm += fmt::format("#0x{:08X}", rotated_operand);
            } else {
                const std::unsigned_integral auto shift = Common::GetBitRange<11, 4>(op2);
                const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(op2);
                if (!Common::IsBitSet<0>(shift)) {
                    const std::unsigned_integral auto shift_amount = Common::GetBitRange<7, 3>(shift);
                    const std::unsigned_integral auto shift_type = Common::GetBitRange<2, 1>(shift);

                    if (!shift_amount) {
                        disasm += GetRegAsStr(rm);
                    } else {
                        disasm += fmt::format("{}, ", GetRegAsStr(rm));

                        constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                        disasm += std::string(shift_types[shift_type]);

                        disasm += fmt::format(" #{}", shift_amount);
                    }
                } else if (!Common::IsBitSet<3>(shift) && Common::IsBitSet<0>(shift)) {
                    const std::unsigned_integral auto rs = Common::GetBitRange<7, 4>(shift);
                    const std::unsigned_integral auto shift_type = Common::GetBitRange<2, 1>(shift);

                    disasm += fmt::format("{}, ", GetRegAsStr(rm));

                    constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                    disasm += std::string(shift_types[shift_type]);

                    disasm += fmt::format(" {}", GetRegAsStr(rs));
                } else {
                    ASSERT(false);
                }
            }

            break;
        case 0x0: // AND
        case 0x1: // EOR
        case 0x2: // SUB
        case 0x3: // RSB
        case 0x4: // ADD
        case 0x5: // ADC
        case 0x6: // SBC
        case 0x7: // RSC
        case 0xC: // ORR
        case 0xE: // BIC
            if (set_condition_codes) {
                disasm += "S";
            }

            disasm += fmt::format(" {}, {}, ", GetRegAsStr(rd), GetRegAsStr(rn));

            if (op2_is_immediate) {
                const std::unsigned_integral auto rotate_amount = Common::GetBitRange<11, 8>(op2);
                const std::unsigned_integral auto imm = Common::GetBitRange<7, 0>(op2);
                u32 rotated_operand = Shift_RotateRight(imm, rotate_amount << 1, false);

                disasm += fmt::format("#0x{:08X}", rotated_operand);
            } else {
                const std::unsigned_integral auto shift = Common::GetBitRange<11, 4>(op2);
                if (!Common::IsBitSet<0>(shift)) {
                    const std::unsigned_integral auto shift_amount = Common::GetBitRange<7, 3>(shift);
                    const std::unsigned_integral auto shift_type = Common::GetBitRange<2, 1>(shift);
                    const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(op2);

                    if (!shift_amount) {
                        disasm += GetRegAsStr(rm);
                    } else {
                        disasm += fmt::format("{}, ", GetRegAsStr(rm));

                        constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                        disasm += std::string(shift_types[shift_type]);

                        disasm += fmt::format(" #{}", shift_amount);
                    }
                } else if (!Common::IsBitSet<3>(shift) && Common::IsBitSet<0>(shift)) {
                    const std::unsigned_integral auto rs = Common::GetBitRange<7, 4>(shift);
                    const std::unsigned_integral auto shift_type = Common::GetBitRange<2, 1>(shift);
                    const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(op2);

                    disasm += fmt::format("{}, ", GetRegAsStr(rm));

                    constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                    disasm += std::string(shift_types[shift_type]);

                    disasm += fmt::format(" {}", GetRegAsStr(rs));
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
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool source_is_spsr = Common::IsBitSet<22>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    std::string disasm;

    disasm += fmt::format("MRS{} ", GetConditionCode(cond));

    disasm += fmt::format("{}, ", GetRegAsStr(rd));

    if (source_is_spsr) {
        disasm += "SPSR";
    } else {
        disasm += "CPSR";
    }

    LTRACE_ARM("{}", disasm);
}

template <bool flag_bits_only>
void ARM7::ARM_DisassembleMSR(const u32 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool destination_is_spsr = Common::IsBitSet<22>(opcode);
    std::string disasm;

    disasm += fmt::format("MSR{} ", GetConditionCode(cond));

    const bool operand_is_immediate = Common::IsBitSet<25>(opcode);
    const std::unsigned_integral auto source_operand = Common::GetBitRange<11, 0>(opcode);

    if (destination_is_spsr) {
        disasm += "SPSR_";
    } else {
        disasm += "CPSR_";
    }

    if constexpr (flag_bits_only) {
        disasm += "flg, ";
    } else {
        disasm += "all, ";
    }

    if (operand_is_immediate) {
        const std::unsigned_integral auto rotate_amount = Common::GetBitRange<11, 8>(opcode);
        const std::unsigned_integral auto immediate = Common::GetBitRange<7, 0>(opcode);

        disasm += fmt::format("#0x{:08X}", Shift_RotateRight(immediate, static_cast<u8>(rotate_amount << 1), false));
    } else {
        const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(source_operand);
        disasm += GetRegAsStr(rm);
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleMultiply(const u32 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool accumulate = Common::IsBitSet<21>(opcode);
    const bool set_condition_codes = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<11, 8>(opcode);
    const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(opcode);
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
        disasm += fmt::format(" {}, {}, {}, {}", GetRegAsStr(rd), GetRegAsStr(rm), GetRegAsStr(rs), GetRegAsStr(rn));
    } else {
        disasm += fmt::format(" {}, {}, {}", GetRegAsStr(rd), GetRegAsStr(rm), GetRegAsStr(rs));
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleMultiplyLong(const u32 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool sign = Common::IsBitSet<22>(opcode);
    const bool accumulate = Common::IsBitSet<21>(opcode);
    const bool set_condition_codes = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rdhi = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rdlo = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<11, 8>(opcode);
    const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(opcode);
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

    disasm += fmt::format(" {}, {}, {}, {}", GetRegAsStr(rdlo), GetRegAsStr(rdhi), GetRegAsStr(rm), GetRegAsStr(rs));

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleSingleDataSwap(const u32 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool swap_byte = Common::IsBitSet<22>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(opcode);
    std::string disasm;

    disasm += fmt::format("SWP{}", GetConditionCode(cond));

    if (swap_byte) {
        disasm += "B";
    }

    disasm += fmt::format(" {}, {}, [{}]", GetRegAsStr(rd), GetRegAsStr(rm), GetRegAsStr(rn));

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleBranchAndExchange(const u32 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<3, 0>(opcode);
    std::string disasm;

    disasm += fmt::format("BX{} {}", GetConditionCode(cond), GetRegAsStr(rn));

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleHalfwordDataTransferRegister(const u32 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool pre_indexing = Common::IsBitSet<24>(opcode);
    const bool add_offset_to_base = Common::IsBitSet<23>(opcode);
    const bool write_back = Common::IsBitSet<21>(opcode);
    const bool load_from_memory = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const bool sign = Common::IsBitSet<6>(opcode);
    const bool halfword = Common::IsBitSet<5>(opcode);
    const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(opcode);
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

    disasm += fmt::format(" {}, ", GetRegAsStr(rd));

    if (pre_indexing) {
        disasm += fmt::format("[{}, ", GetRegAsStr(rn));
        if (!add_offset_to_base) {
            disasm += "-";
        }
        disasm += fmt::format("{}]", GetRegAsStr(rm));

        if (write_back) {
            disasm += "!";
        }
    } else {
        disasm += fmt::format("[{}], ", GetRegAsStr(rn));
        if (!add_offset_to_base) {
            disasm += "-";
        }
        disasm += GetRegAsStr(rm);
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleHalfwordDataTransferImmediate(const u32 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool pre_indexing = Common::IsBitSet<24>(opcode);
    const bool add_offset_to_base = Common::IsBitSet<23>(opcode);
    const bool write_back = Common::IsBitSet<21>(opcode);
    const bool load_from_memory = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto offset_high = Common::GetBitRange<11, 8>(opcode);
    const bool sign = Common::IsBitSet<6>(opcode);
    const bool halfword = Common::IsBitSet<5>(opcode);
    const std::unsigned_integral auto offset_low = Common::GetBitRange<3, 0>(opcode);
    const std::unsigned_integral auto offset = (offset_high << 4) | offset_low;
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

    disasm += fmt::format(" {}, ", GetRegAsStr(rd));

    if (pre_indexing) {
        if (offset == 0) {
            disasm += fmt::format("[{}]", GetRegAsStr(rn));
        } else {
            disasm += fmt::format("[{}, #", GetRegAsStr(rn));
            if (!add_offset_to_base) {
                disasm += "-";
            }
            disasm += fmt::format("0x{:02X}]", offset);

            if (write_back) {
                disasm += "!";
            }
        }
    } else {
        disasm += fmt::format("[{}], #", GetRegAsStr(rn));
        if (!add_offset_to_base) {
            disasm += "-";
        }
        disasm += fmt::format("0x{:02X}", offset);
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleSingleDataTransfer(const u32 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool offset_is_register = Common::IsBitSet<25>(opcode);
    const bool add_before_transfer = Common::IsBitSet<24>(opcode);
    const bool add_offset_to_base = Common::IsBitSet<23>(opcode);
    const bool transfer_byte = Common::IsBitSet<22>(opcode);
    const bool write_back = Common::IsBitSet<21>(opcode);
    const bool load_from_memory = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto offset = Common::GetBitRange<11, 0>(opcode);
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

    disasm += fmt::format(" {}, ", GetRegAsStr(rd));

    if (add_before_transfer) {
        if (offset == 0) {
            disasm += fmt::format("[{}]", GetRegAsStr(rn));
        } else {
            if (offset_is_register) {
                const std::unsigned_integral auto shift = Common::GetBitRange<11, 4>(opcode);
                const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(opcode);

                disasm += fmt::format("[{}, ", GetRegAsStr(rn));
                if (!add_offset_to_base) {
                    disasm += "-";
                }
                disasm += GetRegAsStr(rm);

                if (!Common::IsBitSet<0>(shift)) {
                    const std::unsigned_integral auto shift_amount = Common::GetBitRange<7, 3>(shift);
                    const std::unsigned_integral auto shift_type = Common::GetBitRange<2, 1>(shift);

                    if (!shift_amount) {
                        disasm += "]";
                    } else {
                        constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                        disasm += fmt::format(", {} #{}]", shift_types[shift_type], shift_amount);
                    }
                } else if (!Common::IsBitSet<3>(shift) && Common::IsBitSet<0>(shift)) {
                    UNIMPLEMENTED();
                } else {
                    ASSERT(false);
                }
            } else {
                disasm += fmt::format("[{}, #", GetRegAsStr(rn));
                if (!add_offset_to_base) {
                    disasm += '-';
                }
                disasm += fmt::format("0x{:08X}]", offset);
            }
        }
    } else {
        if (offset_is_register) {
            const std::unsigned_integral auto shift = Common::GetBitRange<11, 4>(offset);
            const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(offset);

            disasm += fmt::format("[{}], ", GetRegAsStr(rn));
            if (!add_offset_to_base) {
                disasm += "-";
            }
            disasm += GetRegAsStr(rm);

            if (!Common::IsBitSet<0>(shift)) {
                const std::unsigned_integral auto shift_amount = Common::GetBitRange<7, 3>(shift);
                const std::unsigned_integral auto shift_type = Common::GetBitRange<2, 1>(shift);

                if (shift_amount) {
                    constexpr std::array<const char*, 4> shift_types = { "LSL", "LSR", "ASR", "ROR" };
                    disasm += fmt::format(", {} #{}", shift_types[shift_type], shift_amount);
                }
            } else if (!Common::IsBitSet<3>(shift) && Common::IsBitSet<0>(shift)) {
                UNIMPLEMENTED();
            } else {
                ASSERT(false);
            }
        } else {
            disasm += fmt::format("[{}], #0x{:08X}", GetRegAsStr(rn), offset);
        }
    }

    if (write_back) {
        disasm += "!";
    }

    LTRACE_ARM("{}", disasm);
}

void ARM7::ARM_DisassembleBlockDataTransfer(const u32 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool pre_indexing = Common::IsBitSet<24>(opcode);
    const bool add_offset_to_base = Common::IsBitSet<23>(opcode);
    const bool load_psr = Common::IsBitSet<22>(opcode);
    const bool write_back = Common::IsBitSet<21>(opcode);
    const bool load_from_memory = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rlist = Common::GetBitRange<15, 0>(opcode);
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

        disasm += fmt::format(" {}", GetRegAsStr(rn));

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
        disasm += GetRegAsStr(set_bits[i]);
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
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const bool link = Common::IsBitSet<24>(opcode);
    s32 offset = Common::GetBitRange<23, 0>(opcode) << 2;
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
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
    const std::unsigned_integral auto comment = Common::GetBitRange<23, 0>(opcode);

    std::string disasm = fmt::format("SWI{} 0x{:X}", GetConditionCode(cond), comment);
    LTRACE_ARM("{}", disasm);
}

void ARM7::Thumb_DisassembleMoveShiftedRegister(const u16 opcode) {
    const std::unsigned_integral auto op = Common::GetBitRange<12, 11>(opcode);
    std::unsigned_integral auto offset = Common::GetBitRange<10, 6>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);

    if (!offset) {
        offset = 32;
    }

    std::string disasm;

    const std::array<std::string, 3> mnemonics = { "LSL", "LSR", "ASR" };
    disasm += mnemonics.at(op);

    disasm += fmt::format(" {}, {}, #{}", GetRegAsStr(rd), GetRegAsStr(rs), offset);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleAddSubtract(const u16 opcode) {
    const bool operand_is_immediate = Common::IsBitSet<10>(opcode);
    const bool subtracting = Common::IsBitSet<9>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);
    std::string disasm;

    if (subtracting) {
        disasm += "SUB";
    } else {
        disasm += "ADD";
    }

    disasm += fmt::format(" {}, {}, ", GetRegAsStr(rd), GetRegAsStr(rs));

    if (operand_is_immediate) {
        const std::unsigned_integral auto immediate = Common::GetBitRange<8, 6>(opcode);
        disasm += fmt::format("#0x{:02X}", immediate);
    } else {
        const std::unsigned_integral auto rn = Common::GetBitRange<8, 6>(opcode);
        disasm += GetRegAsStr(rn);
    }

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleMoveCompareAddSubtractImmediate(const u16 opcode) {
    const std::unsigned_integral auto op = Common::GetBitRange<12, 11>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<10, 8>(opcode);
    const std::unsigned_integral auto offset = Common::GetBitRange<7, 0>(opcode);
    std::string disasm;

    constexpr std::array<const char*, 4> mnemonics = { "MOV", "CMP", "ADD", "SUB" };
    disasm += std::string(mnemonics.at(op));

    disasm += fmt::format(" {}, #0x{:02X}", GetRegAsStr(rd), offset);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleALUOperations(const u16 opcode) {
    const std::unsigned_integral auto op = Common::GetBitRange<9, 6>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);
    std::string disasm;

    const std::array<std::string, 16> mnemonics = {
        "AND", "EOR", "LSL", "LSR", "ASR", "ADC", "SBC", "ROR", "TST", "NEG", "CMP", "CMN", "ORR", "MUL", "BIC", "MVN"
    };
    disasm += mnemonics.at(op);

    disasm += fmt::format(" {}, {}", GetRegAsStr(rd), GetRegAsStr(rs));

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleHiRegisterOperationsBranchExchange(const u16 opcode) {
    const std::unsigned_integral auto op = Common::GetBitRange<9, 8>(opcode);
    const bool h1 = Common::IsBitSet<7>(opcode);
    const bool h2 = Common::IsBitSet<6>(opcode);
    const std::unsigned_integral auto rs_hs = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd_hd = Common::GetBitRange<2, 0>(opcode);
    std::string disasm;

    const std::array<std::string, 4> mnemonics = { "ADD", "CMP", "MOV", "BX" };
    disasm += mnemonics.at(op);

    ASSERT(!(op == 0x3 && h1));

    // BX
    if (op == 0x3) {
        disasm += fmt::format(" {}", GetRegAsStr(h2 ? rs_hs + 8 : rs_hs));
    } else {
        disasm += fmt::format(" {}, ", GetRegAsStr(h1 ? rd_hd + 8 : rd_hd));
        disasm += fmt::format("{}", GetRegAsStr(h2 ? rs_hs + 8 : rs_hs));
    }

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassemblePCRelativeLoad(const u16 opcode) {
    const std::unsigned_integral auto rd = Common::GetBitRange<10, 8>(opcode);
    const std::unsigned_integral auto imm = Common::GetBitRange<7, 0>(opcode);

    LTRACE_THUMB("LDR {}, [PC, #0x{:08X}]", GetRegAsStr(rd), imm);
}

void ARM7::Thumb_DisassembleLoadStoreWithRegisterOffset(const u16 opcode) {
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const bool transfer_byte = Common::IsBitSet<10>(opcode);
    const std::unsigned_integral auto ro = Common::GetBitRange<8, 6>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);
    std::string disasm;

    if (load_from_memory) {
        disasm += "LDR";
    } else {
        disasm += "STR";
    }

    if (transfer_byte) {
        disasm += "B";
    }

    disasm += fmt::format(" {}, [{}, {}]", GetRegAsStr(rd), GetRegAsStr(rb), GetRegAsStr(ro));

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleLoadStoreSignExtendedByteHalfword(const u16 opcode) {
    const bool h_flag = Common::IsBitSet<11>(opcode);
    const bool sign_extend = Common::IsBitSet<10>(opcode);
    const std::unsigned_integral auto ro = Common::GetBitRange<8, 6>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);
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

    disasm += fmt::format(" {}, [{}, {}]", GetRegAsStr(rd), GetRegAsStr(rb), GetRegAsStr(ro));

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleLoadStoreWithImmediateOffset(const u16 opcode) {
    const bool transfer_byte = Common::IsBitSet<12>(opcode);
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const std::unsigned_integral auto offset = Common::GetBitRange<10, 6>(opcode);;
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);
    std::string disasm;

    if (load_from_memory) {
        disasm += "LDR";
    } else {
        disasm += "STR";
    }

    if (transfer_byte) {
        disasm += "B";
    }

    disasm += fmt::format(" {}, [{}, #0x{:02X}]", GetRegAsStr(rd), GetRegAsStr(rb), transfer_byte ? offset : offset << 2);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleLoadStoreHalfword(const u16 opcode) {
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const std::unsigned_integral auto imm = Common::GetBitRange<10, 6>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);
    std::string disasm;

    if (load_from_memory) {
        disasm += "LDRH";
    } else {
        disasm += "STRH";
    }

    disasm += fmt::format(" {}, [{}, #0x{:X}]", GetRegAsStr(rd), GetRegAsStr(rb), imm << 1);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleSPRelativeLoadStore(const u16 opcode) {
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<10, 8>(opcode);
    const std::unsigned_integral auto imm = Common::GetBitRange<7, 0>(opcode);
    std::string disasm;

    if (load_from_memory) {
        disasm += "LDR";
    } else {
        disasm += "STR";
    }

    disasm += fmt::format(" {}, [SP, #0x{:X}]", GetRegAsStr(rd), imm << 2);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleLoadAddress(const u16 opcode) {
    const bool load_from_sp = Common::IsBitSet<11>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<10, 8>(opcode);
    const std::unsigned_integral auto imm = Common::GetBitRange<7, 0>(opcode);
    std::string disasm;

    disasm += fmt::format("ADD {}, ", GetRegAsStr(rd));

    if (load_from_sp) {
        disasm += "SP";
    } else {
        disasm += "PC";
    }

    disasm += fmt::format(", #0x{:X}", imm << 2);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleAddOffsetToStackPointer(const u16 opcode) {
    const bool offset_is_negative = Common::IsBitSet<7>(opcode);
    const std::unsigned_integral auto imm = Common::GetBitRange<6, 0>(opcode);
    std::string disasm;

    disasm += "ADD SP, #";
    if (offset_is_negative) {
        disasm += "-";
    }
    disasm += fmt::format("0x{:02X}", imm << 2);

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassemblePushPopRegisters(const u16 opcode) {
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const bool store_lr_load_pc = Common::IsBitSet<8>(opcode);
    const std::unsigned_integral auto rlist = Common::GetBitRange<7, 0>(opcode);
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
    const auto bit_is_set = [rlist](const u8 i) { return (rlist & (1 << i)) != 0; };
    const auto set_bits = ranges::views::iota(0, 8)
                        | ranges::views::filter(bit_is_set)
                        | ranges::to<std::vector>;

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
        disasm += GetRegAsStr(set_bits[i]);
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
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<10, 8>(opcode);
    const std::unsigned_integral auto rlist = Common::GetBitRange<7, 0>(opcode);
    std::string disasm;

    if (load_from_memory) {
        disasm += "LDMIA";
    } else {
        disasm += "STMIA";
    }

    disasm += fmt::format(" {}!, {{", GetRegAsStr(rb));

    // Go through `rlist`'s 8 bits, and write down which bits are set. This tells
    // us which registers we need to load/store.
    // If bit 0 is set, that corresponds to R0. If bit 1 is set, that corresponds
    // to R1, and so on.
    const auto bit_is_set = [rlist](const u8 i) { return (rlist & (1 << i)) != 0; };
    const auto set_bits = ranges::views::iota(0, 8)
                        | ranges::views::filter(bit_is_set)
                        | ranges::to<std::vector>;

    for (u8 i = 0; i < set_bits.size(); i++) {
        disasm += GetRegAsStr(set_bits[i]);
        if (i != set_bits.size() - 1) {
            disasm += ", ";
        }
    }

    disasm += "}";

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleConditionalBranch(const u16 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<11, 8>(opcode);
    const s8 offset = static_cast<s8>(Common::GetBitRange<7, 0>(opcode));

    std::string disasm;

    ASSERT(cond != 0xF);

    constexpr std::array<const char*, 14> mnemonics = {
        "BEQ", "BNE", "BCS", "BCC", "BMI", "BPL", "BVS", "BVC", "BHI", "BLS", "BGE", "BLT", "BGT", "BLE" 
    };
    disasm += std::string(mnemonics.at(cond));

    disasm += fmt::format(" 0x{:08X}", GetPC() + (offset * 2));

    LTRACE_THUMB("{}", disasm);
}

void ARM7::Thumb_DisassembleSoftwareInterrupt(const u16 opcode) {
    const std::unsigned_integral auto comment = Common::GetBitRange<7, 0>(opcode);

    LTRACE_THUMB("SWI 0x{:X}", comment);
}

void ARM7::Thumb_DisassembleUnconditionalBranch(const u16 opcode) {
    s16 offset = static_cast<s16>((Common::GetBitRange<11, 0>(opcode)) << 1);

    // Sign-extend to 16 bits
    offset <<= 4;
    offset >>= 4;

    LTRACE_THUMB("B 0x{:08X}", GetPC() + offset);
}

void ARM7::Thumb_DisassembleLongBranchWithLink(const u16 opcode) {
    const u16 next_opcode = bus.Read16(GetPC() - 2);
    // Used for LTRACE_DOUBLETHUMB
    const u32 double_opcode = (static_cast<u32>(opcode) << 16) | next_opcode;
    s16 offset = Common::GetBitRange<10, 0>(opcode);
    const u16 next_offset = Common::GetBitRange<10, 0>(next_opcode);

    u32 pc = GetPC();

    const bool first_instruction = !Common::IsBitSet<11>(opcode);
    ASSERT(first_instruction);

    offset <<= 5;
    u32 lr = pc + (static_cast<s32>(offset) << 7);

    const bool second_instruction = Common::IsBitSet<11>(next_opcode);
    ASSERT(second_instruction);

    pc = lr + (next_offset << 1);
    lr = GetPC() | 0b1;

    LDEBUG("LR={:08X}", lr);
    LTRACE_DOUBLETHUMB("BL 0x{:08X}", pc);
}
