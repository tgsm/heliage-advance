#pragma once

#include <array>
#include "keypad.h"
#include "common/types.h"

int main_null(char* argv[]);

void HandleFrontendEvents([[maybe_unused]] Keypad* keypad);
void DisplayFramebuffer([[maybe_unused]] std::array<u16, 240 * 160>& framebuffer);
