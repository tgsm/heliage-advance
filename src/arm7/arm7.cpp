#include <bit>
#include "arm7.h"
#include "common/bits.h"
#include "common/logging.h"

ARM7::ARM7(Bus& bus_, Timers& timers_)
    : bus(bus_), timers(timers_) {
    GenerateThumbLUT();

    // Initialize registers
    cpsr.flags.processor_mode = ProcessorMode::Supervisor;
    SetPC(0x00000000);
}

void ARM7::HandleInterrupts() {
    if (cpsr.flags.irq_disabled) {
        return;
    }

    if (!bus.GetInterrupts().GetIME()) {
        return;
    }

    const u16 ie_reg = bus.GetInterrupts().GetIE();
    const u16 if_reg = bus.GetInterrupts().GetIF();
    if ((if_reg & ie_reg) == 0) {
        return;
    }

    halted = false;

    if (!started_ime_delay) {
        started_ime_delay = true;
        return;
    }

    if (ime_delay) {
        ime_delay--;
        return;
    }

    // FIXME: this doesn't seem right.
    const u32 lr = GetPC() + (cpsr.flags.thumb_mode ? 2 : 0); // - (cpsr.flags.thumb_mode ? 2 : 4);
    const u32 old_cpsr = cpsr.raw;

    cpsr.flags.processor_mode = ProcessorMode::IRQ;
    SetLR(lr);
    cpsr.flags.thumb_mode = false;
    cpsr.flags.irq_disabled = true;
    SetPC(0x00000018);
    SetSPSR(old_cpsr);

    started_ime_delay = false;
    ime_delay = 2;
}

void ARM7::Step(bool dump_registers) {
    HandleInterrupts();

    if (halted) {
        timers.AdvanceCycles(1, Timers::CycleType::None);
        return;
    }

    // FIXME: put the pipeline stuff in its own function
    if (cpsr.flags.thumb_mode) {
        const u16 opcode = pipeline[0];

        pipeline[0] = pipeline[1];
        gpr[15] += 2;
        pipeline[1] = bus.Read16(gpr[15]);

        const Thumb_Instructions instr = DecodeThumbInstruction(opcode);
        if (dump_registers)
            DumpRegisters();
        // DisassembleThumbInstruction(instr, opcode);
        ExecuteThumbInstruction(instr, opcode);
    } else {
        const u32 opcode = pipeline[0];

        pipeline[0] = pipeline[1];
        gpr[15] += 4;
        pipeline[1] = bus.Read32(gpr[15]);

        const ARM_Instructions instr = DecodeARMInstruction(opcode);
        if (dump_registers)
            DumpRegisters();

        // DisassembleARMInstruction(instr, opcode);
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
    if ((opcode & 0x0FFFFFF0) == 0x012FFF10) return ARM_Instructions::BranchAndExchange;
    if ((opcode & 0x0FB00FF0) == 0x01000090) return ARM_Instructions::SingleDataSwap;
    if ((opcode & 0x0F8000F0) == 0x00800090) return ARM_Instructions::MultiplyLong;
    if ((opcode & 0x0FC000F0) == 0x00000090) return ARM_Instructions::Multiply;
    if ((opcode & 0x0E400090) == 0x00400090) return ARM_Instructions::HalfwordDataTransferImmediate;
    if ((opcode & 0x0E400F90) == 0x00000090) return ARM_Instructions::HalfwordDataTransferRegister;
    if ((opcode & 0x0C000000) == 0x00000000) return ARM_Instructions::DataProcessing;

    // The opcode did not meet any of the above conditions.
    return ARM_Instructions::Unknown;
}

void ARM7::ExecuteARMInstruction(const ARM_Instructions instr, const u32 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<31, 28>(opcode);
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
        case ARM_Instructions::SingleDataSwap:
            ARM_SingleDataSwap(opcode);
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
        case ARM_Instructions::SoftwareInterrupt:
            ARM_SoftwareInterrupt(opcode);
            break;

        case ARM_Instructions::Unknown:
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented ARM instruction ({})", Common::GetUnderlyingValue(instr));
    }
}

