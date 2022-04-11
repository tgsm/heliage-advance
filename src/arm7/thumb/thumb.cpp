#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/reverse.hpp>
#include "common/bits.h"
#include "arm7/arm7.h"

void ARM7::Thumb_MoveShiftedRegister(const u16 opcode) {
    const std::unsigned_integral auto op = Common::GetBitRange<12, 11>(opcode);
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
    const std::unsigned_integral auto offset = Common::GetBitRange<10, 6>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);
    const u32 source = GetRegister(rs);

    SetRegister(rd, Shift(source, ShiftType::LSL, offset, true));

    cpsr.flags.negative = Common::IsBitSet<31>(GetRegister(rd));
    cpsr.flags.zero = (GetRegister(rd) == 0);
}

void ARM7::Thumb_LSR(const u16 opcode) {
    std::unsigned_integral auto offset = Common::GetBitRange<10, 6>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);
    const u32 source = GetRegister(rs);

    if (!offset) {
        offset = 32;
    }

    SetRegister(rd, Shift(source, ShiftType::LSR, offset, true));

    cpsr.flags.negative = Common::IsBitSet<31>(GetRegister(rd));
    cpsr.flags.zero = (GetRegister(rd) == 0);
}

void ARM7::Thumb_ASR(const u16 opcode) {
    std::unsigned_integral auto offset = Common::GetBitRange<10, 6>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);
    const u32 source = GetRegister(rs);

    if (!offset) {
        offset = 32;
    }

    SetRegister(rd, Shift(source, ShiftType::ASR, offset, true));

    cpsr.flags.negative = Common::IsBitSet<31>(GetRegister(rd));
    cpsr.flags.zero = (GetRegister(rd) == 0);
}

void ARM7::Thumb_AddSubtract(const u16 opcode) {
    const bool operand_is_immediate = Common::IsBitSet<10>(opcode);
    const bool subtracting = Common::IsBitSet<9>(opcode);
    const std::unsigned_integral auto rn_or_immediate = Common::GetBitRange<8, 6>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);

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
    const std::unsigned_integral auto op = Common::GetBitRange<12, 11>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<10, 8>(opcode);
    const std::unsigned_integral auto offset = Common::GetBitRange<7, 0>(opcode);

    switch (op) {
        case 0x0:
            SetRegister(rd, offset);
            cpsr.flags.negative = Common::IsBitSet<31>(GetRegister(rd));
            cpsr.flags.zero = (GetRegister(rd) == 0);
            break;
        case 0x1:
            CMP(GetRegister(rd), offset);
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
}

void ARM7::Thumb_ALUOperations(const u16 opcode) {
    const std::unsigned_integral auto op = Common::GetBitRange<9, 6>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);

    switch (op) {
        case 0x0: // AND
            SetRegister(rd, GetRegister(rd) & GetRegister(rs));
            break;
        case 0x1: // EOR
            SetRegister(rd, GetRegister(rd) ^ GetRegister(rs));
            break;
        case 0x2: // LSL
            SetRegister(rd, Shift(GetRegister(rd), ShiftType::LSL, GetRegister(rs), true));
            break;
        case 0x3: // LSR
            SetRegister(rd, Shift(GetRegister(rd), ShiftType::LSR, GetRegister(rs), true));
            break;
        case 0x4: // ASR
            SetRegister(rd, Shift(GetRegister(rd), ShiftType::ASR, GetRegister(rs), true));
            break;
        case 0x5: // ADC
            SetRegister(rd, ADC(GetRegister(rd), GetRegister(rs), true));
            break;
        case 0x6: // SBC
            SetRegister(rd, SBC(GetRegister(rd), GetRegister(rs), true));
            break;
        case 0x7: // ROR
            SetRegister(rd, Shift(GetRegister(rd), ShiftType::ROR, GetRegister(rs), true));
            break;
        case 0x8: // TST
            TST(GetRegister(rd), GetRegister(rs));
            return;
        case 0x9: // NEG
            SetRegister(rd, SUB(0, GetRegister(rs), true));
            return;
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
            UNREACHABLE();
    }

    cpsr.flags.negative = Common::IsBitSet<31>(GetRegister(rd));
    cpsr.flags.zero = (GetRegister(rd) == 0);
}

