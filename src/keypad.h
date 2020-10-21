#pragma once

#include "types.h"

class Keypad {
public:
    Keypad() = default;

    enum class Buttons : u16 {
        A = (1 << 0),
        B = (1 << 1),
        Select = (1 << 2),
        Start = (1 << 3),
        Right = (1 << 4),
        Left = (1 << 5),
        Up = (1 << 6),
        Down = (1 << 7),
        R = (1 << 8),
        L = (1 << 9),
    };

    u16 GetState() const { return state; }

    void PressButton(Buttons button) {
        state &= ~static_cast<u16>(button);
    }

    void ReleaseButton(Buttons button) {
        state |= static_cast<u16>(button);
    }
private:
    u16 state = 0xFFFF;
};
