#pragma once

#include <filesystem>
#include <vector>
#include "types.h"

class Cartridge {
public:
    Cartridge(const std::filesystem::path& cartridge_path);

    std::string GetGameTitle();

    u8 Read8(u32 addr) const;
    u16 Read16(u32 addr) const;
    u32 Read32(u32 addr) const;
private:
    void LoadCartridge(const std::filesystem::path& cartridge_path);
    std::vector<u8> rom;
    u32 rom_size = 0;
};
