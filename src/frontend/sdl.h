#pragma once

#include <array>
#include "../keypad.h"
#include "../types.h"

int main_SDL(char* argv[]);

void HandleFrontendEvents(Keypad* keypad);
void DisplayFramebuffer(std::array<u16, 240 * 160>& framebuffer);
