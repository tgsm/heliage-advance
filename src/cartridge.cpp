#include <algorithm>
#include <fstream>
#include "cartridge.h"
#include "common/logging.h"

constexpr int TITLE_OFFSET = 0xA0;
constexpr int TITLE_LENGTH = 12;

Cartridge::Cartridge(const std::filesystem::path& cartridge_path) {
    LINFO("loading cartridge: {}", cartridge_path.string());
    LoadCartridge(cartridge_path);
}

void Cartridge::LoadCartridge(const std::filesystem::path& cartridge_path) {
    std::ifstream stream(cartridge_path.string().c_str(), std::ios::binary);
    ASSERT_MSG(stream.is_open(), "could not open ROM: {}", cartridge_path.string());

    rom_size = std::filesystem::file_size(cartridge_path);
    rom.resize(rom_size);

    stream.read(reinterpret_cast<char*>(rom.data()), rom.size());

    LINFO("cartridge: loaded {} bytes ({} KB)", rom_size, rom_size / 1024);
}

std::string Cartridge::GetGameTitle() const {
    std::string title;

    const auto title_begin = rom.begin() + TITLE_OFFSET;
    const auto title_end = std::find(title_begin, title_begin + TITLE_LENGTH, '\0');
    std::copy(title_begin, title_end, std::back_inserter(title));

    return title;
}
