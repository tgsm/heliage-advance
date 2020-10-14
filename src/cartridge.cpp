#include <fstream>
#include "cartridge.h"
#include "logging.h"

Cartridge::Cartridge(const std::filesystem::path& cartridge_path) {
    LINFO("loading cartridge: %s", cartridge_path.string().c_str());
    LoadCartridge(cartridge_path);
}

void Cartridge::LoadCartridge(const std::filesystem::path& cartridge_path) {
    std::ifstream stream(cartridge_path.string().c_str(), std::ios::binary);
    if (!stream.is_open()) {
        LFATAL("could not open ROM: %s", cartridge_path.string().c_str());
        std::exit(1);
    }

    rom_size = std::filesystem::file_size(cartridge_path);
    rom.resize(rom_size);

    stream.read(reinterpret_cast<char*>(rom.data()), rom.size());

    LINFO("cartridge: loaded %u bytes (%u KB)", rom_size, rom_size / 1024);
}

u8 Cartridge::Read8(u32 addr) const {
    return rom.at(addr);
}

u16 Cartridge::Read16(u32 addr) const {
    const u16 value = ((rom.at(addr + 1) << 8) |
                       (rom.at(addr)));
    return value;
}

u32 Cartridge::Read32(u32 addr) const {
    const u32 value = ((rom.at(addr + 3) << 24) |
                       (rom.at(addr + 2) << 16) |
                       (rom.at(addr + 1) << 8) |
                       (rom.at(addr)));
    return value;
}
