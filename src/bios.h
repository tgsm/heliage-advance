#pragma once

#include <array>
#include <filesystem>
#include <type_traits>
#include "types.h"

constexpr int BIOS_SIZE = 16 * 1024; // 16 KB

class BIOS {
public:
    explicit BIOS(const std::filesystem::path& bios_filename);

    template <UnsignedIntegerMax32 T>
    [[nodiscard]] T Read(u32 addr) const {
        if constexpr (std::is_same_v<T, u8>) {
            return bios.at(addr);
        }

        if constexpr (std::is_same_v<T, u16>) {
            addr &= ~0b1;
            return (bios.at(addr) |
                   (bios.at(addr + 1) << 8));
        }

        if constexpr (std::is_same_v<T, u32>) {
            addr &= ~0b11;
            return (bios.at(addr) |
                   (bios.at(addr + 1) << 8) |
                   (bios.at(addr + 2) << 16) |
                   (bios.at(addr + 3) << 24));
        }
    }
private:
    std::array<u8, BIOS_SIZE> bios;
};
