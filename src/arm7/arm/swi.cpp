#include "arm7/arm7.h"

void ARM7::ARM_SoftwareInterrupt([[maybe_unused]] const u32 opcode) {
    LDEBUG("ARM-mode SWI at {:08X}", GetPC() - 8);

    u32 lr = GetPC() - 4;
    u32 old_cpsr = cpsr.raw;

    cpsr.flags.processor_mode = ProcessorMode::Supervisor;
    SetLR(lr);
    cpsr.flags.irq = true;
    SetPC(0x00000008);
    SetSPSR(old_cpsr);
}
