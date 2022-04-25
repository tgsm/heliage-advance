#include "common/bits.h"
#include "arm7/arm7.h"

void ARM7::Thumb_ConditionalBranch(const u16 opcode) {
    const std::unsigned_integral auto cond = Common::GetBitRange<11, 8>(opcode);
    const s8 offset = Common::GetBitRange<7, 0>(opcode);

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
        timers.AdvanceCycles(2, Timers::CycleType::Sequential);
        timers.AdvanceCycles(1, Timers::CycleType::Nonsequential);
    }
}

void ARM7::Thumb_SoftwareInterrupt([[maybe_unused]] const u16 opcode) {
    LDEBUG("Thumb-mode SWI at {:08X}", GetPC() - 4);

    const u32 lr = GetPC() - 2;
    const u32 old_cpsr = cpsr.raw;

    cpsr.flags.processor_mode = ProcessorMode::Supervisor;
    SetLR(lr);
    cpsr.flags.thumb_mode = false;
    cpsr.flags.irq_disabled = true;
    SetPC(0x00000008);
    SetSPSR(old_cpsr);

    timers.AdvanceCycles(2, Timers::CycleType::Sequential);
    timers.AdvanceCycles(1, Timers::CycleType::Nonsequential);
}

void ARM7::Thumb_UnconditionalBranch(const u16 opcode) {
    s16 offset = Common::GetBitRange<10, 0>(opcode) << 1;

    // Sign-extend to 16 bits
    offset <<= 4;
    offset >>= 4;

    SetPC(GetPC() + offset);

    timers.AdvanceCycles(2, Timers::CycleType::Sequential);
    timers.AdvanceCycles(1, Timers::CycleType::Nonsequential);
}

void ARM7::Thumb_LongBranchWithLink(const u16 opcode) {
    const u16 next_opcode = bus.Read16(GetPC() - 2);
    s16 offset = Common::GetBitRange<10, 0>(opcode);
    const std::unsigned_integral auto next_offset = Common::GetBitRange<10, 0>(next_opcode);

    u32 pc = GetPC();

    offset <<= 5;
    u32 lr = pc + (static_cast<s32>(offset) << 7);

    pc = lr + (next_offset << 1);
    lr = GetPC() | 0b1;

    SetLR(lr);
    SetPC(pc);

    timers.AdvanceCycles(2, Timers::CycleType::Sequential);
    timers.AdvanceCycles(1, Timers::CycleType::Nonsequential);
}
