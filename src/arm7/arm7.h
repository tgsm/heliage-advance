#pragma once

#include "bus.h"
#include "common/logging.h"
#include "types.h"

class ARM7 {
public:
    ARM7(Bus& bus_, PPU& ppu_);

    void Step(bool dump_registers);

    [[nodiscard]] inline u32 GetPC() const { return GetRegister(15); }

private:
    [[nodiscard]] inline u32 GetSP() const { return GetRegister(13); }
    inline void SetSP(u32 value) { SetRegister(13, value); }

    [[nodiscard]] inline u32 GetLR() const { return GetRegister(14); }
    inline void SetLR(u32 value) { SetRegister(14, value); }

    inline void SetPC(u32 value) { SetRegister(15, value); }

    inline u32 GetRegister(u8 reg) const {
        ASSERT(reg <= 15);

        if (reg <= 7) {
            return gpr[reg];
        }

        switch (cpsr.flags.processor_mode) {
            case ProcessorMode::System:
            case ProcessorMode::User:
                return gpr[reg];
            case ProcessorMode::FIQ:
                if (reg >= 8 && reg <= 14) {
                    return fiq_r[reg - 8];
                }

                return gpr[reg];
            case ProcessorMode::Supervisor:
                if (reg == 13 || reg == 14) {
                    return svc_r[reg - 13];
                }

                return gpr[reg];
            case ProcessorMode::Abort:
                if (reg == 13 || reg == 14) {
                    return abt_r[reg - 13];
                }

                return gpr[reg];
            case ProcessorMode::IRQ:
                if (reg == 13 || reg == 14) {
                    return irq_r[reg - 13];
                }

                return gpr[reg];
            case ProcessorMode::Undefined:
                if (reg == 13 || reg == 14) {
                    return und_r[reg - 13];
                }

                return gpr[reg];
            default:
                UNREACHABLE_MSG("invalid ARM7 processor mode 0x{:X}", cpsr.flags.processor_mode);
        }
    }

    inline void SetRegister(u8 reg, u32 value) {
        ASSERT(reg <= 15);

        if (reg <= 7) {
            gpr[reg] = value;
            return;
        }

        switch (cpsr.flags.processor_mode) {
            case ProcessorMode::System:
            case ProcessorMode::User:
                gpr[reg] = value;
                break;
            case ProcessorMode::FIQ:
                if (reg >= 8 && reg <= 14) {
                    fiq_r[reg - 8] = value;
                    break;
                }

                gpr[reg] = value;
                break;
            case ProcessorMode::Supervisor:
                if (reg == 13 || reg == 14) {
                    svc_r[reg - 13] = value;
                    break;
                }

                gpr[reg] = value;
                break;
            case ProcessorMode::Abort:
                if (reg == 13 || reg == 14) {
                    abt_r[reg - 13] = value;
                    break;
                }

                gpr[reg] = value;
                break;
            case ProcessorMode::IRQ:
                if (reg == 13 || reg == 14) {
                    irq_r[reg - 13] = value;
                    break;
                }

                gpr[reg] = value;
                break;
            case ProcessorMode::Undefined:
                if (reg == 13 || reg == 14) {
                    und_r[reg - 13] = value;
                    break;
                }

                gpr[reg] = value;
                break;
            default:
                UNREACHABLE_MSG("invalid ARM7 processor mode 0x{:X}", cpsr.flags.processor_mode);
        }

        if (reg == 15) {
            // Refill the pipeline whenever we change R15 aka the PC
            FillPipeline();
        }
    }

    inline u32 GetSPSR() const {
        switch (cpsr.flags.processor_mode) {
            case ProcessorMode::System:
            case ProcessorMode::User:
                return cpsr.raw;
            case ProcessorMode::Supervisor:
                return spsr_svc.raw;
            case ProcessorMode::FIQ:
                return spsr_fiq.raw;
            default:
                UNIMPLEMENTED_MSG("unimplemented get spsr from mode {:05b}", cpsr.flags.processor_mode);
        }
    }

    inline void SetSPSR(const u32 cpsr_raw) {
        switch (cpsr.flags.processor_mode) {
            case ProcessorMode::System:
            case ProcessorMode::User:
                break;
            case ProcessorMode::Supervisor:
                spsr_svc.raw = cpsr_raw;
                break;
            case ProcessorMode::FIQ:
                spsr_fiq.raw = cpsr_raw;
                break;
            default:
                UNIMPLEMENTED_MSG("unimplemented set spsr for mode {:05b}", cpsr.flags.processor_mode);
        }
    }

    [[nodiscard]] std::string GetRegAsStr(const u8 reg) const;

    enum class ARM_Instructions {
        DataProcessing,
        Multiply,
        MultiplyLong,
        SingleDataSwap,
        BranchAndExchange,
        HalfwordDataTransferRegister,
        HalfwordDataTransferImmediate,
        SingleDataTransfer,
        BlockDataTransfer,
        Branch,
        CoprocessorDataTransfer,
        CoprocessorDataOperation,
        CoprocessorRegisterTransfer,
        SoftwareInterrupt,

        Undefined,
        Unknown,
    };

