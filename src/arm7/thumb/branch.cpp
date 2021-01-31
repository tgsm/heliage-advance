#include "arm7/arm7.h"

void ARM7::Thumb_ConditionalBranch(const u16 opcode) {
    const u8 cond = (opcode >> 8) & 0xF;
    const s8 offset = opcode & 0xFF;

    bool condition = false;
    switch (cond) {
        case 0x0: // EQ
            condition = cpsr.flags.zero;
            break;
        case 0x1: // NE
            condition = !cpsr.flags.zero;
            break;
        case 0x2: // CS
            condition = cpsr.flags.carry;
            break;
        case 0x3: // CC
            condition = !cpsr.flags.carry;
            break;
        case 0x4: // MI
            condition = cpsr.flags.negative;
            break;
        case 0x5: // PL
            condition = !cpsr.flags.negative;
            break;
        case 0x6: // VS
            condition = cpsr.flags.overflow;
            break;
        case 0x7: // VC
            condition = !cpsr.flags.overflow;
            break;
        case 0x8: // HI
            condition = (cpsr.flags.carry && !cpsr.flags.zero);
            break;
        case 0x9: // LS
            condition = (!cpsr.flags.carry || cpsr.flags.zero);
            break;
        case 0xA: // GE
            condition = (cpsr.flags.negative == cpsr.flags.overflow);
            break;
        case 0xB: // LT
            condition = (cpsr.flags.negative != cpsr.flags.overflow);
            break;
        case 0xC: // GT
            condition = (!cpsr.flags.zero && (cpsr.flags.negative == cpsr.flags.overflow));
            break;
        case 0xD: // LE
            condition = (cpsr.flags.zero || (cpsr.flags.negative != cpsr.flags.overflow));
            break;
        default:
            UNREACHABLE_MSG("interpreter: invalid thumb conditional branch condition 0x{:X}", cond);
    }

    if (condition) {
        SetPC(GetPC() + (offset << 1));
    }
}

void ARM7::Thumb_SoftwareInterrupt([[maybe_unused]] const u16 opcode) {
    LDEBUG("Thumb-mode SWI at {:08X}", GetPC() - 4);

    const u32 lr = GetPC() - 2;
    const u32 old_cpsr = cpsr.raw;

    cpsr.flags.processor_mode = ProcessorMode::Supervisor;
    SetLR(lr);
    cpsr.flags.thumb_mode = false;
    cpsr.flags.irq = true;
    SetPC(0x00000008);
    SetSPSR(old_cpsr);
}

void ARM7::Thumb_UnconditionalBranch(const u16 opcode) {
    s16 offset = (opcode & 0x7FF) << 1;

    // Sign-extend to 16 bits
    offset <<= 4;
    offset >>= 4;

    SetPC(GetPC() + offset);
}

void ARM7::Thumb_LongBranchWithLink(const u16 opcode) {
    const u16 next_opcode = mmu.Read16(GetPC() - 2);
    s16 offset = opcode & 0x7FF;
    const u16 next_offset = next_opcode & 0x7FF;

    u32 lr = GetLR();
    u32 pc = GetPC();

    offset <<= 5;
    lr = pc + (static_cast<s32>(offset) << 7);

    pc = lr + (next_offset << 1);
    lr = GetPC() | 0b1;

    SetLR(lr);
    SetPC(pc);
}
