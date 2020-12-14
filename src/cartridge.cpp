#include <fstream>
#include "cartridge.h"
#include "logging.h"

constexpr int TITLE_LENGTH = 12;

Cartridge::Cartridge(const std::filesystem::path& cartridge_path) {
    LINFO("loading cartridge: {}", cartridge_path.string());
    LoadCartridge(cartridge_path);
}

void Cartridge::LoadCartridge(const std::filesystem::path& cartridge_path) {
    std::ifstream stream(cartridge_path.string().c_str(), std::ios::binary);
    if (!stream.is_open()) {
        LFATAL("could not open ROM: {}", cartridge_path.string());
        std::exit(1);
    }

    rom_size = std::filesystem::file_size(cartridge_path);
    rom.resize(rom_size);

    stream.read(reinterpret_cast<char*>(rom.data()), rom.size());

    LINFO("cartridge: loaded {} bytes ({} KB)", rom_size, rom_size / 1024);
}

std::string Cartridge::GetGameTitle() {
    std::string title;

    std::copy_n(rom.begin() + 0xA0, TITLE_LENGTH, std::back_inserter(title));
    return title;
}