    enum class Thumb_Instructions {
        MoveShiftedRegister,
        AddSubtract,
        MoveCompareAddSubtractImmediate,
        ALUOperations,
        HiRegisterOperationsBranchExchange,
        PCRelativeLoad,
        LoadStoreWithRegisterOffset,
        LoadStoreSignExtendedByteHalfword,
        LoadStoreWithImmediateOffset,
        LoadStoreHalfword,
        SPRelativeLoadStore,
        LoadAddress,
        AddOffsetToStackPointer,
        PushPopRegisters,
        MultipleLoadStore,
        ConditionalBranch,
        SoftwareInterrupt,
        UnconditionalBranch,
        LongBranchWithLink,

        Unknown,
    };

    enum class ProcessorMode : u8 {
        User = 0b10000,
        FIQ = 0b10001,
        IRQ = 0b10010,
        Supervisor = 0b10011,
        Abort = 0b10111,
        Undefined = 0b11011,
        System = 0b11111,
    };

    [[nodiscard]] ARM_Instructions DecodeARMInstruction(const u32 opcode) const;
    void ExecuteARMInstruction(const ARM_Instructions instr, const u32 opcode);
    void DisassembleARMInstruction(const ARM_Instructions instr, const u32 opcode);
    [[nodiscard]] Thumb_Instructions DecodeThumbInstruction(const u16 opcode) const;
    void ExecuteThumbInstruction(const Thumb_Instructions instr, const u16 opcode);
    void DisassembleThumbInstruction(const Thumb_Instructions instr, const u16 opcode);

    void DumpRegisters();

    void FillPipeline();
    void Die(const char* format, ...);

    enum class ShiftType {
        LSL,
        LSR,
        ASR,
        ROR,
        RRX,
    };

    [[nodiscard]] u32 Shift(const u64 operand_to_shift, const ShiftType shift_type, const u8 shift_amount);
    [[nodiscard]] u32 Shift_LSL(const u64 operand_to_shift, const u8 shift_amount);
    [[nodiscard]] u32 Shift_LSR(const u64 operand_to_shift, const u8 shift_amount);
    [[nodiscard]] u32 Shift_ASR(const u64 operand_to_shift, const u8 shift_amount);
    [[nodiscard]] u32 Shift_RotateRight(const u32 operand_to_rotate, const u8 rotate_amount);
    [[nodiscard]] u32 Shift_RRX(const u32 operand_to_rotate);

    [[nodiscard]] u32 ADC(const u32 operand1, const u32 operand2, const bool change_flags);
    [[nodiscard]] u32 ADD(const u32 operand1, const u32 operand2, const bool change_flags);
    void CMN(const u32 operand1, const u32 operand2);
    void CMP(const u32 operand1, const u32 operand2);
    [[nodiscard]] u32 SBC(const u32 operand1, const u32 operand2, const bool change_flags);
    [[nodiscard]] u32 SUB(const u32 operand1, const u32 operand2, const bool change_flags);
    void TEQ(const u32 operand1, const u32 operand2);
    void TST(const u32 operand1, const u32 operand2);

    [[nodiscard]] bool CheckConditionCode(const u8 cond);
    [[nodiscard]] std::string GetConditionCode(const u8 cond);

    // Instruction pipeline
    std::array<u32, 2> pipeline {};

    // General purpose registers
    std::array<u32, 16> gpr {};

    // FIQ-mode registers
    std::array<u32, 7> fiq_r {};

    // Supervisor-mode registers
    std::array<u32, 2> svc_r = { 0x03007FE0, 0 };

    // Abort-mode registers
    std::array<u32, 2> abt_r {};

    // IRQ-mode registers
    std::array<u32, 2> irq_r = { 0x03007FA0, 0 };

    // Undefined-mode registers
    std::array<u32, 2> und_r = { 0x03007F00, 0 };

    union PSR {
        u32 raw = 0;
        struct {
            // FIXME: this struct assumes the user is running on a little-endian system.
            ProcessorMode processor_mode : 5;
            bool thumb_mode : 1;
            bool fiq_disabled : 1;
            bool irq_disabled : 1;
            u32 : 20;
            bool overflow : 1;
            bool carry : 1;
            bool zero : 1;
            bool negative : 1;
        } flags;
    };

    PSR cpsr;
    PSR spsr_fiq;
    PSR spsr_svc;
    PSR spsr_abt;
    PSR spsr_irq;
    PSR spsr_und;

    Bus& bus;
    PPU& ppu;

    void ARM_DataProcessing(const u32 opcode);
    void ARM_DisassembleDataProcessing(const u32 opcode);

    void ARM_MRS(const u32 opcode);
    void ARM_DisassembleMRS(const u32 opcode);
    template <bool flag_bits_only>
    void ARM_MSR(const u32 opcode);
    template <bool flag_bits_only>
    void ARM_DisassembleMSR(const u32 opcode);

    void ARM_Multiply(const u32 opcode);
    void ARM_DisassembleMultiply(const u32 opcode);

    void ARM_MultiplyLong(const u32 opcode);
    void ARM_DisassembleMultiplyLong(const u32 opcode);