template <std::size_t I>
consteval ARM7::Thumb_Instructions ARM7::GenerateThumbLUT_Impl() {
    if ((I & 0xF0) == 0xF0) return Thumb_Instructions::LongBranchWithLink;
    if ((I & 0xF8) == 0xE0) return Thumb_Instructions::UnconditionalBranch;
    if ((I & 0xFF) == 0xDF) return Thumb_Instructions::SoftwareInterrupt;
    if ((I & 0xF0) == 0xD0) return Thumb_Instructions::ConditionalBranch;
    if ((I & 0xF0) == 0xC0) return Thumb_Instructions::MultipleLoadStore;
    if ((I & 0xF6) == 0xB4) return Thumb_Instructions::PushPopRegisters;
    if ((I & 0xFF) == 0xB0) return Thumb_Instructions::AddOffsetToStackPointer;
    if ((I & 0xF0) == 0xA0) return Thumb_Instructions::LoadAddress;
    if ((I & 0xF0) == 0x90) return Thumb_Instructions::SPRelativeLoadStore;
    if ((I & 0xF0) == 0x80) return Thumb_Instructions::LoadStoreHalfword;
    if ((I & 0xE0) == 0x60) return Thumb_Instructions::LoadStoreWithImmediateOffset;
    if ((I & 0xF2) == 0x52) return Thumb_Instructions::LoadStoreSignExtendedByteHalfword;
    if ((I & 0xF2) == 0x50) return Thumb_Instructions::LoadStoreWithRegisterOffset;
    if ((I & 0xF8) == 0x48) return Thumb_Instructions::PCRelativeLoad;
    if ((I & 0xFC) == 0x44) return Thumb_Instructions::HiRegisterOperationsBranchExchange;
    if ((I & 0xFC) == 0x40) return Thumb_Instructions::ALUOperations;
    if ((I & 0xE0) == 0x20) return Thumb_Instructions::MoveCompareAddSubtractImmediate;
    if ((I & 0xF8) == 0x18) return Thumb_Instructions::AddSubtract;
    if ((I & 0xE0) == 0x00) return Thumb_Instructions::MoveShiftedRegister;

    // The opcode did not meet any of the above conditions.
    return Thumb_Instructions::Unknown;
}

constexpr void ARM7::GenerateThumbLUT() {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        ((thumb_lut[I] = GenerateThumbLUT_Impl<I>()), ...);
    }(std::make_index_sequence<THUMB_LUT_SIZE>{});
}

ARM7::Thumb_Instructions ARM7::DecodeThumbInstruction(const u16 opcode) const {
    return thumb_lut[Common::GetBitRange<15, 8>(opcode)];
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
        case Thumb_Instructions::SPRelativeLoadStore:
            Thumb_SPRelativeLoadStore(opcode);
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
        case Thumb_Instructions::SoftwareInterrupt:
            Thumb_SoftwareInterrupt(opcode);
            break;
        case Thumb_Instructions::UnconditionalBranch:
            Thumb_UnconditionalBranch(opcode);
            break;
        case Thumb_Instructions::LongBranchWithLink:
            Thumb_LongBranchWithLink(opcode);
            break;

        case Thumb_Instructions::Unknown:
        default:
            UNIMPLEMENTED_MSG("interpreter: unknown THUMB instruction ({})", Common::GetUnderlyingValue(instr));
    }
}

void ARM7::DumpRegisters() {
    fmt::print("r0: {:08X} r1: {:08X} r2: {:08X} r3: {:08X}\n", GetRegister(0), GetRegister(1), GetRegister(2), GetRegister(3));
    fmt::print("r4: {:08X} r5: {:08X} r6: {:08X} r7: {:08X}\n", GetRegister(4), GetRegister(5), GetRegister(6), GetRegister(7));
    fmt::print("r8: {:08X} r9: {:08X} r10:{:08X} r11:{:08X}\n", GetRegister(8), GetRegister(9), GetRegister(10), GetRegister(11));
    fmt::print("r12:{:08X} sp: {:08X} lr: {:08X} pc: {:08X}\n", GetRegister(12), GetSP(), GetLR(), cpsr.flags.thumb_mode ? GetPC() - 4 : GetPC() - 8);
    fmt::print("cpsr:{:08X}\n", cpsr.raw);
}

