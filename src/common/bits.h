#pragma once

#include <utility>
#include "common/types.h"

namespace Common {

template <u8 Bit, std::integral T>
static constexpr bool IsBitSet(const T value) {
    static_assert(Bit < TypeSizeInBits<T>);
    return (value & (T(1) << Bit)) != 0;
}

template <u8 UpperBound, u8 LowerBound, std::integral T>
static consteval T GetBitMaskFromRange() {
    if (UpperBound < LowerBound) {
        return GetBitMaskFromRange<LowerBound, UpperBound, T>();
    }

    static_assert(UpperBound != LowerBound, "Invalid one-bit-wide range. You should just use Common::IsBitSet instead.");

    T mask = 0;
    for (T bit = LowerBound; bit <= UpperBound; bit++) {
        mask |= (T(1) << bit);
    }

    return mask;
}

template <u8 UpperBound, u8 LowerBound, std::integral T>
static constexpr T GetBitRange(const T value) {
    if (UpperBound < LowerBound) {
        return GetBitRange<LowerBound, UpperBound>(value);
    }

    static_assert(UpperBound < TypeSizeInBits<T>, "Upper bound is higher than T's size");

    const T mask = GetBitMaskFromRange<UpperBound, LowerBound, T>() >> LowerBound;
    return (value >> LowerBound) & mask;
}

template <u8 UpperBound, u8 LowerBound, std::integral T>
static constexpr void DisableBitRange(T& value) {
    if (UpperBound < LowerBound) {
        DisableBitRange<LowerBound, UpperBound, T>(value);
        return;
    }

    static_assert(UpperBound < TypeSizeInBits<T>, "Upper bound is higher than T's size");
    static_assert(UpperBound != LowerBound);

    value &= ~Common::GetBitMaskFromRange<UpperBound, LowerBound, T>();
}

}
