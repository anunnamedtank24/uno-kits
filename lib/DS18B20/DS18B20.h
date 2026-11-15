#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

namespace DS18B20 {
    // DS18B20 commands (datasheet: ROM commands, function commands)
    constexpr uint8_t SKIP_ROM         = 0xCC;
    constexpr uint8_t CONVERT_T        = 0x44;
    constexpr uint8_t WRITE_SCRATCHPAD = 0x4E;
    constexpr uint8_t READ_SCRATCHPAD  = 0xBE;

    // scratchpad: temperature LSB, MSB, TH, TL, configuration, 3 reserved, CRC
    constexpr uint8_t SCRATCHPAD_SIZE = 9;

    // the temperature register before the first conversion: 85.0 °C
    constexpr int16_t POWER_ON_RAW = 0x0550;

    // Thermometer: the DS18B20 digital thermometer (VCC, DQ, GND; the bare part, the waterproof probe and KY-001 alike), one sensor on the wire (Skip ROM)
    // powered from VCC (3.0 ~ 5.5 V); parasite power (VCC tied to GND) is not supported
    //
    // Wire: a struct of static functions (see lib/Uno/Uno_one_wire.h)
    //   void init()
    //   bool reset()             true if a device answered
    //   void write8(uint8_t)     least significant bit first
    //   uint8_t read8()
    //   bool readBit()
    //   uint32_t nowMs()
    //
    // raw value: signed, 1/16 °C per step (-55 ~ +125 °C = -880 ~ 2000)
    // functions return a result: OK or one of the errors below
    template <class Wire>
    class Thermometer {
    public:
        static constexpr uint8_t OK = 0;
        static constexpr uint8_t ERROR_NO_SENSOR = 1;  // no presence pulse: wiring, pull-up, power
        static constexpr uint8_t ERROR_CRC = 2;        // scratchpad CRC wrong: noise, long wire, weak pull-up
        static constexpr uint8_t ERROR_TIMEOUT = 3;    // conversion not done in time (e.g. parasite power)
        static constexpr uint8_t ERROR_RESOLUTION = 4; // bits not 9 ~ 12

        // init the wire and set the resolution
        inline uint8_t init(uint8_t bits = 12);

        // 9 ~ 12 bits: steps of 0.5 / 0.25 / 0.125 / 0.0625 °C, conversions of 94 / 188 / 375 / 750 ms
        // kept in the sensor's RAM only: lost when it loses power (init() sets it again)
        inline uint8_t setResolution(uint8_t bits);
        inline uint8_t resolution() const;
        inline uint16_t conversionMs() const;

        // ---------- step by step (no waiting) ----------

        // starts a conversion: ask isConversionDone(), or wait conversionMs(), then readRaw()
        inline uint8_t startConversion();

        // only right after startConversion(): the sensor answers 0 while converting
        inline bool isConversionDone();

        // the last conversion; unused low bits of a lower resolution are cleared
        inline uint8_t readRaw(int16_t &raw);

        // ---------- all in one (waits up to conversionMs() + 50 ms) ----------

        inline uint8_t readCentiCelsius(int16_t &centiCelsius);

        // ---------- conversions ----------

        // 1/100 °C, rounded: 376 (23.5 °C) -> 2350
        static inline int16_t toCentiCelsius(int16_t raw);

        // floating point (about 1 KB of flash if nothing else uses float)
        static inline float toCelsius(int16_t raw);

        // Dallas / Maxim CRC-8 (x^8 + x^5 + x^4 + 1)
        static inline uint8_t crc8(const uint8_t *data, uint8_t length);

    private:
        uint8_t _bits = 12;

        // reset + Skip ROM
        inline uint8_t select();

        inline uint8_t readScratchpad(uint8_t *data);
    };

    // ================= definitions =================

    template <class Wire>
    inline uint8_t Thermometer<Wire>::init(uint8_t bits) {
        Wire::init();
        return setResolution(bits);
    }

    template <class Wire>
    inline uint8_t Thermometer<Wire>::setResolution(uint8_t bits) {
        if (bits < 9 || bits > 12) {
            return ERROR_RESOLUTION;
        }
        // TH / TL (alarm bytes) are written back unchanged
        uint8_t data[SCRATCHPAD_SIZE];
        uint8_t result = readScratchpad(data);
        if (result != OK) {
            return result;
        }
        result = select();
        if (result != OK) {
            return result;
        }
        Wire::write8(WRITE_SCRATCHPAD);
        Wire::write8(data[2]);
        Wire::write8(data[3]);
        // configuration: 0 R1 R0 1 1 1 1 1
        Wire::write8((uint8_t)(((bits - 9) << 5) | 0x1F));
        _bits = bits;
        return OK;
    }

