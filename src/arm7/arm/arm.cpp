#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/reverse.hpp>
#include "common/bits.h"
#include "arm7/arm7.h"

void ARM7::ARM_Multiply(const u32 opcode) {
    const bool accumulate = Common::IsBitSet<21>(opcode);
    const bool set_condition_codes = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<11, 8>(opcode);
    const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(opcode);

    u32 result = GetRegister(rm) * GetRegister(rs);
    if (accumulate) {
        result += GetRegister(rn);
    }

    SetRegister(rd, result);

    if (set_condition_codes) {
        cpsr.flags.negative = Common::IsBitSet<31>(result);
        cpsr.flags.zero = (result == 0);
    }
}

void ARM7::ARM_MultiplyLong(const u32 opcode) {
    const bool sign = Common::IsBitSet<22>(opcode);
    const bool accumulate = Common::IsBitSet<21>(opcode);
    const bool set_condition_codes = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rdhi = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rdlo = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto rs = Common::GetBitRange<11, 8>(opcode);
    const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(opcode);

    if (sign) {
        const s64 rm_se = (static_cast<s64>(GetRegister(rm)) << 32) >> 32;
        const s64 rs_se = (static_cast<s64>(GetRegister(rs)) << 32) >> 32;

        s64 result = rm_se * rs_se;
        if (accumulate) {
            result += ((static_cast<s64>(GetRegister(rdhi)) << 32) | GetRegister(rdlo));
        }
        SetRegister(rdlo, Common::GetBitRange<31, 0>(result));
        SetRegister(rdhi, Common::GetBitRange<63, 32>(result));

        if (set_condition_codes) {
            cpsr.flags.negative = Common::IsBitSet<63>(result);
            cpsr.flags.zero = (result == 0);
        }
    } else {
        u64 result = static_cast<u64>(GetRegister(rm)) * static_cast<u64>(GetRegister(rs));
        if (accumulate) {
            result += ((static_cast<u64>(GetRegister(rdhi)) << 32) | GetRegister(rdlo));
        }
        SetRegister(rdlo, Common::GetBitRange<31, 0>(result));
        SetRegister(rdhi, Common::GetBitRange<63, 32>(result));

        if (set_condition_codes) {
            cpsr.flags.negative = Common::IsBitSet<63>(result);
            cpsr.flags.zero = (result == 0);
        }
    }
}

void ARM7::ARM_SingleDataSwap(const u32 opcode) {
    const bool swap_byte = Common::IsBitSet<22>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(opcode);

    const u32 address = GetRegister(rn);

    if (swap_byte) {
        const u8 swap_value = bus.Read8(address);
        bus.Write8(address, GetRegister(rm));
        SetRegister(rd, swap_value);
    } else {
        const u32 swap_value = bus.Read32(address);
        bus.Write32(address, GetRegister(rm));
        SetRegister(rd, swap_value);
    }
}

void ARM7::ARM_HalfwordDataTransferRegister(const u32 opcode) {
    const bool load_from_memory = Common::IsBitSet<20>(opcode);
    const bool sign = Common::IsBitSet<6>(opcode);
    const bool halfword = Common::IsBitSet<5>(opcode);

    const u8 conditions = (load_from_memory << 1) | halfword;
    switch (conditions) {
        case 0b00:
            ARM_SingleDataSwap(opcode);
            break;
        case 0b01:
            ARM_HalfwordDataTransferRegister_Impl<false, true>(opcode, sign);
            break;
        case 0b10:
            ARM_HalfwordDataTransferRegister_Impl<true, false>(opcode, sign);
            break;
        case 0b11:
            ARM_HalfwordDataTransferRegister_Impl<true, true>(opcode, sign);
            break;
        default:
            UNREACHABLE();
    }
}

