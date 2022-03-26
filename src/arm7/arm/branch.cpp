#include "common/bits.h"
#include "arm7/arm7.h"

void ARM7::ARM_BranchAndExchange(const u32 opcode) {
    const u8 rn = Common::GetBitRange<3, 0>(opcode);

    // If bit 0 of Rn is set, we switch to THUMB mode. Else, we switch to ARM mode.
    cpsr.flags.thumb_mode = Common::IsBitSet<0>(GetRegister(rn));
    LDEBUG("thumb: {}", cpsr.flags.thumb_mode);

    SetPC(GetRegister(rn) & ~0b1);
}

void ARM7::ARM_Branch(const u32 opcode) {
    const bool link = Common::IsBitSet<24>(opcode);
    s32 offset = (Common::GetBitRange<23, 0>(opcode)) << 2;

    offset <<= 6;
    offset >>= 6;

    if (link) {
        SetLR(GetPC() - 4);
    }

    SetPC(GetPC() + offset);
}

void ARM7::ARM_SoftwareInterrupt([[maybe_unused]] const u32 opcode) {
    LDEBUG("ARM-mode SWI at {:08X}", GetPC() - 8);

    u32 lr = GetPC() - 4;
    u32 old_cpsr = cpsr.raw;

    cpsr.flags.processor_mode = ProcessorMode::Supervisor;
    SetLR(lr);
    cpsr.flags.irq_disabled = true;
    SetPC(0x00000008);
    SetSPSR(old_cpsr);
}
