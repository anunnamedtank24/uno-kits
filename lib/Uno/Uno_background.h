#ifndef UNO_BACKGROUND_H
#define UNO_BACKGROUND_H

#include "Uno.h"
#include <avr/interrupt.h>

namespace Uno {
    // runs functions in the background, about every 1 ms, so a program does not have to call update() itself:
    //   L298N::TwoWheelCar<...> car;
    //   Uno::UnoBackground::add([] { car.update(); });     // a lambda without captures is a function
    //   car.forward(180);                                   // drives on by itself; loop() may wait, delay, print ...
    //
    // Timer0's compare A interrupt (Timer0 also runs millis): it comes once every Timer0 period (1.024 ms)
    // whatever OCR0A is, so D6 PWM keeps working; nothing else of the Arduino core uses it
    //
    // the functions run inside the interrupt with interrupts on again (millis and the serial keep going);
    // a run that is not finished when the next one is due is not started twice (it is skipped)
    // keep them short (well under 1 ms): update() functions are (they never wait)
    // the interrupt is defined in this header: include it in one .cpp file only
    struct UnoBackground : UnoBoard {
        // how many functions can be added
        static constexpr uint8_t MAX_TASKS = 6;

        // adds a function (runs from the next ms on); false if full or already there
        static inline bool add(void (*task)());

        // removes it; false if not there
        static inline bool remove(void (*task)());

        // removes every function
        static inline void clear();

        // stops / starts again all of them (e.g. while changing several things at once)
        static inline void pause();
        static inline void resume();
        static inline bool isPaused();

        // functions added
        static inline uint8_t count();

        // runs that were skipped because the last one was still going (too slow functions)
        static inline uint16_t skipped();
    };

    namespace background_state {
        inline void (*volatile tasks[UnoBackground::MAX_TASKS])() = {};
        inline volatile uint8_t count = 0;
        inline volatile bool running = false;
        inline volatile bool paused = false;
        inline volatile uint16_t skipped = 0;

        inline void run() {
            if (running) {
                if (skipped < 65535) {
                    skipped++;
                }
                return;
            }
            running = true;
            sei();                          // millis, serial, pin interrupts go on meanwhile
            if (!paused) {
                for (uint8_t i = 0; i < count; i++) {
                    void (*task)() = tasks[i];
                    if (task != nullptr) {
                        task();
                    }
                }
            }
            cli();
            running = false;
        }
    }

    // ================= definitions =================

    inline bool UnoBackground::add(void (*task)()) {
        if (task == nullptr) {
            return false;
        }
        uint8_t saved = stopInterrupts();
        bool added = false;
        bool there = false;
        for (uint8_t i = 0; i < background_state::count; i++) {
            if (background_state::tasks[i] == task) {
                there = true;
            }
        }
        if (!there && background_state::count < MAX_TASKS) {
            background_state::tasks[background_state::count] = task;
            background_state::count++;
            added = true;
        }
        TIMSK0 |= (1 << OCIE0A);
        restoreInterrupts(saved);
        return added;
    }

    inline bool UnoBackground::remove(void (*task)()) {
        uint8_t saved = stopInterrupts();
        bool removed = false;
        for (uint8_t i = 0; i < background_state::count; i++) {
            if (background_state::tasks[i] == task) {
                for (uint8_t j = i; j + 1 < background_state::count; j++) {
                    background_state::tasks[j] = background_state::tasks[j + 1];
                }
                background_state::count--;
                background_state::tasks[background_state::count] = nullptr;
                removed = true;
                break;
            }
        }
        if (background_state::count == 0) {
            TIMSK0 &= ~(1 << OCIE0A);
        }
        restoreInterrupts(saved);
        return removed;
    }

    inline void UnoBackground::clear() {
        uint8_t saved = stopInterrupts();
        TIMSK0 &= ~(1 << OCIE0A);
        for (uint8_t i = 0; i < MAX_TASKS; i++) {
            background_state::tasks[i] = nullptr;
        }
        background_state::count = 0;
        restoreInterrupts(saved);
    }

    inline void UnoBackground::pause() {
        background_state::paused = true;
    }

    inline void UnoBackground::resume() {
        background_state::paused = false;
    }

    inline bool UnoBackground::isPaused() {
        return background_state::paused;
    }

    inline uint8_t UnoBackground::count() {
        return background_state::count;
    }

    inline uint16_t UnoBackground::skipped() {
        uint8_t saved = stopInterrupts();
        uint16_t skipped = background_state::skipped;
        restoreInterrupts(saved);
        return skipped;
    }
}

// Timer0 compare A, once a Timer0 period (~ 1 ms): the background functions
ISR(TIMER0_COMPA_vect) {
    Uno::background_state::run();
}

#endif
