#pragma once

#include <concepts>
#include <cstdint>

typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;

typedef std::int8_t s8;
typedef std::int16_t s16;
typedef std::int32_t s32;
typedef std::int64_t s64;

template <typename T>
concept UnsignedIntegerMax32 = std::same_as<T, u8> || std::same_as<T, u16> || std::same_as<T, u32>;

namespace Common {

template <typename T>
constexpr std::size_t TypeSizeInBits = sizeof(T) * 8;

template <typename T1, typename T2>
constexpr bool TypeIsSame = std::is_same_v<T1, T2>;

}