void ARM7::Thumb_HiRegisterOperationsBranchExchange(const u16 opcode) {
    const std::unsigned_integral auto op = Common::GetBitRange<9, 8>(opcode);
    const bool h1 = Common::IsBitSet<7>(opcode);
    const bool h2 = Common::IsBitSet<6>(opcode);
    const std::unsigned_integral auto rs_hs = Common::GetBitRange<5, 3>(opcode) + (h2 ? 8u : 0u);
    const std::unsigned_integral auto rd_hd = Common::GetBitRange<2, 0>(opcode) + (h1 ? 8u : 0u);

    ASSERT(!(op == 0x3 && h1));

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
            cpsr.flags.thumb_mode = Common::IsBitSet<0>(GetRegister(rs_hs));
            SetPC(GetRegister(rs_hs) & ~0b1);
            return;
        default:
            UNREACHABLE();
    }

    cpsr.flags.negative = Common::IsBitSet<31>(GetRegister(rd_hd));
    cpsr.flags.zero = (GetRegister(rd_hd) == 0);
}

void ARM7::Thumb_PCRelativeLoad(const u16 opcode) {
    const std::unsigned_integral auto rd = Common::GetBitRange<10, 8>(opcode);
    const std::unsigned_integral auto imm = Common::GetBitRange<7, 0>(opcode);

    SetRegister(rd, bus.Read32((GetPC() + (imm << 2)) & ~0x3));
}

void ARM7::Thumb_LoadStoreWithRegisterOffset(const u16 opcode) {
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const bool transfer_byte = Common::IsBitSet<10>(opcode);
    const std::unsigned_integral auto ro = Common::GetBitRange<8, 6>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);

    const u32 address = GetRegister(rb) + GetRegister(ro);
    const bool address_is_word_aligned = ((address & 0b11) == 0);

    if (load_from_memory) {
        if (transfer_byte) {
            SetRegister(rd, bus.Read8(address));
        } else {
            if (address_is_word_aligned) {
                SetRegister(rd, bus.Read32(address));
            } else {
                const u32 value = std::rotr(bus.Read32(address & ~0b11), (address & 0b11) * 8);
                SetRegister(rd, value);
            }
        }
    } else {
        if (transfer_byte) {
            bus.Write8(address, GetRegister(rd));
        } else {
            if (address_is_word_aligned) {
                bus.Write32(address, GetRegister(rd));
            } else {
                bus.Write32(address & ~0b11, GetRegister(rd));
            }
        }
    }
}

void ARM7::Thumb_LoadStoreSignExtendedByteHalfword(const u16 opcode) {
    const bool h_flag = Common::IsBitSet<11>(opcode);
    const bool sign_extend = Common::IsBitSet<10>(opcode);
    const std::unsigned_integral auto ro = Common::GetBitRange<8, 6>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);

    if (sign_extend) {
        if (h_flag) {
            s32 rd_se = bus.Read16(GetRegister(rb) + GetRegister(ro));
            rd_se <<= 16;
            rd_se >>= 16;
            SetRegister(rd, rd_se);
        } else {
            s32 rd_se = bus.Read8(GetRegister(rb) + GetRegister(ro));
            rd_se <<= 24;
            rd_se >>= 24;
            SetRegister(rd, rd_se);
        }
    } else {
        if (h_flag) {
            SetRegister(rd, bus.Read16(GetRegister(rb) + GetRegister(ro)));
        } else {
            bus.Write16(GetRegister(rb) + GetRegister(ro), GetRegister(rd));
        }
    }
}

void ARM7::Thumb_LoadStoreWithImmediateOffset(const u16 opcode) {
    const bool transfer_byte = Common::IsBitSet<12>(opcode);
    const bool load_from_memory = Common::IsBitSet<11>(opcode);

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
    const std::unsigned_integral auto offset = Common::GetBitRange<10, 6>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);

    bus.Write8(GetRegister(rb) + offset, GetRegister(rd));
}

void ARM7::Thumb_LoadByteWithImmediateOffset(const u16 opcode) {
    const std::unsigned_integral auto offset = Common::GetBitRange<10, 6>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);

    SetRegister(rd, bus.Read8(GetRegister(rb) + offset));
}

void ARM7::Thumb_StoreWordWithImmediateOffset(const u16 opcode) {
    const std::unsigned_integral auto offset = Common::GetBitRange<10, 6>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);

    const u32 address = GetRegister(rb) + (offset << 2);
    bus.Write32(address & ~0b11, GetRegister(rd));
}

void ARM7::Thumb_LoadWordWithImmediateOffset(const u16 opcode) {
    const std::unsigned_integral auto offset = Common::GetBitRange<10, 6>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);

    const u32 address = GetRegister(rb) + (offset << 2);
    const bool address_is_word_aligned = ((address & 0b11) == 0);

    if (address_is_word_aligned) {
        SetRegister(rd, bus.Read32(address));
    } else {
        const u32 value = std::rotr(bus.Read32(address & ~0b11), (address & 0b11) * 8);
        SetRegister(rd, value);
    }
}

