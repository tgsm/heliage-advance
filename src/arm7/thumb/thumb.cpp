#if defined(__GNUC__) && !defined(__clang__)
#include <ranges>
#endif
#include "arm7/arm7.h"

void ARM7::Thumb_MoveShiftedRegister(const u16 opcode) {
    const u8 op = (opcode >> 11) & 0x3;
    switch (op) {
        case 0:
            Thumb_LSL(opcode);
            break;
        case 1:
            Thumb_LSR(opcode);
            break;
        case 2:
            Thumb_ASR(opcode);
            break;
        default:
            UNREACHABLE();
    }
}

void ARM7::Thumb_LSL(const u16 opcode) {
    u8 offset = (opcode >> 6) & 0x1F;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    const u32 source = GetRegister(rs);

    if (!offset) {
        offset = 32;
    }

    SetRegister(rd, Shift(source, ShiftType::LSL, offset));

    cpsr.flags.negative = GetRegister(rd) & (1 << 31);
    cpsr.flags.zero = (GetRegister(rd) == 0);
}

void ARM7::Thumb_LSR(const u16 opcode) {
    u8 offset = (opcode >> 6) & 0x1F;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    const u32 source = GetRegister(rs);

    if (!offset) {
        offset = 32;
    }

    SetRegister(rd, Shift(source, ShiftType::LSR, offset));

    cpsr.flags.negative = GetRegister(rd) & (1 << 31);
    cpsr.flags.zero = (GetRegister(rd) == 0);
}

void ARM7::Thumb_ASR(const u16 opcode) {
    u8 offset = (opcode >> 6) & 0x1F;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;
    const u32 source = GetRegister(rs);

    if (!offset) {
        offset = 32;
    }

    SetRegister(rd, Shift(source, ShiftType::ASR, offset));

    cpsr.flags.negative = GetRegister(rd) & (1 << 31);
    cpsr.flags.zero = (GetRegister(rd) == 0);
    LWARN("verify ASR's carry flag");
}

void ARM7::Thumb_AddSubtract(const u16 opcode) {
    const bool operand_is_immediate = (opcode >> 10) & 0b1;
    const bool subtracting = (opcode >> 9) & 0b1;
    const u8 rn_or_immediate = (opcode >> 6) & 0x7;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    if (subtracting) {
        if (operand_is_immediate) {
            SetRegister(rd, SUB(GetRegister(rs), rn_or_immediate, true));
        } else {
            SetRegister(rd, SUB(GetRegister(rs), GetRegister(rn_or_immediate), true));
        }
    } else {
        if (operand_is_immediate) {
            SetRegister(rd, ADD(GetRegister(rs), rn_or_immediate, true));
        } else {
            SetRegister(rd, ADD(GetRegister(rs), GetRegister(rn_or_immediate), true));
        }
    }
}

void ARM7::Thumb_MoveCompareAddSubtractImmediate(const u16 opcode) {
    const u8 op = (opcode >> 11) & 0x3;
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 offset = opcode & 0xFF;

    switch (op) {
        case 0x0:
            SetRegister(rd, offset);
            break;
        case 0x1:
            cpsr.flags.zero = (GetRegister(rd) == offset);
            return;
        case 0x2:
            SetRegister(rd, ADD(GetRegister(rd), offset, true));
            break;
        case 0x3:
            SetRegister(rd, SUB(GetRegister(rd), offset, true));
            break;

        default:
            UNREACHABLE_MSG("interpreter: illegal thumb MCASI op 0x{:X}", op);
    }

    cpsr.flags.negative = (GetRegister(rd) & (1 << 31));
    cpsr.flags.zero = (GetRegister(rd) == 0);
}

