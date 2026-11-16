#ifndef UNO_H
#define UNO_H

#include <Arduino.h>
#include <util/delay.h>

// Arduino UNO (ATmega328P), register level: the helpers every interface below is built on
// interfaces (include the ones you use, each includes this file):
//   Uno_ST7789.h  Uno::UnoST7789<DC, RES, ...>  Bus for ST7789 screens
//   Uno_serial.h  Uno::UnoSerial<BAUD>          Link over USB serial (D0 / D1), e.g. for video::StreamPlayer
//   Uno_one_wire.h  Uno::UnoOneWire<PIN>        Wire for 1-Wire devices, e.g. DS18B20::Thermometer
//   Uno_I2C.h     Uno::UnoI2C<SCL, SDA>         Bus for I2C devices on any pins, e.g. SSD1306 OLEDs
//   Uno_pin.h     Uno::UnoPin<PIN>              Pin to read, e.g. switches::Switch
//                 Uno::UnoOutputPin<PIN>        Pin to drive, e.g. buzzer::ActiveBuzzer
//                 Uno::UnoPwmPin<PIN>           Pwm: brightness on a PWM pin, e.g. LED::RGBLED
//   Uno_interrupt_pin.h  Uno::UnoInterruptPin<PIN>  InterruptPin: a function on every change, e.g. NEC::Receiver
//   Uno_tone.h    Uno::UnoTone<PIN>             square wave on any pin (Timer1), e.g. buzzer::PassiveBuzzer
//   Uno_pcm_out.h Uno::UnoPcmOut<PIN>           SampleOut: sound samples on D9 / D10 (Timer1 + Timer2), audio::SamplePlayer
// this file:     Uno::                         pins, SPI, UART, PWM, flash, time, pin checks
//
// pins are Arduino numbers: D0 ~ D13 = 0 ~ 13, A0 ~ A5 = 14 ~ 19

namespace Uno {
    // "no pin" for optional signals
    constexpr uint8_t NONE = 255;

    // ---------- pin functions ----------

    namespace function {
        constexpr uint32_t DIGITAL        = 1UL << 0;  // every pin
        constexpr uint32_t PWM_TIMER0     = 1UL << 1;  // D5 OC0B, D6 OC0A (Timer0 also runs millis / delay)
        constexpr uint32_t PWM_TIMER1     = 1UL << 2;  // D9 OC1A, D10 OC1B
        constexpr uint32_t PWM_TIMER2     = 1UL << 3;  // D3 OC2B, D11 OC2A
        constexpr uint32_t ANALOG_IN      = 1UL << 4;  // A0 ~ A5 (ADC)
        constexpr uint32_t UART_RX        = 1UL << 5;  // D0 (USB serial / upload)
        constexpr uint32_t UART_TX        = 1UL << 6;  // D1 (USB serial / upload)
        constexpr uint32_t UART_XCK       = 1UL << 7;  // D4 (USART clock)
        constexpr uint32_t SPI_SS         = 1UL << 8;  // D10 (must stay output in master mode)
        constexpr uint32_t SPI_MOSI       = 1UL << 9;  // D11
        constexpr uint32_t SPI_MISO       = 1UL << 10; // D12
        constexpr uint32_t SPI_SCK        = 1UL << 11; // D13
        constexpr uint32_t I2C_SDA        = 1UL << 12; // A4
        constexpr uint32_t I2C_SCL        = 1UL << 13; // A5
        constexpr uint32_t EXT_INT        = 1UL << 14; // D2 INT0, D3 INT1
        constexpr uint32_t PIN_CHANGE_INT = 1UL << 15; // every pin
        constexpr uint32_t COMPARATOR     = 1UL << 16; // D6 AIN0, D7 AIN1
        constexpr uint32_t INPUT_CAPTURE  = 1UL << 17; // D8 ICP1
        constexpr uint32_t CLOCK_OUT      = 1UL << 18; // D8 CLKO (fuse)
        constexpr uint32_t COUNTER_INPUT  = 1UL << 19; // D4 T0, D5 T1
        constexpr uint32_t LED            = 1UL << 20; // D13 on-board LED

