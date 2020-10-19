#include <cstdio>
#include "frontend/frontend.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("usage: %s <cartridge>\n", argv[0]);
        return 1;
    }

#ifdef HA_FRONTEND_SDL
    return main_SDL(argv);
#else
    return main_null(argv);
#endif
}
