#include "timer.h"

void Timer::SetControl(const u16 value) {
    // If the timer was off and we are now turning it on, reload the counter.
    if (!control.flags.running && Common::IsBitSet<7>(value)) {
        counter = reload;
    }

    control.raw = value;

    // Bits 3-5 and 8-15 are not used.
    Common::DisableBitRange<3, 5>(control.raw);
    Common::DisableBitRange<8, 15>(control.raw);
}

void Timers::AdvanceCycles(u16 cycles, const CycleType cycle_type) {
    ppu.AdvanceCycles(cycles);

    if (cycle_type == CycleType::None) {
        return;
    }

    for (; cycles > 0; cycles--) {
        if (timer0.control.flags.running) {
            timer0.counter++;
            if (timer0.counter == 0) {
                interrupts.RequestInterrupt(Interrupts::Bits::Timer0Overflow);
            }
        }

        if (timer1.control.flags.running) {
            timer1.counter++;
            if (timer1.counter == 0) {
                interrupts.RequestInterrupt(Interrupts::Bits::Timer1Overflow);
            }
        }

        if (timer2.control.flags.running) {
            timer2.counter++;
            if (timer2.counter == 0) {
                interrupts.RequestInterrupt(Interrupts::Bits::Timer2Overflow);
            }
        }

        if (timer3.control.flags.running) {
            timer3.counter++;
            if (timer3.counter == 0) {
                interrupts.RequestInterrupt(Interrupts::Bits::Timer3Overflow);
            }
        }
    }
}