        constexpr uint32_t PWM  = PWM_TIMER0 | PWM_TIMER1 | PWM_TIMER2;
        constexpr uint32_t UART = UART_RX | UART_TX;
        constexpr uint32_t SPI  = SPI_MOSI | SPI_MISO | SPI_SCK; // SS is usable as output
    }

    constexpr uint32_t D_ = function::DIGITAL | function::PIN_CHANGE_INT;

    constexpr uint32_t PIN_FUNCTIONS[20] = {
        D_ | function::UART_RX,                                             // D0  PD0
        D_ | function::UART_TX,                                             // D1  PD1
        D_ | function::EXT_INT,                                             // D2  PD2
        D_ | function::EXT_INT | function::PWM_TIMER2,                      // D3  PD3
        D_ | function::UART_XCK | function::COUNTER_INPUT,                  // D4  PD4
        D_ | function::PWM_TIMER0 | function::COUNTER_INPUT,                // D5  PD5
        D_ | function::PWM_TIMER0 | function::COMPARATOR,                   // D6  PD6
        D_ | function::COMPARATOR,                                          // D7  PD7
        D_ | function::INPUT_CAPTURE | function::CLOCK_OUT,                 // D8  PB0
        D_ | function::PWM_TIMER1,                                          // D9  PB1
        D_ | function::PWM_TIMER1 | function::SPI_SS,                       // D10 PB2
        D_ | function::PWM_TIMER2 | function::SPI_MOSI,                     // D11 PB3
        D_ | function::SPI_MISO,                                            // D12 PB4
        D_ | function::SPI_SCK | function::LED,                             // D13 PB5
        D_ | function::ANALOG_IN,                                           // A0  PC0
        D_ | function::ANALOG_IN,                                           // A1  PC1
        D_ | function::ANALOG_IN,                                           // A2  PC2
        D_ | function::ANALOG_IN,                                           // A3  PC3
        D_ | function::ANALOG_IN | function::I2C_SDA,                       // A4  PC4
        D_ | function::ANALOG_IN | function::I2C_SCL,                       // A5  PC5
    };

    // ---------- pin checks (compile time) ----------

    inline constexpr bool exists(uint8_t pin) {
        return pin < 20;
    }

    inline constexpr uint32_t functions(uint8_t pin) {
        return exists(pin) ? PIN_FUNCTIONS[pin] : 0;
    }

    inline constexpr bool has(uint8_t pin, uint32_t function) {
        return (functions(pin) & function) != 0;
    }

    // NONE or an existing pin
    inline constexpr bool optional(uint8_t pin) {
        return pin == NONE || exists(pin);
    }

    // two signals on one real pin
    inline constexpr bool same(uint8_t a, uint8_t b) {
        return a != NONE && a == b;
    }

    // bit of the pin in its port register
    inline constexpr uint8_t mask(uint8_t pin) {
        return !exists(pin) ? 0
             : pin < 8      ? (uint8_t)(1 << pin)
             : pin < 14     ? (uint8_t)(1 << (pin - 8))
             :                (uint8_t)(1 << (pin - 14));
    }

    // compile-time warnings: static_assert(Uno::warn::Xxx<condition>::ok, "")
    // warns (does not stop the build) when condition is true
    namespace warn {
        template <bool> struct DuplicatePin { static constexpr bool ok = true; };
        template <> struct DuplicatePin<true> {
            __attribute__((deprecated("two signals share one pin")))
            static constexpr bool ok = true;
        };

        template <bool> struct SerialPin { static constexpr bool ok = true; };
        template <> struct SerialPin<true> {
            __attribute__((deprecated("D0 / D1 are USB serial: upload and Serial will conflict")))
            static constexpr bool ok = true;
        };

        template <bool> struct NotPwmPin { static constexpr bool ok = true; };
        template <> struct NotPwmPin<true> {
            __attribute__((deprecated("not a PWM pin (3, 5, 6, 9, 10, 11): on / off only")))
            static constexpr bool ok = true;
        };

