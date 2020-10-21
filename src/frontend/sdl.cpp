#include <SDL2/SDL.h>
#include <cstdio>
#include <filesystem>
#include "../cartridge.h"
#include "../gba.h"
#include "../keypad.h"
#include "../logging.h"

constexpr int GBA_SCREEN_WIDTH = 240;
constexpr int GBA_SCREEN_HEIGHT = 160;

namespace {

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* framebuffer_output;
SDL_Event event;

bool running = false;

}

void HandleFrontendEvents(Keypad* keypad) {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
#define KEYDOWN(k, button) if (event.key.keysym.sym == k) keypad->PressButton(Keypad::Buttons::button)
                KEYDOWN(SDLK_UP, Up);
                KEYDOWN(SDLK_DOWN, Down);
                KEYDOWN(SDLK_LEFT, Left);
                KEYDOWN(SDLK_RIGHT, Right);
                KEYDOWN(SDLK_a, A);
                KEYDOWN(SDLK_s, B);
                KEYDOWN(SDLK_BACKSPACE, Select);
                KEYDOWN(SDLK_RETURN, Start);
#undef KEYDOWN
                break;
            case SDL_KEYUP:
#define KEYUP(k, button) if (event.key.keysym.sym == k) keypad->ReleaseButton(Keypad::Buttons::button)
                KEYUP(SDLK_UP, Up);
                KEYUP(SDLK_DOWN, Down);
                KEYUP(SDLK_LEFT, Left);
                KEYUP(SDLK_RIGHT, Right);
                KEYUP(SDLK_a, A);
                KEYUP(SDLK_s, B);
                KEYUP(SDLK_BACKSPACE, Select);
                KEYUP(SDLK_RETURN, Start);
#undef KEYUP
                break;
            case SDL_QUIT:
                running = false;
                break;
        }
    }
}

void DisplayFramebuffer(std::array<u16, 240 * 160>& framebuffer) {
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(framebuffer_output, nullptr, framebuffer.data(), GBA_SCREEN_WIDTH * 2);
    SDL_RenderCopy(renderer, framebuffer_output, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Shutdown() {
    LINFO("shutting down SDL");
    SDL_DestroyTexture(framebuffer_output);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main_SDL(char* argv[]) {
    const std::filesystem::path cartridge_path = argv[1];
    Cartridge cartridge(cartridge_path);

    GBA gba(cartridge);

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("heliage-advance", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GBA_SCREEN_WIDTH * 2, GBA_SCREEN_HEIGHT * 2, 0);
    if (!window) {
        printf("failed to create SDL window: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("failed to create SDL renderer: %s\n", SDL_GetError());
        return 1;
    }

    framebuffer_output = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT);
    if (!framebuffer_output) {
        LFATAL("failed to create framebuffer output texture: %s", SDL_GetError());
        return 1;
    }

    std::string window_title = "heliage-advance";
    std::string game_title = cartridge.GetGameTitle();
    if (!game_title.empty()) {
        window_title += " - " + game_title;
    }
    SDL_SetWindowTitle(window, window_title.c_str());

    running = true;
    while (running) {
        gba.Run();
    }

    Shutdown();
    return 0;
}
