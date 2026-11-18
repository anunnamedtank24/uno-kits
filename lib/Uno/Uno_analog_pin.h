#ifndef UNO_ANALOG_PIN_H
#define UNO_ANALOG_PIN_H

#include "Uno_pin.h"

namespace Uno {
    // what 1023 means: the voltage the ADC compares with
    enum AnalogReference : uint8_t {
        REFERENCE_VCC,        // AVcc = the 5V pin (~ 4.6 ~ 5.2 V on USB): sensors powered from 5V (the default)
        REFERENCE_INTERNAL,   // the chip's own 1.1 V (± 10 %, steady): small voltages, battery checks
        REFERENCE_EXTERNAL    // the AREF pin: whatever is wired there (0 ~ 5 V); never with the others on AREF
    };

    // one analog input A0 ~ A5 (14 ~ 19), e.g. a potentiometer, a light sensor, a joystick axis (lib/analog)
    // the ADC: 10 bits (0 ~ 1023), ~ 112 µs a reading (clock 125 kHz), one reading at a time
    // the pin stays a digital input too: read() works as for UnoInputPin
    //
    // REFERENCE: what 1023 means (see AnalogReference); REFERENCE_MILLIVOLTS: its voltage for readMillivolts()
    // (VCC: 5000, or measure the 5V pin with a meter; INTERNAL: 1100, or measure AREF while it is used)
    template <uint8_t PIN, AnalogReference REFERENCE = REFERENCE_VCC,
              uint16_t REFERENCE_MILLIVOLTS = (REFERENCE == REFERENCE_INTERNAL ? 1100 : 5000)>
    struct UnoAnalogPin : UnoInputPin<PIN, false> {
        static_assert(has(PIN, function::ANALOG_IN), "Uno::UnoAnalogPin: PIN must be A0 ~ A5 (14 ~ 19)");

        // ---------- AnalogPin functions ----------

        // the largest reading
        static constexpr uint16_t MAX = 1023;

        // input (no pull-up), ADC on
        static inline void init();

        // 0 ~ 1023 (0 V ~ the reference), waits ~ 112 µs
        static inline uint16_t readAnalog();

        // the mean of count readings (1 ~ 64): less noise, count × 112 µs
        static inline uint16_t readAnalogAverage(uint8_t count);

        // the voltage, mV (with REFERENCE_MILLIVOLTS)
        static inline uint16_t readMillivolts();

        static inline uint16_t referenceMillivolts();

        // read() (digital), nowMs ... : from UnoInputPin / UnoBasicPin / UnoBoard
    };

    // the chip itself, measured with the ADC
    struct UnoChip : UnoBoard {
        // the real voltage of the 5V pin, mV (the 1.1 V reference measured against it; ± 10 % unless calibrated:
        // BANDGAP_MILLIVOLTS); to correct REFERENCE_MILLIVOLTS of the VCC pins, or to watch a battery
        static inline uint16_t readVccMillivolts();

        // the chip's temperature, 0.1 °C (its own sensor: ± 10 °C without calibration, warmer than the air)
        static inline int16_t readTemperature();

        // the 1.1 V reference of this chip, mV: measure it on AREF (a meter) while readVccMillivolts runs
        static constexpr uint16_t BANDGAP_MILLIVOLTS = 1100;
    };

    namespace analog_state {
        // the last ADMUX value, to wait after a change of the reference
        inline uint8_t lastMux = 0xFF;

        // REFS bits: VCC 01, INTERNAL 11, EXTERNAL 00
        inline constexpr uint8_t refsBits(AnalogReference reference) {
            return reference == REFERENCE_VCC ? (1 << REFS0)
                 : reference == REFERENCE_INTERNAL ? ((1 << REFS1) | (1 << REFS0))
                 : 0;
        }

        // one conversion of channel (0 ~ 8, 14 = 1.1 V), with the reference bits
        inline uint16_t convert(uint8_t channel, uint8_t refs) {
            ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);   // on, clock / 128
            uint8_t mux = (uint8_t)(refs | (channel & 0x0F));
            if (mux != lastMux) {
                ADMUX = mux;
                lastMux = mux;
                // the reference capacitor and the multiplexer settle: wait, then throw one reading away
                delayMicroseconds(400);
                ADCSRA |= (1 << ADSC);
                while (ADCSRA & (1 << ADSC)) {
                }
            }
            ADCSRA |= (1 << ADSC);
            while (ADCSRA & (1 << ADSC)) {
            }
            return ADC;
        }
    }

    // ================= definitions =================

    template <uint8_t PIN, AnalogReference REFERENCE, uint16_t REFERENCE_MILLIVOLTS>
    inline void UnoAnalogPin<PIN, REFERENCE, REFERENCE_MILLIVOLTS>::init() {
        pinInput<PIN>(false);
        ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    }

    template <uint8_t PIN, AnalogReference REFERENCE, uint16_t REFERENCE_MILLIVOLTS>
    inline uint16_t UnoAnalogPin<PIN, REFERENCE, REFERENCE_MILLIVOLTS>::readAnalog() {
        return analog_state::convert((uint8_t)(PIN - 14), analog_state::refsBits(REFERENCE));
    }

    template <uint8_t PIN, AnalogReference REFERENCE, uint16_t REFERENCE_MILLIVOLTS>
    inline uint16_t UnoAnalogPin<PIN, REFERENCE, REFERENCE_MILLIVOLTS>::readAnalogAverage(uint8_t count) {
        if (count < 1) {
            count = 1;
        } else if (count > 64) {
            count = 64;
        }
        uint32_t sum = 0;
        for (uint8_t i = 0; i < count; i++) {
            sum += readAnalog();
        }
        return (uint16_t)((sum + count / 2) / count);
    }

    template <uint8_t PIN, AnalogReference REFERENCE, uint16_t REFERENCE_MILLIVOLTS>
    inline uint16_t UnoAnalogPin<PIN, REFERENCE, REFERENCE_MILLIVOLTS>::readMillivolts() {
        return (uint16_t)(((uint32_t)readAnalog() * REFERENCE_MILLIVOLTS + 511) / 1023);
    }

    template <uint8_t PIN, AnalogReference REFERENCE, uint16_t REFERENCE_MILLIVOLTS>
    inline uint16_t UnoAnalogPin<PIN, REFERENCE, REFERENCE_MILLIVOLTS>::referenceMillivolts() {
        return REFERENCE_MILLIVOLTS;
    }

    inline uint16_t UnoChip::readVccMillivolts() {
        // channel 14 = the 1.1 V bandgap, against AVcc: reading = 1.1 V × 1023 / Vcc
        uint16_t reading = analog_state::convert(14, analog_state::refsBits(REFERENCE_VCC));
        if (reading == 0) {
            return 0;
        }
        return (uint16_t)(((uint32_t)BANDGAP_MILLIVOLTS * 1023UL + reading / 2) / reading);
    }

    inline int16_t UnoChip::readTemperature() {
        // channel 8 against the internal 1.1 V: 314 mV at 25 °C, ~ 1 mV a °C (datasheet, typical)
        uint16_t reading = analog_state::convert(8, analog_state::refsBits(REFERENCE_INTERNAL));
        int32_t tenthsMillivolts = (int32_t)reading * BANDGAP_MILLIVOLTS * 10L / 1023L;
        return (int16_t)(tenthsMillivolts - 3140 + 250);
    }
}

#endif