    void ARM_SingleDataSwap(const u32 opcode);
    void ARM_DisassembleSingleDataSwap(const u32 opcode);

    void ARM_BranchAndExchange(const u32 opcode);
    void ARM_DisassembleBranchAndExchange(const u32 opcode);

    void ARM_HalfwordDataTransferRegister(const u32 opcode);
    void ARM_DisassembleHalfwordDataTransferRegister(const u32 opcode);
    template <bool load_from_memory, bool transfer_halfword>
    void ARM_HalfwordDataTransferRegister_Impl(const u32 opcode, const bool sign);

    void ARM_HalfwordDataTransferImmediate(const u32 opcode);
    void ARM_DisassembleHalfwordDataTransferImmediate(const u32 opcode);
    void ARM_LoadHalfwordImmediate(const u32 opcode, const bool sign);
    void ARM_StoreHalfwordImmediate(const u32 opcode, const bool sign);
    void ARM_LoadSignedByte(const u32 opcode);

    void ARM_SingleDataTransfer(const u32 opcode);
    void ARM_DisassembleSingleDataTransfer(const u32 opcode);
    template <bool load_from_memory, bool transfer_byte>
    void ARM_SingleDataTransfer_Impl(const u32 opcode);

    void ARM_BlockDataTransfer(const u32 opcode);
    void ARM_DisassembleBlockDataTransfer(const u32 opcode);

    void ARM_Branch(const u32 opcode);
    void ARM_DisassembleBranch(const u32 opcode);
    
    // TODO: Coprocessor data transfer

    // TODO: Coprocessor data operation

    // TODO: Coprocessor register transfer

    void ARM_SoftwareInterrupt(const u32 opcode);
    void ARM_DisassembleSoftwareInterrupt(const u32 opcode);
    void ARM_SWI_Div();

    void Thumb_MoveShiftedRegister(const u16 opcode);
    void Thumb_DisassembleMoveShiftedRegister(const u16 opcode);
    void Thumb_LSL(const u16 opcode);
    void Thumb_LSR(const u16 opcode);
    void Thumb_ASR(const u16 opcode);

    void Thumb_AddSubtract(const u16 opcode);
    void Thumb_DisassembleAddSubtract(const u16 opcode);

    void Thumb_MoveCompareAddSubtractImmediate(const u16 opcode);
    void Thumb_DisassembleMoveCompareAddSubtractImmediate(const u16 opcode);

    void Thumb_ALUOperations(const u16 opcode);
    void Thumb_DisassembleALUOperations(const u16 opcode);

    void Thumb_HiRegisterOperationsBranchExchange(const u16 opcode);
    void Thumb_DisassembleHiRegisterOperationsBranchExchange(const u16 opcode);

    void Thumb_PCRelativeLoad(const u16 opcode);
    void Thumb_DisassemblePCRelativeLoad(const u16 opcode);

    void Thumb_LoadStoreWithRegisterOffset(const u16 opcode);
    void Thumb_DisassembleLoadStoreWithRegisterOffset(const u16 opcode);

    void Thumb_LoadStoreSignExtendedByteHalfword(const u16 opcode);
    void Thumb_DisassembleLoadStoreSignExtendedByteHalfword(const u16 opcode);

    void Thumb_LoadStoreWithImmediateOffset(const u16 opcode);
    void Thumb_DisassembleLoadStoreWithImmediateOffset(const u16 opcode);
    void Thumb_StoreByteWithImmediateOffset(const u16 opcode);
    void Thumb_LoadByteWithImmediateOffset(const u16 opcode);
    void Thumb_StoreWordWithImmediateOffset(const u16 opcode);
    void Thumb_LoadWordWithImmediateOffset(const u16 opcode);

    void Thumb_LoadStoreHalfword(const u16 opcode);
    void Thumb_DisassembleLoadStoreHalfword(const u16 opcode);

    void Thumb_SPRelativeLoadStore(const u16 opcode);
    void Thumb_DisassembleSPRelativeLoadStore(const u16 opcode);

    void Thumb_LoadAddress(const u16 opcode);
    void Thumb_DisassembleLoadAddress(const u16 opcode);

    void Thumb_AddOffsetToStackPointer(const u16 opcode);
    void Thumb_DisassembleAddOffsetToStackPointer(const u16 opcode);

    void Thumb_PushPopRegisters(const u16 opcode);
    void Thumb_DisassemblePushPopRegisters(const u16 opcode);

    void Thumb_MultipleLoadStore(const u16 opcode);
    void Thumb_DisassembleMultipleLoadStore(const u16 opcode);

    void Thumb_ConditionalBranch(const u16 opcode);
    void Thumb_DisassembleConditionalBranch(const u16 opcode);

    void Thumb_SoftwareInterrupt(const u16 opcode);
    void Thumb_DisassembleSoftwareInterrupt(const u16 opcode);

    void Thumb_UnconditionalBranch(const u16 opcode);
    void Thumb_DisassembleUnconditionalBranch(const u16 opcode);

    void Thumb_LongBranchWithLink(const u16 opcode);
    void Thumb_DisassembleLongBranchWithLink(const u16 opcode);
};
