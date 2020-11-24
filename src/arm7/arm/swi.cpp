#include "arm7/arm7.h"

void ARM7::ARM_SoftwareInterrupt(const u32 opcode) {
    // LDEBUG("ARM-mode SWI at %08X", GetPC() - 8);
    // cpsr.flags.processor_mode = ProcessorMode::Supervisor;
    // SetLR(GetPC() - 4);
    // cpsr.flags.irq = true;
    // SetPC(0x00000008);

    // spsr.raw = cpsr.raw;
    u8 interrupt = (opcode >> 16) & 0xFF;
    switch (interrupt) {
        case 0x06:
            ARM_SWI_Div();
            break;

        default:
            UNIMPLEMENTED_MSG("unimplemented software interrupt 0x%02X", interrupt);
    }
}

void ARM7::ARM_SWI_Div() {
    s32 numerator = GetRegister(0);
    s32 denominator = GetRegister(1);
    SetRegister(0, numerator / denominator);
    SetRegister(1, numerator % denominator);
    SetRegister(3, std::abs(numerator / denominator));
}