        template <bool> struct Timer0Pin { static constexpr bool ok = true; };
        template <> struct Timer0Pin<true> {
            __attribute__((deprecated("D5 / D6 PWM uses Timer0 (millis / delay): frequency fixed at ~976 Hz")))
            static constexpr bool ok = true;
        };

        template <bool> struct BaudError { static constexpr bool ok = true; };
        template <> struct BaudError<true> {
            __attribute__((deprecated("baud rate is more than 2% off at 16 MHz, the link may lose bytes")))
            static constexpr bool ok = true;
        };
    }

    // ---------- digital ----------

    template <uint8_t P>
    inline void pinOutput() {
        static_assert(optional(P), "Uno: pin must be 0 ~ 19 or NONE");
        if (P == NONE) {
            return;
        }
        if (P < 8) {
            DDRD |= mask(P);
        } else if (P < 14) {
            DDRB |= mask(P);
        } else {
            DDRC |= mask(P);
        }
    }

    template <uint8_t P>
    inline void pinHigh() {
        static_assert(optional(P), "Uno: pin must be 0 ~ 19 or NONE");
        if (P == NONE) {
            return;
        }
        if (P < 8) {
            PORTD |= mask(P);
        } else if (P < 14) {
            PORTB |= mask(P);
        } else {
            PORTC |= mask(P);
        }
    }

    template <uint8_t P>
    inline void pinLow() {
        static_assert(optional(P), "Uno: pin must be 0 ~ 19 or NONE");
        if (P == NONE) {
            return;
        }
        if (P < 8) {
            PORTD &= ~mask(P);
        } else if (P < 14) {
            PORTB &= ~mask(P);
        } else {
            PORTC &= ~mask(P);
        }
    }

    // input; pullUp: the internal ~35 kΩ pull-up resistor
    template <uint8_t P>
    inline void pinInput(bool pullUp) {
        static_assert(optional(P), "Uno: pin must be 0 ~ 19 or NONE");
        if (P == NONE) {
            return;
        }
        if (P < 8) {
            DDRD &= ~mask(P);
        } else if (P < 14) {
            DDRB &= ~mask(P);
        } else {
            DDRC &= ~mask(P);
        }
        if (pullUp) {
            pinHigh<P>();
        } else {
            pinLow<P>();
        }
    }

    // true = HIGH; NONE reads false
    template <uint8_t P>
    inline bool pinRead() {
        static_assert(optional(P), "Uno: pin must be 0 ~ 19 or NONE");
        if (P == NONE) {
            return false;
        }
        if (P < 8) {
            return (PIND & mask(P)) != 0;
        }
        if (P < 14) {
            return (PINB & mask(P)) != 0;
        }
        return (PINC & mask(P)) != 0;
    }

    // ---------- SPI ----------

    // hardware SPI master on D11 (MOSI) / D12 (MISO) / D13 (SCK), frequency / 2 (8 MHz)
    // mode 0 ~ 3: CPOL = bit 1, CPHA = bit 0
    inline void spiSet(uint8_t mode) {
        // SS (D10) must be output BEFORE master mode,
        // otherwise a LOW on SS drops SPI into slave mode
        DDRB |= mask(10) | mask(11) | mask(13);

        // SPR1 = 0, SPR0 = 0: frequency / 4
        SPCR = (1 << SPE) | (1 << MSTR) | ((mode & 0x03) << CPHA);

        // SPI2X: frequency * 2 -> frequency / 2
        SPSR |= (1 << SPI2X);
    }

    // ---------- UART ----------

    // USART0 on D0 (RX) / D1 (TX) = USB serial, 8N1, double speed (U2X): baud = F_CPU / (8 * (ubrr + 1))
    // polling only: no interrupt, so Arduino Serial must not be used at the same time
    inline void uartSet(uint16_t ubrr) {
        UCSR0A = (1 << U2X0);
        UBRR0 = ubrr;
        UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
        UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    }

