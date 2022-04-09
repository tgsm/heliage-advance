#pragma once

#include "common/defines.h"
#include "common/logging.h"
#include "common/types.h"

class Interrupts {
public:
    Interrupts() = default;

    enum class Bits : u16 {
        VBlank = (1 << 0),
        HBlank = (1 << 1),
        VCounterMatch = (1 << 2),
        Timer0Overflow = (1 << 3),
        Timer1Overflow = (1 << 4),
        Timer2Overflow = (1 << 5),
        Timer3Overflow = (1 << 6),
        Serial = (1 << 7),
        DMA0 = (1 << 8),
        DMA1 = (1 << 9),
        DMA2 = (1 << 10),
        DMA3 = (1 << 11),
        Keypad = (1 << 12),
        GamePak = (1 << 13),
    };

    [[nodiscard]] ALWAYS_INLINE u16 GetIF() const { return interrupts_requested; }

    ALWAYS_INLINE void SetIF(const u16 value) {
        interrupts_requested ^= value;
    }

    [[nodiscard]] ALWAYS_INLINE u16 GetIE() const { return interrupts_enabled; }

    ALWAYS_INLINE void SetIE(const u16 value) {
        interrupts_enabled = value;
    }

    [[nodiscard]] ALWAYS_INLINE bool GetIME() const { return interrupt_master_enable; }

    ALWAYS_INLINE void SetIME(const bool value) {
        interrupt_master_enable = value;
    }

    ALWAYS_INLINE void RequestInterrupt(const Bits bit) {
        interrupts_requested |= static_cast<u16>(bit);
    }

private:
    u16 interrupts_requested = 0;
    u16 interrupts_enabled = 0;
    bool interrupt_master_enable = false;
};