void ARM7::Thumb_ALUOperations(const u16 opcode) {
    const u8 op = (opcode >> 6) & 0xF;
    const u8 rs = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    switch (op) {
        case 0x0: // AND
            SetRegister(rd, GetRegister(rd) & GetRegister(rs));
            break;
        case 0x1: // EOR
            SetRegister(rd, GetRegister(rd) ^ GetRegister(rs));
            break;
        case 0x2: // LSL
            SetRegister(rd, Shift(GetRegister(rd), ShiftType::LSL, GetRegister(rs)));
            break;
        case 0x3: // LSR
            SetRegister(rd, Shift(GetRegister(rd), ShiftType::LSR, GetRegister(rs)));
            break;
        case 0x4: // ASR
            SetRegister(rd, Shift(GetRegister(rd), ShiftType::ASR, GetRegister(rs)));
            break;
        case 0x5: // ADC
            SetRegister(rd, ADC(GetRegister(rd), GetRegister(rs), true));
            break;
        case 0x7: // ROR
            SetRegister(rd, Shift(GetRegister(rd), ShiftType::ROR, GetRegister(rs)));
            break;
        case 0x8: { // TST
            u32 result = GetRegister(rd) & GetRegister(rs);
            cpsr.flags.zero = (result == 0);
            cpsr.flags.negative = (result & (1 << 31));
            return;
        }
        case 0x9: // NEG
            SetRegister(rd, -GetRegister(rs));
            break;
        case 0xA: // CMP
            CMP(GetRegister(rd), GetRegister(rs));
            return;
        case 0xB: // CMN
            CMN(GetRegister(rd), GetRegister(rs));
            return;
        case 0xC: // ORR
            SetRegister(rd, GetRegister(rd) | GetRegister(rs));
            break;
        case 0xD: // MUL
            SetRegister(rd, GetRegister(rd) * GetRegister(rs));
            break;
        case 0xE: // BIC
            SetRegister(rd, GetRegister(rd) & ~GetRegister(rs));
            break;
        case 0xF: // MVN
            SetRegister(rd, ~GetRegister(rs));
            break;
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb alu op 0x{:X}", op);
    }

    cpsr.flags.negative = (GetRegister(rd) & (1 << 31));
    cpsr.flags.zero = (GetRegister(rd) == 0);
}

void ARM7::Thumb_HiRegisterOperationsBranchExchange(const u16 opcode) {
    const u8 op = (opcode >> 8) & 0x3;
    const bool h1 = (opcode >> 7) & 0b1;
    const bool h2 = (opcode >> 6) & 0b1;
    u8 rs_hs = (opcode >> 3) & 0x7;
    u8 rd_hd = opcode & 0x7;

    ASSERT(!(op == 0x3 && h1));

    if (h1) {
        rd_hd += 8;
    }

    if (h2) {
        rs_hs += 8;
    }

    switch (op) {
        case 0x0:
            SetRegister(rd_hd, GetRegister(rd_hd) + GetRegister(rs_hs));
            // If we're setting R15 through here, we need to halfword align it
            // and refill the pipeline.
            if (rd_hd == 15) {
                SetPC((GetPC() - 2) & ~0b1);
            }
            break;
        case 0x1:
            CMP(GetRegister(rd_hd), GetRegister(rs_hs));
            return;
        case 0x2:
            SetRegister(rd_hd, GetRegister(rs_hs));
            // If we're setting R15 through here, we need to halfword align it
            // and refill the pipeline.
            if (rd_hd == 15) {
                SetPC((GetPC() - 2) & ~0b1);
            }
            break;
        case 0x3:
            cpsr.flags.thumb_mode = GetRegister(rs_hs) & 0b1;
            SetPC(GetRegister(rs_hs) & ~0b1);
            return;
        default:
            UNREACHABLE();
    }

    cpsr.flags.negative = (GetRegister(rd_hd) & (1 << 31));
    cpsr.flags.zero = (GetRegister(rd_hd) == 0);
}

void ARM7::Thumb_PCRelativeLoad(const u16 opcode) {
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 imm = opcode & 0xFF;

    SetRegister(rd, mmu.Read32((GetPC() + (imm << 2)) & ~0x3));
}

void ARM7::Thumb_LoadStoreWithRegisterOffset(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const bool transfer_byte = (opcode >> 10) & 0b1;
    const u8 ro = (opcode >> 6) & 0x7;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    if (load_from_memory) {
        if (transfer_byte) {
            SetRegister(rd, mmu.Read8(GetRegister(rb) + GetRegister(ro)));
        } else {
            SetRegister(rd, mmu.Read32(GetRegister(rb) + GetRegister(ro)));
        }
    } else {
        if (transfer_byte) {
            mmu.Write8(GetRegister(rb) + GetRegister(ro), GetRegister(rd) & 0xFF);
        } else {
            mmu.Write32(GetRegister(rb) + GetRegister(ro), GetRegister(rd));
        }
    }
}