    // ubrr closest to a baud rate, and the baud rate it really gives
    inline constexpr uint16_t uartUbrr(uint32_t baud) {
        return (uint16_t)((F_CPU + 4UL * baud) / (8UL * baud) - 1);
    }

    inline constexpr uint32_t uartRealBaud(uint32_t baud) {
        return F_CPU / (8UL * (uartUbrr(baud) + 1UL));
    }

    // error of the real baud rate, in 0.1 %
    inline constexpr uint32_t uartErrorPermille(uint32_t baud) {
        return uartRealBaud(baud) > baud ? (uartRealBaud(baud) - baud) * 1000UL / baud
                                         : (baud - uartRealBaud(baud)) * 1000UL / baud;
    }

    // ---------- PWM ----------

    // 8-bit fast PWM, ~976 Hz; duty 0 = LOW, 255 = HIGH (output disconnected from the timer)
    // non-PWM pins: duty > 0 = HIGH, 0 = LOW; NONE: nothing
    // Timer0 (D5, D6): only the output is touched, its mode belongs to millis / delay
    template <uint8_t P>
    inline void pwmSet(uint8_t duty) {
        static_assert(optional(P), "Uno: pin must be 0 ~ 19 or NONE");
        if (P == NONE) {
            return;
        }
        pinOutput<P>();

        bool full = (duty == 0 || duty == 255) || !has(P, function::PWM);
        if (full) {
            // disconnect the timer output, drive the pin directly
            if (P == 6) {
                TCCR0A &= ~(1 << COM0A1);
            } else if (P == 5) {
                TCCR0A &= ~(1 << COM0B1);
            } else if (P == 9) {
                TCCR1A &= ~(1 << COM1A1);
            } else if (P == 10) {
                TCCR1A &= ~(1 << COM1B1);
            } else if (P == 11) {
                TCCR2A &= ~(1 << COM2A1);
            } else if (P == 3) {
                TCCR2A &= ~(1 << COM2B1);
            }
            if (duty == 0) {
                pinLow<P>();
            } else {
                pinHigh<P>();
            }
            return;
        }

        if (P == 6 || P == 5) {
            if (P == 6) {
                OCR0A = duty;
                TCCR0A |= (1 << COM0A1);
            } else {
                OCR0B = duty;
                TCCR0A |= (1 << COM0B1);
            }
        } else if (P == 9 || P == 10) {
            // fast PWM 8-bit (WGM = 0101), clk / 64
            TCCR1A = (TCCR1A & ~(1 << WGM11)) | (1 << WGM10);
            TCCR1B = (TCCR1B & ~((1 << WGM13) | (1 << CS12))) | (1 << WGM12) | (1 << CS11) | (1 << CS10);
            if (P == 9) {
                OCR1A = duty;
                TCCR1A |= (1 << COM1A1);
            } else {
                OCR1B = duty;
                TCCR1A |= (1 << COM1B1);
            }
        } else {
            // P == 11 || P == 3
            // fast PWM (WGM = 011), clk / 64
            TCCR2A |= (1 << WGM21) | (1 << WGM20);
            TCCR2B = (TCCR2B & ~((1 << WGM22) | (1 << CS21) | (1 << CS20))) | (1 << CS22);
            if (P == 11) {
                OCR2A = duty;
                TCCR2A |= (1 << COM2A1);
            } else {
                OCR2B = duty;
                TCCR2A |= (1 << COM2B1);
            }
        }
    }

    // ---------- flash (PROGMEM) ----------

    inline uint8_t readFlash8(const uint8_t *address) {
        return pgm_read_byte(address);
    }

    inline uint16_t readFlash16(const uint16_t *address) {
        return pgm_read_word(address);
    }

    // ---------- delay ----------

    // busy-wait, independent of Arduino timer0
    inline void delayMs(uint16_t ms) {
        while (ms--) {
            _delay_ms(1);
        }
    }

    // ---------- time ----------

    // ms since power on (Arduino millis, Timer0), wraps after ~49 days
    inline uint32_t nowMs() {
        return millis();
    }
}

#endif