void ARM7::Thumb_LoadStoreHalfword(const u16 opcode) {
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const std::unsigned_integral auto imm = Common::GetBitRange<10, 6>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<5, 3>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<2, 0>(opcode);

    const u32 address = GetRegister(rb) + (imm << 1);

    if (load_from_memory) {
        SetRegister(rd, bus.Read16(address));
    } else {
        bus.Write16(address, static_cast<u16>(GetRegister(rd)));
    }
}

void ARM7::Thumb_SPRelativeLoadStore(const u16 opcode) {
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<10, 8>(opcode);
    const std::unsigned_integral auto imm = Common::GetBitRange<7, 0>(opcode);

    if (load_from_memory) {
        SetRegister(rd, bus.Read32(GetSP() + (imm << 2)));
    } else {
        bus.Write32(GetSP() + (imm << 2), GetRegister(rd));
    }
}

void ARM7::Thumb_LoadAddress(const u16 opcode) {
    const bool load_from_sp = Common::IsBitSet<11>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<10, 8>(opcode);
    const std::unsigned_integral auto imm = Common::GetBitRange<7, 0>(opcode);

    u32 address = 0;
    if (load_from_sp) {
        address = GetSP();
    } else {
        address = GetPC() & ~0b11;
    }
    const u32 offset = imm << 2;

    SetRegister(rd, address + offset);
}

void ARM7::Thumb_AddOffsetToStackPointer(const u16 opcode) {
    const bool offset_is_negative = Common::IsBitSet<7>(opcode);
    const std::unsigned_integral auto imm = Common::GetBitRange<6, 0>(opcode);

    if (offset_is_negative) {
        SetSP(GetSP() - (imm << 2));
    } else {
        SetSP(GetSP() + (imm << 2));
    }
}

void ARM7::Thumb_PushPopRegisters(const u16 opcode) {
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const bool store_lr_load_pc = Common::IsBitSet<8>(opcode);
    const std::unsigned_integral auto rlist = Common::GetBitRange<7, 0>(opcode);

    // Go through `rlist`'s 8 bits, and write down which bits are set. This tells
    // us which registers we need to load/store.
    // If bit 0 is set, that corresponds to R0. If bit 1 is set, that corresponds
    // to R1, and so on.
    const auto bit_is_set = [rlist](const u8 i) { return (rlist & (1 << i)) != 0; };
    const auto set_bits = ranges::views::iota(0, 8)
                        | ranges::views::filter(bit_is_set)
                        | ranges::to<std::vector>;

    if (set_bits.empty()) {
        if (!store_lr_load_pc) {
            return;
        }

        if (load_from_memory) {
            SetPC(bus.Read32(GetSP()) & ~0b1);
            SetSP(GetSP() + 4);
        } else {
            SetSP(GetSP() - 4);
            bus.Write32(GetSP(), GetLR());
        }

        return;
    }

    if (load_from_memory) {
        for (u8 reg : set_bits) {
            SetRegister(reg, bus.Read32(GetSP()));
            SetSP(GetSP() + 4);
        }

        if (store_lr_load_pc) {
            SetPC(bus.Read32(GetSP()) & ~0b1);
            SetSP(GetSP() + 4);
        }
    } else {
        if (store_lr_load_pc) {
            SetSP(GetSP() - 4);
            bus.Write32(GetSP(), GetLR());
        }

        for (u8 reg : set_bits | ranges::views::reverse) {
            SetSP(GetSP() - 4);
            bus.Write32(GetSP(), GetRegister(reg));
        }
    }
}

void ARM7::Thumb_MultipleLoadStore(const u16 opcode) {
    const bool load_from_memory = Common::IsBitSet<11>(opcode);
    const std::unsigned_integral auto rb = Common::GetBitRange<10, 8>(opcode);
    const std::unsigned_integral auto rlist = Common::GetBitRange<7, 0>(opcode);

    // Go through `rlist`'s 8 bits, and write down which bits are set. This tells
    // us which registers we need to load/store.
    // If bit 0 is set, that corresponds to R0. If bit 1 is set, that corresponds
    // to R1, and so on.
    const auto bit_is_set = [rlist](const u8 i) { return (rlist & (1 << i)) != 0; };
    const auto set_bits = ranges::views::iota(0, 8)
                        | ranges::views::filter(bit_is_set)
                        | ranges::to<std::vector>;

    if (set_bits.empty()) {
        return;
    }

    if (load_from_memory) {
        for (u8 i : set_bits) {
            SetRegister(i, bus.Read32(GetRegister(rb)));
            SetRegister(rb, GetRegister(rb) + 4);
        }
    } else {
        for (u8 reg : set_bits) {
            bus.Write32(GetRegister(rb), GetRegister(reg));
            SetRegister(rb, GetRegister(rb) + 4);
        }
    }
}
