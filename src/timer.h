#pragma once

#include "common/defines.h"
#include "common/types.h"
#include "interrupts.h"
#include "ppu.h"

class Timer {
public:
    explicit Timer(Interrupts& interrupts_) : interrupts(interrupts_) {}

    [[nodiscard]] ALWAYS_INLINE u16 GetCounter() const { return counter; }
    ALWAYS_INLINE void SetReload(const u16 value) { reload = value; }

    [[nodiscard]] ALWAYS_INLINE u16 GetControl() const { return control.raw; }
    void SetControl(u16 value);
    
private:
    friend class Timers;

    union {
        u16 raw;
        struct {
            u16 prescaler : 2;
            bool countup_timing : 1;
            u16 : 3;
            bool irq_enable : 1;
            bool running : 1;
            u16 : 8;
        } flags;
    } control {};
    u16 counter {};
    u16 reload {};

    Interrupts& interrupts;
};

class Timers {
public:
    Timers(Interrupts& interrupts_, PPU& ppu_)
        : interrupts(interrupts_),
          ppu(ppu_),
          timer0(interrupts),
          timer1(interrupts),
          timer2(interrupts),
          timer3(interrupts) {}

    enum class CycleType {
        None,
        Nonsequential,
        Sequential,
        Internal,
    };

    void AdvanceCycles(u16 cycles, CycleType cycle_type);

    [[nodiscard]] ALWAYS_INLINE u16 GetWaitstateControl() const { return waitstate_control; }
    ALWAYS_INLINE void SetWaitstateControl(const u16 value) { waitstate_control = value; }

private:
    Interrupts& interrupts;
    PPU& ppu;

    // TODO
    u16 waitstate_control = 0;

public:
    Timer timer0;
    Timer timer1;
    Timer timer2;
    Timer timer3;
};
