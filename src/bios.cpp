#include <fstream>
#include "bios.h"
#include "logging.h"

BIOS::BIOS(const std::filesystem::path& bios_filename) {
    ASSERT_MSG(std::filesystem::is_regular_file(bios_filename), "Provided BIOS is not a regular file");

    std::size_t file_size = std::filesystem::file_size(bios_filename);
    ASSERT_MSG(file_size == BIOS_SIZE, "Provided BIOS is not 16 KB");

    std::ifstream stream(bios_filename, std::ios::binary);
    ASSERT_MSG(stream.is_open(), "Could not open provided BIOS");

    stream.read(reinterpret_cast<char*>(bios.data()), file_size);
}
