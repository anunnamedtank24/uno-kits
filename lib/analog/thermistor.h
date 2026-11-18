#ifndef ANALOG_THERMISTOR_H
#define ANALOG_THERMISTOR_H

#include <stdint.h>
#include <math.h>

namespace analog {
    // an NTC thermistor (its resistance falls as it warms) in a voltage divider with a fixed resistor, e.g. KY-013
    //
    //   NTC_ON_GND = true:   VCC -- fixed -- (pin) -- NTC -- GND     warmer = lower reading
    //   NTC_ON_GND = false:  VCC -- NTC -- (pin) -- fixed -- GND     warmer = higher reading
    //
    // the temperature by the Beta equation:  1 / T = 1 / T0 + ln(R / R0) / BETA   (T in kelvin)
    //   R0_OHMS:    the NTC at 25 °C (10 kΩ for most)
    //   BETA:       from its datasheet (3950 for most cheap ones; 3435, 4100 ...)
    //   FIXED_OHMS: the other resistor of the divider (10 kΩ on most modules)
    // accuracy: ± 1 ~ 2 °C with the typical values; for better, measure R0 at a known temperature
    //
    // AnalogPin: as for Input (lib/Uno/Uno_analog_pin.h, UnoAnalogPin): init, readAnalogAverage, MAX
    // float math (logf): ~ 1 KB of flash
    template <class AnalogPin, bool NTC_ON_GND = true, uint32_t R0_OHMS = 10000, uint16_t BETA = 3950,
              uint32_t FIXED_OHMS = 10000>
    class Thermistor {
    public:
        inline void init();

        // reads (an average of 8, ~ 1 ms) and returns the temperature, 0.1 °C;
        // -32768 if the reading is at an end (open or shorted: not connected?)
        inline int16_t readTemperature();

        // the NTC's resistance of the last reading, Ω (0 if open or shorted)
        inline uint32_t ohms() const;

        // a known temperature now (0.1 °C, e.g. from a DS18B20 next to it): corrects R0
        inline void calibrate(int16_t tenthsC);

        static constexpr int16_t INVALID = -32768;

    private:
        uint32_t _ohms = 0;
        float _r0 = (float)R0_OHMS;
    };

    // ================= definitions =================

    template <class AnalogPin, bool NTC_ON_GND, uint32_t R0, uint16_t BETA, uint32_t FIXED>
    inline void Thermistor<AnalogPin, NTC_ON_GND, R0, BETA, FIXED>::init() {
        AnalogPin::init();
    }

    template <class AnalogPin, bool NTC_ON_GND, uint32_t R0, uint16_t BETA, uint32_t FIXED>
    inline int16_t Thermistor<AnalogPin, NTC_ON_GND, R0, BETA, FIXED>::readTemperature() {
        uint16_t reading = AnalogPin::readAnalogAverage(8);
        if (reading == 0 || reading >= AnalogPin::MAX) {
            _ohms = 0;
            return INVALID;
        }
        // the divider: NTC on GND -> R = fixed × reading / (MAX - reading)
        float ratio = (float)reading / (float)(AnalogPin::MAX - reading);
        float ohms = NTC_ON_GND ? (float)FIXED * ratio : (float)FIXED / ratio;
        _ohms = (uint32_t)ohms;
        float inverseKelvin = 1.0f / 298.15f + logf(ohms / _r0) / (float)BETA;
        float celsius = 1.0f / inverseKelvin - 273.15f;
        return (int16_t)(celsius * 10.0f + (celsius >= 0 ? 0.5f : -0.5f));
    }

    template <class AnalogPin, bool NTC_ON_GND, uint32_t R0, uint16_t BETA, uint32_t FIXED>
    inline uint32_t Thermistor<AnalogPin, NTC_ON_GND, R0, BETA, FIXED>::ohms() const {
        return _ohms;
    }

    template <class AnalogPin, bool NTC_ON_GND, uint32_t R0, uint16_t BETA, uint32_t FIXED>
    inline void Thermistor<AnalogPin, NTC_ON_GND, R0, BETA, FIXED>::calibrate(int16_t tenthsC) {
        if (readTemperature() == INVALID || _ohms == 0) {
            return;
        }
        // R0 = R / exp(BETA × (1/T - 1/T0))
        float kelvin = (float)tenthsC / 10.0f + 273.15f;
        _r0 = (float)_ohms / expf((float)BETA * (1.0f / kelvin - 1.0f / 298.15f));
    }
}

#endif