    template <class Wire>
    inline uint8_t Thermometer<Wire>::resolution() const {
        return _bits;
    }

    template <class Wire>
    inline uint16_t Thermometer<Wire>::conversionMs() const {
        // 750 ms at 12 bits, halved per bit less (rounded up)
        return (uint16_t)((750 + (1 << (12 - _bits)) - 1) >> (12 - _bits));
    }

    template <class Wire>
    inline uint8_t Thermometer<Wire>::startConversion() {
        uint8_t result = select();
        if (result != OK) {
            return result;
        }
        Wire::write8(CONVERT_T);
        return OK;
    }

    template <class Wire>
    inline bool Thermometer<Wire>::isConversionDone() {
        return Wire::readBit();
    }

    template <class Wire>
    inline uint8_t Thermometer<Wire>::readRaw(int16_t &raw) {
        uint8_t data[SCRATCHPAD_SIZE];
        uint8_t result = readScratchpad(data);
        if (result != OK) {
            return result;
        }
        raw = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
        // 9 bits: bits 0 ~ 2 undefined, 10 bits: 0 ~ 1, 11 bits: 0
        raw &= (int16_t)~((1 << (12 - _bits)) - 1);
        return OK;
    }

    template <class Wire>
    inline uint8_t Thermometer<Wire>::readCentiCelsius(int16_t &centiCelsius) {
        uint8_t result = startConversion();
        if (result != OK) {
            return result;
        }
        uint32_t startMs = Wire::nowMs();
        while (!isConversionDone()) {
            if (Wire::nowMs() - startMs > (uint32_t)conversionMs() + 50) {
                return ERROR_TIMEOUT;
            }
        }
        int16_t raw;
        result = readRaw(raw);
        if (result != OK) {
            return result;
        }
        centiCelsius = toCentiCelsius(raw);
        return OK;
    }

    template <class Wire>
    inline int16_t Thermometer<Wire>::toCentiCelsius(int16_t raw) {
        // raw * 100 / 16 = raw * 25 / 4, rounded half away from zero
        int32_t scaled = (int32_t)raw * 25;
        scaled += scaled >= 0 ? 2 : -2;
        return (int16_t)(scaled / 4);
    }

    template <class Wire>
    inline float Thermometer<Wire>::toCelsius(int16_t raw) {
        return raw / 16.0f;
    }

    template <class Wire>
    inline uint8_t Thermometer<Wire>::crc8(const uint8_t *data, uint8_t length) {
        uint8_t crc = 0;
        for (uint8_t i = 0; i < length; i++) {
            uint8_t byte = data[i];
            for (uint8_t bit = 0; bit < 8; bit++) {
                bool mix = (crc ^ byte) & 0x01;
                crc >>= 1;
                if (mix) {
                    crc ^= 0x8C;
                }
                byte >>= 1;
            }
        }
        return crc;
    }

    template <class Wire>
    inline uint8_t Thermometer<Wire>::select() {
        if (!Wire::reset()) {
            return ERROR_NO_SENSOR;
        }
        Wire::write8(SKIP_ROM);
        return OK;
    }

    template <class Wire>
    inline uint8_t Thermometer<Wire>::readScratchpad(uint8_t *data) {
        uint8_t result = select();
        if (result != OK) {
            return result;
        }
        Wire::write8(READ_SCRATCHPAD);
        for (uint8_t i = 0; i < SCRATCHPAD_SIZE; i++) {
            data[i] = Wire::read8();
        }
        // all 0xFF: nothing drove the line (e.g. the sensor let go after the presence pulse)
        bool allHigh = true;
        for (uint8_t i = 0; i < SCRATCHPAD_SIZE; i++) {
            if (data[i] != 0xFF) {
                allHigh = false;
            }
        }
        if (allHigh) {
            return ERROR_NO_SENSOR;
        }
        if (crc8(data, SCRATCHPAD_SIZE - 1) != data[SCRATCHPAD_SIZE - 1]) {
            return ERROR_CRC;
        }
        return OK;
    }
}

#endif
