#include <cstdio>
#include <filesystem>
#include "cartridge.h"
#include "gba.h"
#include "logging.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("usage: %s <cartridge>\n", argv[0]);
        return 1;
    }

    const std::filesystem::path cartridge_path = argv[1];
    Cartridge cartridge(cartridge_path);

    GBA gba(cartridge);
    gba.Run();

    return 0;
}
