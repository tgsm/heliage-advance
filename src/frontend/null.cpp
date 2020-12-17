#include <filesystem>
#include "../bios.h"
#include "../cartridge.h"
#include "../gba.h"

void HandleFrontendEvents([[maybe_unused]] Keypad* keypad) {

}

void DisplayFramebuffer([[maybe_unused]] std::array<u16, 240 * 160>& framebuffer) {

}

int main_null(char* argv[]) {
    const std::filesystem::path bios_path = argv[1];
    BIOS bios(bios_path);
    const std::filesystem::path cartridge_path = argv[2];
    Cartridge cartridge(cartridge_path);

    GBA gba(bios, cartridge);

    while (true) {
        gba.Run();
    }

    return 0;
}
