#pragma once

#include <filesystem>
#include <vector>
#include "types.h"

class Cartridge {
public:
    Cartridge(const std::filesystem::path& cartridge_path);

    std::string GetGameTitle();

    template <typename T>
    T Read(u32 addr) const {
        static_assert(std::is_same<T, u8>::value ||
                      std::is_same<T, u16>::value ||
                      std::is_same<T, u32>::value, "T must be a u8, u16, or u32");

        if constexpr (std::is_same<T, u8>::value) {
            return rom.at(addr);
        }

        if constexpr (std::is_same<T, u16>::value) {
            addr &= ~0b1;
            return (rom.at(addr) |
                   (rom.at(addr + 1) << 8));
        }

        if constexpr (std::is_same<T, u32>::value) {
            addr &= ~0b11;
            return (rom.at(addr) |
                   (rom.at(addr + 1) << 8) |
                   (rom.at(addr + 2) << 16) |
                   (rom.at(addr + 3) << 24));
        }
    }
private:
    void LoadCartridge(const std::filesystem::path& cartridge_path);
    std::vector<u8> rom;
    u32 rom_size = 0;
};
