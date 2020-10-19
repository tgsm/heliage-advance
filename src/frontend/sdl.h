#pragma once

#include <array>
#include "../types.h"

int main_SDL(char* argv[]);

void HandleFrontendEvents();
void DisplayFramebuffer(std::array<u16, 240 * 160>& framebuffer);