void ARM7::Thumb_LoadStoreSignExtendedByteHalfword(const u16 opcode) {
    const bool h_flag = (opcode >> 11) & 0b1;
    const bool sign_extend = (opcode >> 10) & 0b1;
    const u8 ro = (opcode >> 6) & 0x7;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    if (sign_extend) {
        if (h_flag) {
            SetRegister(rd, mmu.Read16(GetRegister(rb) + GetRegister(ro)));

            // Set bits 31-16 of Rd to what's in bit 15.
            if (!(GetRegister(rd) & (1 << 15))) {
                return;
            }

            for (u8 i = 16; i < 31; i++) {
                SetRegister(rd, GetRegister(rd) | (1 << i));
            }
        } else {
            SetRegister(rd, mmu.Read8(GetRegister(rb) + GetRegister(ro)));

            // Set bits 31-8 of Rd to what's in bit 15.
            if (!(GetRegister(rd) & (1 << 7))) {
                return;
            }

            for (u8 i = 8; i < 31; i++) {
                SetRegister(rd, GetRegister(rd) | (1 << i));
            }
        }
    } else {
        if (h_flag) {
            SetRegister(rd, mmu.Read16(GetRegister(rb) + GetRegister(ro)));
        } else {
            UNIMPLEMENTED_MSG("unimplemented thumb store (not sign-extended) halfword");
        }
    }
}

void ARM7::Thumb_LoadStoreWithImmediateOffset(const u16 opcode) {
    const bool transfer_byte = (opcode >> 12) & 0b1;
    const bool load_from_memory = (opcode >> 11) & 0b1;

    if (transfer_byte) {
        if (load_from_memory) {
            Thumb_LoadByteWithImmediateOffset(opcode);
        } else {
            Thumb_StoreByteWithImmediateOffset(opcode);
        }
    } else {
        if (load_from_memory) {
            Thumb_LoadWordWithImmediateOffset(opcode);
        } else {
            Thumb_StoreWordWithImmediateOffset(opcode);
        }
    }
}

void ARM7::Thumb_StoreByteWithImmediateOffset(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    mmu.Write8(GetRegister(rd), GetRegister(rb) + offset);
}

void ARM7::Thumb_LoadByteWithImmediateOffset(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    SetRegister(rd, mmu.Read8(GetRegister(rb) + offset));
}

void ARM7::Thumb_StoreWordWithImmediateOffset(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    mmu.Write32(GetRegister(rd), GetRegister(rb) + offset);
}

void ARM7::Thumb_LoadWordWithImmediateOffset(const u16 opcode) {
    const u8 offset = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    SetRegister(rd, mmu.Read32(GetRegister(rb) + offset));
}

void ARM7::Thumb_LoadStoreHalfword(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const u8 imm = (opcode >> 6) & 0x1F;
    const u8 rb = (opcode >> 3) & 0x7;
    const u8 rd = opcode & 0x7;

    if (load_from_memory) {
        SetRegister(rd, mmu.Read16(GetRegister(rb) + imm));
    } else {
        mmu.Write16(GetRegister(rb) + imm, static_cast<u16>(GetRegister(rd)));
    }
}

void ARM7::Thumb_SPRelativeLoadStore(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 imm = opcode & 0xFF;

    if (load_from_memory) {
        SetRegister(rd, mmu.Read32(GetSP() + (imm << 2)));
    } else {
        mmu.Write32(GetSP() + (imm << 2), GetRegister(rd));
    }
}

void ARM7::Thumb_LoadAddress(const u16 opcode) {
    const bool load_from_sp = (opcode >> 11) & 0b1;
    const u8 rd = (opcode >> 8) & 0x7;
    const u8 imm = opcode & 0xFF;

    SetRegister(rd, (imm << 2) + (load_from_sp ? GetSP() : GetPC() & ~0b11));
}

void ARM7::Thumb_AddOffsetToStackPointer(const u16 opcode) {
    const bool offset_is_negative = (opcode >> 7) & 0b1;
    const u8 imm = opcode & 0x7F;

    if (offset_is_negative) {
        SetSP(GetSP() - imm);
    } else {
        SetSP(GetSP() + imm);
    }
}