void ARM7::FillPipeline() {
    if (cpsr.flags.thumb_mode) {
        pipeline[0] = bus.Read16(gpr[15]);
        // LDEBUG("r15={:08X} p0={:04X}", gpr[15], pipeline[0]);
        gpr[15] += 2;
        pipeline[1] = bus.Read16(gpr[15]);
        // LDEBUG("r15={:08X} p1={:04X}", gpr[15], pipeline[1]);
    } else {
        pipeline[0] = bus.Read32(gpr[15]);
        // LDEBUG("r15={:08X} p0={:08X}", gpr[15], pipeline[0]);
        gpr[15] += 4;
        pipeline[1] = bus.Read32(gpr[15]);
        // LDEBUG("r15={:08X} p1={:08X}", gpr[15], pipeline[1]);
    }
}

u32 ARM7::Shift(const u64 operand_to_shift, const ShiftType shift_type, const u8 shift_amount, const bool set_condition_codes) {
    if (!shift_amount) { // shift by 0 digits
        return operand_to_shift;
    }

    switch (shift_type) {
        case ShiftType::LSL:
            return Shift_LSL(operand_to_shift, shift_amount, set_condition_codes);
        case ShiftType::LSR:
            return Shift_LSR(operand_to_shift, shift_amount, set_condition_codes);
        case ShiftType::ASR:
            return Shift_ASR(operand_to_shift, shift_amount, set_condition_codes);
        case ShiftType::ROR:
            return Shift_RotateRight(operand_to_shift, shift_amount, set_condition_codes);
        default:
            UNREACHABLE();
    }
}

u32 ARM7::Shift_LSL(const u64 operand_to_shift, const u8 shift_amount, const bool set_condition_codes) {
    if (shift_amount >= 32) {
        if (set_condition_codes) {
            if (shift_amount == 32) {
                cpsr.flags.carry = Common::IsBitSet<0>(operand_to_shift);
            } else {
                cpsr.flags.carry = false;
            }
        }

        return 0;
    }

    if (set_condition_codes) {
        cpsr.flags.carry = Common::IsBitSet<0>(operand_to_shift >> (32 - shift_amount));
    }

    return operand_to_shift << shift_amount;
}

u32 ARM7::Shift_LSR(const u64 operand_to_shift, const u8 shift_amount, const bool set_condition_codes) {
    if (shift_amount >= 32) {
        if (set_condition_codes) {
            if (shift_amount == 32) {
                cpsr.flags.carry = Common::IsBitSet<31>(operand_to_shift);
            } else {
                cpsr.flags.carry = false;
            }
        }

        return 0;
    }

    if (set_condition_codes) {
        cpsr.flags.carry = Common::IsBitSet<0>(operand_to_shift >> (shift_amount - 1));
    }

    return operand_to_shift >> shift_amount;
}

u32 ARM7::Shift_ASR(const u64 operand_to_shift, const u8 shift_amount, const bool set_condition_codes) {
    u32 result = 0;

    if (shift_amount >= 32) {
        result = u32(s32(operand_to_shift) >> 31);
        if (set_condition_codes) {
            cpsr.flags.carry = Common::IsBitSet<0>(result);
        }
    } else {
        result = u32(s32(operand_to_shift) >> shift_amount);
        if (set_condition_codes) {
            cpsr.flags.carry = Common::IsBitSet<0>(operand_to_shift >> (shift_amount - 1));
        }
    }

    return result;
}

