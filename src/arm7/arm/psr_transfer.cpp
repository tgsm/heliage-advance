#include "arm7/arm7.h"

// TODO: from starbreeze:
// "you might have to add some things to your msr later. (e.g. you can't change
// control bits in User mode, there's a mask field that controls access to the
// individual PSR fields that the ARM7TDMI reference doesn't tell you about, ...)"
//
// "msr won't be the only way to set a PSR. There's mode changes and PSR transfers
// in block/single data transfers, CPU exceptions and some data processing instructions."

void ARM7::ARM_MRS(const u32 opcode) {
    const bool source_is_spsr = (opcode >> 22) & 0b1;
    const u8 rd = (opcode >> 12) & 0xF;

    if (source_is_spsr) {
        SetRegister(rd, GetSPSR());
    } else {
        SetRegister(rd, cpsr.raw);
    }
}

void ARM7::ARM_MSR_AllBits(const u32 opcode) {
    const bool destination_is_spsr = (opcode >> 22) & 0b1;
    const u8 rm = opcode & 0xF;

    if (destination_is_spsr) {
        SetSPSR(GetRegister(rm));
    } else {
        cpsr.raw = GetRegister(rm);
    }
}

void ARM7::ARM_MSR_FlagBits(const u32 opcode) {
    const bool destination_is_spsr = (opcode >> 22) & 0b1;
    const bool operand_is_immediate = (opcode >> 25) & 0b1;
    const u16 source_operand = opcode & 0xFFF;

    if (operand_is_immediate) {
        const u8 rotate_amount = (source_operand >> 8) & 0xF;
        const u8 immediate = source_operand & 0xFF;

        if (destination_is_spsr) {
            SetSPSR(GetSPSR() & ~0xFFFFFF00);
            SetSPSR(GetSPSR() | (Shift_RotateRight(immediate, rotate_amount << 1) & 0xFFFFFF00));
        } else {
            cpsr.raw &= ~0xFFFFFF00;
            cpsr.raw |= (Shift_RotateRight(immediate, rotate_amount << 1) & 0xFFFFFF00);
        }
    } else {
        const u8 rm = source_operand & 0xF;
        if (destination_is_spsr) {
            SetSPSR(GetSPSR() & ~0xFFFFFF00);
            SetSPSR(GetSPSR() | (GetRegister(rm) & 0xFFFFFF00));
        } else {
            cpsr.raw &= ~0xFFFFFF00;
            cpsr.raw |= (GetRegister(rm) & 0xFFFFFF00);
        }
    }
}
