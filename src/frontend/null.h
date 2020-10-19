#pragma once

#include <array>
#include "../types.h"

int main_null(char* argv[]);

void HandleFrontendEvents();
void DisplayFramebuffer([[maybe_unused]] std::array<u16, 240 * 160>& framebuffer);