u32 ARM7::Shift_RotateRight(const u32 operand_to_rotate, const u8 rotate_amount, const bool set_condition_codes) {
    if (!rotate_amount) { // rotate by 0 digits
        return operand_to_rotate;
    }

    const u32 result = std::rotr(operand_to_rotate, rotate_amount);

    if (set_condition_codes) {
        cpsr.flags.carry = Common::IsBitSet<31>(result);
    }

    return result;
}

u32 ARM7::Shift_RRX(const u32 operand_to_rotate, const bool set_condition_codes) {
    const u32 result = (operand_to_rotate >> 1) | (cpsr.flags.carry << 31);

    if (set_condition_codes) {
        cpsr.flags.carry = Common::IsBitSet<0>(operand_to_rotate);
    }

    return result;
}

u32 ARM7::ADC(const u32 operand1, const u32 operand2, const bool change_flags) {
    u32 result = operand1 + operand2 + cpsr.flags.carry;
    if (change_flags) {
        cpsr.flags.negative = Common::IsBitSet<31>(result);
        cpsr.flags.zero = (result == 0);
        cpsr.flags.carry = (result < operand1);
        cpsr.flags.overflow = ((operand1 ^ result) & (operand2 ^ result)) >> 31;
    }

    return result;
}

u32 ARM7::ADD(const u32 operand1, const u32 operand2, const bool change_flags) {
    u32 result = operand1 + operand2;
    if (change_flags) {
        cpsr.flags.negative = Common::IsBitSet<31>(result);
        cpsr.flags.zero = (result == 0);
        cpsr.flags.carry = (result < operand1);
        cpsr.flags.overflow = ((operand1 ^ result) & (operand2 ^ result)) >> 31;
    }

    return result;
}

void ARM7::CMN(const u32 operand1, const u32 operand2) {
    u32 result = operand1 + operand2;

    cpsr.flags.negative = Common::IsBitSet<31>(result);
    cpsr.flags.zero = (result == 0);
    cpsr.flags.carry = (result < operand1);
    cpsr.flags.overflow = ((operand1 ^ result) & (operand2 ^ result)) >> 31;
}

void ARM7::CMP(const u32 operand1, const u32 operand2) {
    u32 result = operand1 - operand2;

    cpsr.flags.negative = Common::IsBitSet<31>(result);
    cpsr.flags.zero = (result == 0);
    cpsr.flags.carry = (result <= operand1);
    cpsr.flags.overflow = ((operand1 ^ result) & (~operand2 ^ result)) >> 31;
}

u32 ARM7::SBC(const u32 operand1, const u32 operand2, const bool change_flags) {
    u32 result = operand1 - operand2 - !cpsr.flags.carry;
    if (change_flags) {
        cpsr.flags.negative = Common::IsBitSet<31>(result);
        cpsr.flags.zero = (result == 0);
        cpsr.flags.carry = (result <= operand1);
        cpsr.flags.overflow = ((operand1 ^ result) & (~operand2 ^ result)) >> 31;
    }

    return result;
}

u32 ARM7::SUB(const u32 operand1, const u32 operand2, const bool change_flags) {
    u32 result = operand1 - operand2;
    if (change_flags) {
        cpsr.flags.negative = Common::IsBitSet<31>(result);
        cpsr.flags.zero = (result == 0);
        cpsr.flags.carry = (result <= operand1);
        cpsr.flags.overflow = ((operand1 ^ result) & (~operand2 ^ result)) >> 31;
    }

    return result;
}

void ARM7::TEQ(const u32 operand1, const u32 operand2) {
    u32 result = operand1 ^ operand2;

    cpsr.flags.negative = Common::IsBitSet<31>(result);
    cpsr.flags.zero = (result == 0);
    // carry flag?
}

void ARM7::TST(const u32 operand1, const u32 operand2) {
    u32 result = operand1 & operand2;

    cpsr.flags.negative = Common::IsBitSet<31>(result);
    cpsr.flags.zero = (result == 0);
    // carry flag?
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
        [[unlikely]] default: // cond is 0xF
            UNREACHABLE_MSG("invalid condition code 0x{:X}", cond);
    }
}