void ARM7::Thumb_PushPopRegisters(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const bool store_lr_load_pc = (opcode >> 8) & 0b1;
    const u8 rlist = opcode & 0xFF;

    // Go through `rlist`'s 8 bits, and write down which bits are set. This tells
    // us which registers we need to load/store.
    // If bit 0 is set, that corresponds to R0. If bit 1 is set, that corresponds
    // to R1, and so on.
    std::vector<u8> set_bits;
    for (u8 i = 0; i < 8; i++) {
        if (rlist & (1 << i)) {
            set_bits.push_back(i);
        }
    }

    if (set_bits.empty()) {
        if (!store_lr_load_pc) {
            return;
        }

        if (load_from_memory) {
            SetPC(mmu.Read32(GetSP()) & ~0b1);
            SetSP(GetSP() + 4);
        } else {
            mmu.Write32(GetSP(), GetLR());
            SetSP(GetSP() - 4);
        }

        return;
    }

    if (load_from_memory) {
        for (u8 reg : set_bits) {
            SetRegister(reg, mmu.Read32(GetSP()));
            SetSP(GetSP() + 4);
        }

        if (store_lr_load_pc) {
            SetPC(mmu.Read32(GetSP()) & ~0b1);
            SetSP(GetSP() + 4);
        }
    } else {
        if (store_lr_load_pc) {
            SetSP(GetSP() - 4);
            mmu.Write32(GetSP(), GetLR());
        }

// TODO: Remove this when Clang supports the ranges library.
#if defined(__GNUC__) && !defined(__clang__)
        for (u8 reg : set_bits | std::views::reverse) {
            SetSP(GetSP() - 4);
            mmu.Write32(GetSP(), GetRegister(reg));
        }
#else
        for (int i = set_bits.size() - 1; i >= 0; i--) {
            SetSP(GetSP() - 4);
            mmu.Write32(GetSP(), GetRegister(set_bits[i]));
        }
#endif
    }
}

void ARM7::Thumb_MultipleLoadStore(const u16 opcode) {
    const bool load_from_memory = (opcode >> 11) & 0b1;
    const u8 rb = (opcode >> 8) & 0x7;
    const u8 rlist = opcode & 0xFF;

    // Go through `rlist`'s 8 bits, and write down which bits are set. This tells
    // us which registers we need to load/store.
    // If bit 0 is set, that corresponds to R0. If bit 1 is set, that corresponds
    // to R1, and so on.
    std::vector<u8> set_bits;
    for (u8 i = 0; i < 8; i++) {
        if (rlist & (1 << i)) {
            set_bits.push_back(i);
        }
    }

    if (set_bits.empty()) {
        return;
    }

    if (load_from_memory) {
        for (u8 i : set_bits) {
            SetRegister(i, mmu.Read32(GetRegister(rb)));
            SetRegister(rb, GetRegister(rb) + 4);
        }
    } else {
        for (u8 reg : set_bits) {
            mmu.Write32(GetRegister(rb), GetRegister(reg));
            SetRegister(rb, GetRegister(rb) + 4);
        }
    }
}

void ARM7::Thumb_ConditionalBranch(const u16 opcode) {
    const u8 cond = (opcode >> 8) & 0xF;
    const s8 offset = opcode & 0xFF;

    bool condition = false;
    switch (cond) {
        case 0x0:
            condition = cpsr.flags.zero;
            break;
        case 0x1:
            condition = !cpsr.flags.zero;
            break;
        case 0x2:
            condition = cpsr.flags.carry;
            break;
        case 0x3:
            condition = !cpsr.flags.carry;
            break;
        case 0x4:
            condition = cpsr.flags.negative;
            break;
        case 0x5:
            condition = !cpsr.flags.negative;
            break;
        case 0x6:
            condition = cpsr.flags.overflow;
            break;
        case 0x7:
            condition = !cpsr.flags.overflow;
            break;
        case 0x8:
            condition = (cpsr.flags.carry && !cpsr.flags.zero);
            break;
        case 0x9:
            condition = (!cpsr.flags.carry || cpsr.flags.zero);
            break;
        case 0xB:
            condition = (cpsr.flags.negative != cpsr.flags.overflow);
            break;
        default:
            UNIMPLEMENTED_MSG("interpreter: unimplemented thumb conditional branch condition 0x{:X}", cond);
    }

    if (condition) {
        SetPC(GetPC() + (offset << 1));
    }
}

// TODO: Thumb-mode software interrupt

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
