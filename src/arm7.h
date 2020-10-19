#pragma once

#include "mmu.h"
#include "types.h"

class ARM7 {
public:
    ARM7(MMU& mmu);

    void Step(bool disassemble);
private:
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

    ARM_Instructions DecodeARMInstruction(const u32 opcode) const;
    void ExecuteARMInstruction(const ARM_Instructions instr, const u32 opcode);
    void DisassembleARMInstruction(const ARM_Instructions instr, const u32 opcode);
    Thumb_Instructions DecodeThumbInstruction(const u16 opcode) const;
    void ExecuteThumbInstruction(const Thumb_Instructions instr, const u16 opcode);
    void DisassembleThumbInstruction(const Thumb_Instructions instr, const u16 opcode);

    void DumpRegisters(const bool thumb);

    void FillPipeline();
    void Die(const char* format, ...);

    u32 Shift(const u8 operand_to_shift, const u8 shift_type, const u8 shift_amount);
    u32 Shift_RotateRight(const u32 operand_to_rotate, const u8 rotate_amount);

    bool CheckConditionCode(const u8 cond);
    std::string GetConditionCode(const u8 cond);

    // Instruction pipeline
    std::array<u32, 2> pipeline;

    // General purpose registers
    std::array<u32, 16> r;

    union {
        u32 raw = 0;
        struct {
            // FIXME: this struct assumes the user is running on a little-endian system.
            u8 processor_mode : 5;
            bool thumb_mode : 1;
            bool fiq : 1;
            bool irq : 1;
            u32 : 20;
            bool overflow : 1;
            bool carry : 1;
            bool zero : 1;
            bool negative : 1;
        } flags;
    } cpsr;

    MMU& mmu;

    void ARM_Branch(const u32 opcode);
    void ARM_DisassembleBranch(const u32 opcode);

    void ARM_DataProcessing(const u32 opcode);
    void ARM_DisassembleDataProcessing(const u32 opcode);

    void ARM_SingleDataTransfer(const u32 opcode);
    void ARM_DisassembleSingleDataTransfer(const u32 opcode);
    void ARM_LoadWord(const u32 opcode);
    void ARM_StoreWord(const u32 opcode);
    void ARM_LoadByte(const u32 opcode);
    void ARM_StoreByte(const u32 opcode);

    void ARM_MSR(const u32 opcode);
    void ARM_DisassembleMSR(const u32 opcode);

    void ARM_BranchAndExchange(const u32 opcode);
    void ARM_DisassembleBranchAndExchange(const u32 opcode);

    void ARM_HalfwordDataTransferImmediate(const u32 opcode);
    void ARM_DisassembleHalfwordDataTransferImmediate(const u32 opcode);
    void ARM_HalfwordDataTransferRegister(const u32 opcode);
    void ARM_DisassembleHalfwordDataTransferRegister(const u32 opcode);
    void ARM_StoreHalfwordImmediate(const u32 opcode, const bool sign);
    void ARM_StoreHalfwordRegister(const u32 opcode, const bool sign);
    void ARM_LoadHalfwordImmediate(const u32 opcode, const bool sign);

    void ARM_BlockDataTransfer(const u32 opcode);
    void ARM_DisassembleBlockDataTransfer(const u32 opcode);

    void ARM_Multiply(const u32 opcode);
    void ARM_DisassembleMultiply(const u32 opcode);

    void Thumb_PCRelativeLoad(const u16 opcode);
    void Thumb_DisassemblePCRelativeLoad(const u16 opcode);

    void Thumb_MoveShiftedRegister(const u16 opcode);
    void Thumb_DisassembleMoveShiftedRegister(const u16 opcode);
    void Thumb_LSL(const u16 opcode);
    void Thumb_LSR(const u16 opcode);
    void Thumb_ASR(const u16 opcode);

    void Thumb_ConditionalBranch(const u16 opcode);
    void Thumb_DisassembleConditionalBranch(const u16 opcode);

    void Thumb_MoveCompareAddSubtractImmediate(const u16 opcode);
    void Thumb_DisassembleMoveCompareAddSubtractImmediate(const u16 opcode);

    void Thumb_LongBranchWithLink(const u16 opcode);
    void Thumb_DisassembleLongBranchWithLink(const u16 opcode);

    void Thumb_AddSubtract(const u16 opcode);
    void Thumb_DisassembleAddSubtract(const u16 opcode);

    void Thumb_ALUOperations(const u16 opcode);
    void Thumb_DisassembleALUOperations(const u16 opcode);

    void Thumb_MultipleLoadStore(const u16 opcode);
    void Thumb_DisassembleMultipleLoadStore(const u16 opcode);

    void Thumb_HiRegisterOperationsBranchExchange(const u16 opcode);
    void Thumb_DisassembleHiRegisterOperationsBranchExchange(const u16 opcode);

    void Thumb_LoadStoreWithImmediateOffset(const u16 opcode);
    void Thumb_DisassembleLoadStoreWithImmediateOffset(const u16 opcode);
    void Thumb_StoreWordWithImmediateOffset(const u16 opcode);

    void Thumb_PushPopRegisters(const u16 opcode);
    void Thumb_DisassemblePushPopRegisters(const u16 opcode);

    void Thumb_UnconditionalBranch(const u16 opcode);
    void Thumb_DisassembleUnconditionalBranch(const u16 opcode);

    void Thumb_LoadAddress(const u16 opcode);
    void Thumb_DisassembleLoadAddress(const u16 opcode);
};