template <bool load_from_memory, bool transfer_halfword>
void ARM7::ARM_HalfwordDataTransferRegister_Impl(const u32 opcode, const bool sign) {
    const bool pre_indexing = Common::IsBitSet<24>(opcode);
    const bool add_offset_to_base = Common::IsBitSet<23>(opcode);
    const bool write_back = Common::IsBitSet<21>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(opcode);

    u32 address = GetRegister(rn);
    if (pre_indexing) {
        if (add_offset_to_base) {
            address += GetRegister(rm);
        } else {
            address -= GetRegister(rm);
        }

        if constexpr (load_from_memory) {
            if constexpr (transfer_halfword) {
                if (sign) {
                    SetRegister(rd, (static_cast<s16>(bus.Read16(address)) << 16) >> 16);
                } else {
                    SetRegister(rd, bus.Read16(address));
                }
            } else {
                if (sign) {
                    SetRegister(rd, (static_cast<s8>(bus.Read8(address)) << 24) >> 24);
                } else {
                    SetRegister(rd, bus.Read8(address));
                }
            }
        } else {
            if constexpr (transfer_halfword) {
                bus.Write16(address, GetRegister(rd));
            } else {
                bus.Write8(address, GetRegister(rd));
            }
        }

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        if constexpr (load_from_memory) {
            if constexpr (transfer_halfword) {
                if (sign) {
                    SetRegister(rd, static_cast<s16>(bus.Read16(address) << 16) >> 16);
                } else {
                    SetRegister(rd, bus.Read16(address));
                }
            } else {
                if (sign) {
                    SetRegister(rd, static_cast<s8>(bus.Read8(address) << 24) >> 24);
                } else {
                    SetRegister(rd, bus.Read8(address));
                }
            }
        } else {
            if constexpr (transfer_halfword) {
                bus.Write16(address, GetRegister(rd));
            } else {
                bus.Write8(address, GetRegister(rd));
            }
        }

        if (add_offset_to_base) {
            address += GetRegister(rm);
        } else {
            address -= GetRegister(rm);
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_HalfwordDataTransferImmediate(const u32 opcode) {
    const bool load_from_memory = Common::IsBitSet<20>(opcode);
    const bool sign = Common::IsBitSet<6>(opcode);
    const bool halfword = Common::IsBitSet<5>(opcode);

    const u8 conditions = (load_from_memory << 1) | halfword;
    switch (conditions) {
        case 0b00:
            ARM_SingleDataSwap(opcode);
            break;
        case 0b01:
            ARM_StoreHalfwordImmediate(opcode, sign);
            break;
        case 0b10:
            // Not to be confused with ARM_LoadByte
            ARM_LoadSignedByte(opcode);
            break;
        case 0b11:
            ARM_LoadHalfwordImmediate(opcode, sign);
            break;
        default:
            UNREACHABLE();
    }
}

void ARM7::ARM_LoadHalfwordImmediate(const u32 opcode, const bool sign) {
    const bool pre_indexing = Common::IsBitSet<24>(opcode);
    const bool add_offset_to_base = Common::IsBitSet<23>(opcode);
    const bool write_back = Common::IsBitSet<21>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto offset_high = Common::GetBitRange<11, 8>(opcode);
    const std::unsigned_integral auto offset_low = Common::GetBitRange<3, 0>(opcode);
    const std::unsigned_integral auto offset = (offset_high << 4) | offset_low;

    u32 address = GetRegister(rn);
    bool address_is_halfword_aligned = ((address & 0b1) == 0b0);

    if (pre_indexing) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        address_is_halfword_aligned = ((address & 0b1) == 0b0);

        if (sign) {
            s16 value = bus.Read16(address & ~0b1);

            if (!address_is_halfword_aligned) {
                value >>= 8;
            }

            SetRegister(rd, value);
        } else {
            const u32 value = std::rotr(u32(bus.Read16(address)), (address & 0b1) * 8);
            SetRegister(rd, value);
        }

        if (write_back && rd != rn) {
            SetRegister(rn, address);
        }
    } else {
        if (sign) {
            s16 value = bus.Read16(address & ~0b1);

            if (!address_is_halfword_aligned) {
                value >>= 8;
            }

            SetRegister(rd, value);
        } else {
            const u32 value = std::rotr(u32(bus.Read16(address)), (address & 0b1) * 8);
            SetRegister(rd, value);
        }

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        if (rd != rn) {
            SetRegister(rn, address);
        }
    }
}

void ARM7::ARM_StoreHalfwordImmediate(const u32 opcode, const bool sign) {
    const bool pre_indexing = Common::IsBitSet<24>(opcode);
    const bool add_offset_to_base = Common::IsBitSet<23>(opcode);
    const bool write_back = Common::IsBitSet<21>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto offset_high = Common::GetBitRange<11, 8>(opcode);
    const std::unsigned_integral auto offset_low = Common::GetBitRange<3, 0>(opcode);
    const u8 offset = (offset_high << 4) | offset_low;

    u32 address = GetRegister(rn);
    const u16 value = GetRegister(rd);
    if (pre_indexing) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        bus.Write16(address, sign ? static_cast<s16>(value) : value);

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        bus.Write16(address, sign ? static_cast<s16>(value) : value);
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_LoadSignedByte(const u32 opcode) {
    const bool pre_indexing = Common::IsBitSet<24>(opcode);
    const bool add_offset_to_base = Common::IsBitSet<23>(opcode);
    const bool write_back = Common::IsBitSet<21>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    const std::unsigned_integral auto offset_high = Common::GetBitRange<11, 8>(opcode);
    const std::unsigned_integral auto offset_low = Common::GetBitRange<3, 0>(opcode);
    const s8 offset = (offset_high << 4) | offset_low;

    u32 address = GetRegister(rn);
    if (pre_indexing) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rd, static_cast<s8>(bus.Read8(address)));

        if (write_back) {
            SetRegister(rn, address);
        }
    } else {
        SetRegister(rd, static_cast<s8>(bus.Read8(address)));

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        SetRegister(rn, address);
    }
}

void ARM7::ARM_SingleDataTransfer(const u32 opcode) {
    const u16 operation = (opcode >> 20) & 0b101;
    switch (operation) {
        case 0b000:
            // Store word
            ARM_SingleDataTransfer_Impl<false, false>(opcode);
            break;
        case 0b001:
            // Load word
            ARM_SingleDataTransfer_Impl<true, false>(opcode);
            break;
        case 0b100:
            // Store byte
            ARM_SingleDataTransfer_Impl<false, true>(opcode);
            break;
        case 0b101:
            // Load byte
            ARM_SingleDataTransfer_Impl<true, true>(opcode);
            break;
        default:
            UNREACHABLE();
    }
}

template <bool load_from_memory, bool transfer_byte>
void ARM7::ARM_SingleDataTransfer_Impl(const u32 opcode) {
    const bool offset_is_register = Common::IsBitSet<25>(opcode);
    const bool add_before_transfer = Common::IsBitSet<24>(opcode);
    const bool add_offset_to_base = Common::IsBitSet<23>(opcode);
    const bool write_back = Common::IsBitSet<21>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rd = Common::GetBitRange<15, 12>(opcode);
    s32 offset = 0;

    if (offset_is_register) {
        const std::unsigned_integral auto shift = Common::GetBitRange<11, 4>(opcode);
        const std::unsigned_integral auto rm = Common::GetBitRange<3, 0>(opcode);

        if (!Common::IsBitSet<0>(shift)) {
            std::unsigned_integral auto shift_amount = Common::GetBitRange<7, 3>(shift);
            auto shift_type = ShiftType(Common::GetBitRange<2, 1>(shift));

            if (shift_amount == 0 && shift_type != ShiftType::LSL) {
                if (shift_type == ShiftType::ROR) {
                    shift_type = ShiftType::RRX;
                } else {
                    shift_amount = 32;
                }
            }

            if (shift_type == ShiftType::RRX) {
                offset = Shift_RRX(GetRegister(rm), false);
            } else {
                offset = Shift(GetRegister(rm), shift_type, shift_amount, false);
            }
        } else if ((shift & 0b1001) == 0b0001) {
            UNIMPLEMENTED();
        } else {
            ASSERT(false);
        }
    } else {
        offset = Common::GetBitRange<11, 0>(opcode);
    }

    u32 address = GetRegister(rn);
    if (add_before_transfer) {
        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        if constexpr (load_from_memory) {
            if constexpr (transfer_byte) {
                SetRegister(rd, bus.Read8(address));
            } else {
                const u32 value = std::rotr(bus.Read32(address & ~0b11), (address & 0b11) * 8);
                SetRegister(rd, value);
            }
        } else {
            if constexpr (transfer_byte) {
                bus.Write8(address, GetRegister(rd));
            } else {
                bus.Write32(address & ~0b11, GetRegister(rd));
            }
        }

        if (write_back && rd != rn) {
            SetRegister(rn, address);
        }
    } else {
        if constexpr (load_from_memory) {
            if constexpr (transfer_byte) {
                SetRegister(rd, bus.Read8(address));
            } else {
                const u32 value = std::rotr(bus.Read32(address & ~0b11), (address & 0b11) * 8);
                SetRegister(rd, value);
            }
        } else {
            if constexpr (transfer_byte) {
                bus.Write8(address, GetRegister(rd));
            } else {
                bus.Write32(address & ~0b11, GetRegister(rd));
            }
        }

        if (add_offset_to_base) {
            address += offset;
        } else {
            address -= offset;
        }

        if (rd != rn) {
            SetRegister(rn, address);
        }
    }
}

void ARM7::ARM_BlockDataTransfer(const u32 opcode) {
    const bool pre_indexing = Common::IsBitSet<24>(opcode);
    const bool add_offset_to_base = Common::IsBitSet<23>(opcode);
    const bool load_psr = Common::IsBitSet<22>(opcode);
    const bool write_back = Common::IsBitSet<21>(opcode);
    const bool load_from_memory = Common::IsBitSet<20>(opcode);
    const std::unsigned_integral auto rn = Common::GetBitRange<19, 16>(opcode);
    const std::unsigned_integral auto rlist = Common::GetBitRange<15, 0>(opcode);

    if (load_psr) {
        LERROR("unimplemented load PSR in block data transfer");
    }

    // Go through `rlist`'s 16 bits, and write down which bits are set. This tells
    // us which registers we need to load/store.
    // If bit 0 is set, that corresponds to R0. If bit 1 is set, that corresponds
    // to R1, and so on.
    const auto bit_is_set = [rlist](const u8 i) { return (rlist & (1 << i)) != 0; };
    const auto set_bits = ranges::views::iota(0, 16)
                        | ranges::views::filter(bit_is_set)
                        | ranges::to<std::vector>;

    if (set_bits.empty()) {
        return;
    }

    const bool rn_is_in_rlist = std::find(set_bits.begin(), set_bits.end(), rn) != set_bits.end();

    u32 address = GetRegister(rn);
    if (load_from_memory) {
        if (add_offset_to_base) {
            for (u8 reg : set_bits) {
                if (pre_indexing) {
                    address += 4;
                    SetRegister(reg, bus.Read32(address));
                } else {
                    SetRegister(reg, bus.Read32(address));
                    address += 4;
                }
            }
        } else {
            for (u8 reg : set_bits | ranges::views::reverse) {
                if (pre_indexing) {
                    address -= 4;
                    SetRegister(reg, bus.Read32(address));
                } else {
                    SetRegister(reg, bus.Read32(address));
                    address -= 4;
                }
            }
        }
    } else {
        if (add_offset_to_base) {
            for (u8 reg : set_bits) {
                if (pre_indexing) {
                    address += 4;
                    bus.Write32(address, GetRegister(reg));
                } else {
                    bus.Write32(address, GetRegister(reg));
                    address += 4;
                }
            }
        } else {
            for (u8 reg : set_bits | ranges::views::reverse) {
                if (pre_indexing) {
                    address -= 4;
                    bus.Write32(address, GetRegister(reg));
                } else {
                    bus.Write32(address, GetRegister(reg));
                    address -= 4;
                }
            }
        }
    }

    if (write_back) {
        if (load_from_memory && rn_is_in_rlist) {
            return;
        }

        SetRegister(rn, address);
    }
}

// TODO: ARM coprocessor data transfer

// TODO: ARM coprocessor data operation

// TODO: ARM coprocessor register transfer
