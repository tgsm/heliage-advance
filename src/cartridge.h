#pragma once

#include <filesystem>
#include <vector>
#include "common/types.h"

class Cartridge {
public:
    explicit Cartridge(const std::filesystem::path& cartridge_path);

    [[nodiscard]] std::string GetGameTitle();

    [[nodiscard]] std::size_t GetSize() const { return rom_size; }

    template <UnsignedIntegerMax32 T>
    [[nodiscard]] T Read(u32 addr) const {
        if constexpr (std::is_same_v<T, u8>) {
            return rom.at(addr);
        }

        if constexpr (std::is_same_v<T, u16>) {
            addr &= ~0b1;
            return (rom.at(addr) |
                   (rom.at(addr + 1) << 8));
        }

        if constexpr (std::is_same_v<T, u32>) {
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
