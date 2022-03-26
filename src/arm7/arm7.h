#pragma once

#include "bus.h"
#include "common/logging.h"
#include "common/types.h"

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

    [[nodiscard]] std::string GetRegAsStr(u8 reg) const;

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

    [[nodiscard]] ARM_Instructions DecodeARMInstruction(u32 opcode) const;
    void ExecuteARMInstruction(ARM_Instructions instr, u32 opcode);
    void DisassembleARMInstruction(ARM_Instructions instr, u32 opcode);
    [[nodiscard]] Thumb_Instructions DecodeThumbInstruction(u16 opcode) const;
    void ExecuteThumbInstruction(Thumb_Instructions instr, u16 opcode);
    void DisassembleThumbInstruction(Thumb_Instructions instr, u16 opcode);

    void DumpRegisters();

    void FillPipeline();

    enum class ShiftType {
        LSL,
        LSR,
        ASR,
        ROR,
        RRX,
    };

    [[nodiscard]] u32 Shift(u64 operand_to_shift, ShiftType shift_type, u8 shift_amount);
    [[nodiscard]] u32 Shift_LSL(u64 operand_to_shift, u8 shift_amount);
    [[nodiscard]] u32 Shift_LSR(u64 operand_to_shift, u8 shift_amount);
    [[nodiscard]] u32 Shift_ASR(u64 operand_to_shift, u8 shift_amount);
    [[nodiscard]] u32 Shift_RotateRight(u32 operand_to_rotate, u8 rotate_amount);
    [[nodiscard]] u32 Shift_RRX(u32 operand_to_rotate);

    [[nodiscard]] u32 ADC(u32 operand1, u32 operand2, bool change_flags);
    [[nodiscard]] u32 ADD(u32 operand1, u32 operand2, bool change_flags);
    void CMN(u32 operand1, u32 operand2);
    void CMP(u32 operand1, u32 operand2);
    [[nodiscard]] u32 SBC(u32 operand1, u32 operand2, bool change_flags);
    [[nodiscard]] u32 SUB(u32 operand1, u32 operand2, bool change_flags);
    void TEQ(u32 operand1, u32 operand2);
    void TST(u32 operand1, u32 operand2);

    [[nodiscard]] bool CheckConditionCode(u8 cond);
    [[nodiscard]] std::string GetConditionCode(u8 cond);

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

    void ARM_DataProcessing(u32 opcode);
    void ARM_DisassembleDataProcessing(u32 opcode);

    void ARM_MRS(u32 opcode);
    void ARM_DisassembleMRS(u32 opcode);
    template <bool flag_bits_only>
    void ARM_MSR(u32 opcode);
    template <bool flag_bits_only>
    void ARM_DisassembleMSR(u32 opcode);

    void ARM_Multiply(u32 opcode);
    void ARM_DisassembleMultiply(u32 opcode);

    void ARM_MultiplyLong(u32 opcode);
    void ARM_DisassembleMultiplyLong(u32 opcode);

    void ARM_SingleDataSwap(u32 opcode);
    void ARM_DisassembleSingleDataSwap(u32 opcode);

    void ARM_BranchAndExchange(u32 opcode);
    void ARM_DisassembleBranchAndExchange(u32 opcode);

    void ARM_HalfwordDataTransferRegister(u32 opcode);
    void ARM_DisassembleHalfwordDataTransferRegister(u32 opcode);
    template <bool load_from_memory, bool transfer_halfword>
    void ARM_HalfwordDataTransferRegister_Impl(u32 opcode, bool sign);

    void ARM_HalfwordDataTransferImmediate(u32 opcode);
    void ARM_DisassembleHalfwordDataTransferImmediate(u32 opcode);
    void ARM_LoadHalfwordImmediate(u32 opcode, bool sign);
    void ARM_StoreHalfwordImmediate(u32 opcode, bool sign);
    void ARM_LoadSignedByte(u32 opcode);

    void ARM_SingleDataTransfer(u32 opcode);
    void ARM_DisassembleSingleDataTransfer(u32 opcode);
    template <bool load_from_memory, bool transfer_byte>
    void ARM_SingleDataTransfer_Impl(u32 opcode);

    void ARM_BlockDataTransfer(u32 opcode);
    void ARM_DisassembleBlockDataTransfer(u32 opcode);

    void ARM_Branch(u32 opcode);
    void ARM_DisassembleBranch(u32 opcode);
    
    // TODO: Coprocessor data transfer

    // TODO: Coprocessor data operation

    // TODO: Coprocessor register transfer

    void ARM_SoftwareInterrupt(u32 opcode);
    void ARM_DisassembleSoftwareInterrupt(u32 opcode);

    void Thumb_MoveShiftedRegister(u16 opcode);
    void Thumb_DisassembleMoveShiftedRegister(u16 opcode);
    void Thumb_LSL(u16 opcode);
    void Thumb_LSR(u16 opcode);
    void Thumb_ASR(u16 opcode);

    void Thumb_AddSubtract(u16 opcode);
    void Thumb_DisassembleAddSubtract(u16 opcode);

    void Thumb_MoveCompareAddSubtractImmediate(u16 opcode);
    void Thumb_DisassembleMoveCompareAddSubtractImmediate(u16 opcode);

    void Thumb_ALUOperations(u16 opcode);
    void Thumb_DisassembleALUOperations(u16 opcode);

    void Thumb_HiRegisterOperationsBranchExchange(u16 opcode);
    void Thumb_DisassembleHiRegisterOperationsBranchExchange(u16 opcode);

    void Thumb_PCRelativeLoad(u16 opcode);
    void Thumb_DisassemblePCRelativeLoad(u16 opcode);

    void Thumb_LoadStoreWithRegisterOffset(u16 opcode);
    void Thumb_DisassembleLoadStoreWithRegisterOffset(u16 opcode);

    void Thumb_LoadStoreSignExtendedByteHalfword(u16 opcode);
    void Thumb_DisassembleLoadStoreSignExtendedByteHalfword(u16 opcode);

    void Thumb_LoadStoreWithImmediateOffset(u16 opcode);
    void Thumb_DisassembleLoadStoreWithImmediateOffset(u16 opcode);
    void Thumb_StoreByteWithImmediateOffset(u16 opcode);
    void Thumb_LoadByteWithImmediateOffset(u16 opcode);
    void Thumb_StoreWordWithImmediateOffset(u16 opcode);
    void Thumb_LoadWordWithImmediateOffset(u16 opcode);

    void Thumb_LoadStoreHalfword(u16 opcode);
    void Thumb_DisassembleLoadStoreHalfword(u16 opcode);

    void Thumb_SPRelativeLoadStore(u16 opcode);
    void Thumb_DisassembleSPRelativeLoadStore(u16 opcode);

    void Thumb_LoadAddress(u16 opcode);
    void Thumb_DisassembleLoadAddress(u16 opcode);

    void Thumb_AddOffsetToStackPointer(u16 opcode);
    void Thumb_DisassembleAddOffsetToStackPointer(u16 opcode);

    void Thumb_PushPopRegisters(u16 opcode);
    void Thumb_DisassemblePushPopRegisters(u16 opcode);

    void Thumb_MultipleLoadStore(u16 opcode);
    void Thumb_DisassembleMultipleLoadStore(u16 opcode);

    void Thumb_ConditionalBranch(u16 opcode);
    void Thumb_DisassembleConditionalBranch(u16 opcode);

    void Thumb_SoftwareInterrupt(u16 opcode);
    void Thumb_DisassembleSoftwareInterrupt(u16 opcode);

    void Thumb_UnconditionalBranch(u16 opcode);
    void Thumb_DisassembleUnconditionalBranch(u16 opcode);

    void Thumb_LongBranchWithLink(u16 opcode);
    void Thumb_DisassembleLongBranchWithLink(u16 opcode);
};
