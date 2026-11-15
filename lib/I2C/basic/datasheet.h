#ifndef I2C_DATASHEET_H
#define I2C_DATASHEET_H

#include <stdint.h>

// SSD1306 OLED controller (128 x 64 dots, monochrome) over I2C
// reference: Solomon Systech SSD1306 datasheet Rev 1.1:
//   8.1.5 I2C interface, 9 command table (Table 9-1), 10 command descriptions,
//   charge pump: application note "Charge Pump Regulator" (command 8Dh)
//
// I2C (8.1.5): address 0111 10 SA0 = 0x3C (SA0 = 0, most modules) or 0x3D,
// then a control byte: Co (bit 7) = 0: only data bytes follow, D/C# (bit 6): 0 = commands, 1 = RAM data
//   0x00 command stream: every following byte is a command or a command's parameter
//   0x40 data stream:    every following byte goes to the display RAM
// a read returns the status byte (bit 6 = 1: display off); the display RAM cannot be read over I2C
//
// display RAM (GDDRAM, 10.1.3): 128 columns x 8 pages; a byte is 8 rows of one column,
// bit 0 at the top of its page
//
// Bus: a struct providing these static functions (see lib/Uno/Uno_I2C.h)
//   void init()
//   bool start(uint8_t address, bool read)   START + address; true if ACK
//   bool write8(uint8_t)                     true if ACK
//   uint8_t read8(bool ack)
//   void stop()
//   void delayMs(uint16_t ms)
//   uint32_t nowMs()
//   uint8_t readFlash8(const uint8_t *), uint16_t readFlash16(const uint16_t *), FlashString
//                                            (used by graphics::Text)
namespace SSD1306 {
    constexpr uint8_t ADDRESS_SA0_LOW  = 0x3C;
    constexpr uint8_t ADDRESS_SA0_HIGH = 0x3D;

    // control bytes (8.1.5.1)
    constexpr uint8_t CONTROL_COMMANDS = 0x00;
    constexpr uint8_t CONTROL_DATA     = 0x40;

    // status byte (8.1.5.2 / 10.1)
    constexpr uint8_t STATUS_DISPLAY_OFF = 0x40;

    // ===== Table 9-1: 1. fundamental commands =====
    constexpr uint8_t SET_CONTRAST          = 0x81;
    constexpr uint8_t ENTIRE_DISPLAY_RESUME = 0xA4; // follow the RAM
    constexpr uint8_t ENTIRE_DISPLAY_ON     = 0xA5; // every dot on, RAM ignored
    constexpr uint8_t NORMAL_DISPLAY        = 0xA6; // RAM 1 = dot on
    constexpr uint8_t INVERSE_DISPLAY       = 0xA7; // RAM 0 = dot on
    constexpr uint8_t DISPLAY_OFF           = 0xAE; // sleep
    constexpr uint8_t DISPLAY_ON            = 0xAF;

    // ===== 2. scrolling commands =====
    constexpr uint8_t RIGHT_HORIZONTAL_SCROLL          = 0x26;
    constexpr uint8_t LEFT_HORIZONTAL_SCROLL           = 0x27;
    constexpr uint8_t VERTICAL_RIGHT_HORIZONTAL_SCROLL = 0x29;
    constexpr uint8_t VERTICAL_LEFT_HORIZONTAL_SCROLL  = 0x2A;
    constexpr uint8_t DEACTIVATE_SCROLL                = 0x2E;
    constexpr uint8_t ACTIVATE_SCROLL                  = 0x2F;
    constexpr uint8_t SET_VERTICAL_SCROLL_AREA         = 0xA3;

    // scroll step interval, in frames (10.2.1)
    constexpr uint8_t SCROLL_2_FRAMES   = 0x07;
    constexpr uint8_t SCROLL_3_FRAMES   = 0x04;
    constexpr uint8_t SCROLL_4_FRAMES   = 0x05;
    constexpr uint8_t SCROLL_5_FRAMES   = 0x00;
    constexpr uint8_t SCROLL_25_FRAMES  = 0x06;
    constexpr uint8_t SCROLL_64_FRAMES  = 0x01;
    constexpr uint8_t SCROLL_128_FRAMES = 0x02;
    constexpr uint8_t SCROLL_256_FRAMES = 0x03;

    // ===== 3. addressing setting commands =====
    constexpr uint8_t SET_LOWER_COLUMN    = 0x00; // + low nibble, page mode only
    constexpr uint8_t SET_HIGHER_COLUMN   = 0x10; // + high nibble, page mode only
    constexpr uint8_t SET_ADDRESSING_MODE = 0x20;
    constexpr uint8_t SET_COLUMN_ADDRESS  = 0x21; // horizontal / vertical mode only
    constexpr uint8_t SET_PAGE_ADDRESS    = 0x22; // horizontal / vertical mode only
    constexpr uint8_t SET_PAGE_START      = 0xB0; // + page 0 ~ 7, page mode only

    constexpr uint8_t ADDRESSING_HORIZONTAL = 0x00; // column by column, then the next page
    constexpr uint8_t ADDRESSING_VERTICAL   = 0x01; // page by page, then the next column
    constexpr uint8_t ADDRESSING_PAGE       = 0x02; // inside one page only (reset)

    // ===== 4. hardware configuration commands =====
    constexpr uint8_t SET_START_LINE        = 0x40; // + line 0 ~ 63
    constexpr uint8_t SEGMENT_REMAP_OFF     = 0xA0; // column 0 -> SEG0 (reset)
    constexpr uint8_t SEGMENT_REMAP_ON      = 0xA1; // column 127 -> SEG0: mirrored left / right
    constexpr uint8_t SET_MULTIPLEX_RATIO   = 0xA8;
    constexpr uint8_t COM_SCAN_NORMAL       = 0xC0; // COM0 -> COM[N-1] (reset)
    constexpr uint8_t COM_SCAN_REMAPPED     = 0xC8; // COM[N-1] -> COM0: mirrored up / down
    constexpr uint8_t SET_DISPLAY_OFFSET    = 0xD3;
    constexpr uint8_t SET_COM_PINS          = 0xDA;

    // ===== 5. timing & driving scheme setting commands =====
    constexpr uint8_t SET_CLOCK             = 0xD5;
    constexpr uint8_t SET_PRECHARGE         = 0xD9;
    constexpr uint8_t SET_VCOMH_DESELECT    = 0xDB;
    constexpr uint8_t NOP                   = 0xE3;

    constexpr uint8_t VCOMH_0_65 = 0x00; // ~0.65 x VCC
    constexpr uint8_t VCOMH_0_77 = 0x20; // ~0.77 x VCC (reset)
    constexpr uint8_t VCOMH_0_83 = 0x30; // ~0.83 x VCC

    // ===== charge pump (application note) =====
    constexpr uint8_t SET_CHARGE_PUMP     = 0x8D;
    constexpr uint8_t CHARGE_PUMP_DISABLE = 0x10; // reset: VCC from outside
    constexpr uint8_t CHARGE_PUMP_ENABLE  = 0x14; // 7.5 V made inside from VBAT (3.3 / 5 V modules)

    // ===== 6. advance graphic commands =====
    constexpr uint8_t SET_FADE_BLINK = 0x23;
    constexpr uint8_t SET_ZOOM_IN    = 0xD6;

    constexpr uint8_t FADE_BLINK_OFF  = 0x00;
    constexpr uint8_t FADE_OUT        = 0x20; // fades out once, stays off
    constexpr uint8_t BLINK           = 0x30; // fades out and in, again and again

    template <class Bus>
    class Datasheet {
    public:
        // the Bus this screen runs on, for layers above (e.g. graphics::Text reads flash with it)
        using BusType = Bus;

        // address: ADDRESS_SA0_LOW (0x3C, most modules) or ADDRESS_SA0_HIGH (0x3D)
        inline Datasheet(uint8_t address = ADDRESS_SA0_LOW);

        inline uint8_t address() const;

        // true if the last transfer was answered (ACK) all the way;
        // false: nothing at the address (wiring, power, pull-ups, address)
        inline bool ok() const;

        // ---------- low level ----------

        // a command stream: START, address, CONTROL_COMMANDS; then writeByte() for each byte, end()
        inline void beginCommands();

        // a data stream into the display RAM, at the address set before
        inline void beginData();

        inline void writeByte(uint8_t byte);
        inline void end();

        // one command with 0 ~ 6 parameters, in one stream
        inline void sendCommand(uint8_t command);
        inline void sendCommand(uint8_t command, uint8_t a);
        inline void sendCommand(uint8_t command, uint8_t a, uint8_t b);
        inline void sendCommand(const uint8_t *bytes, uint8_t count);

        // the status byte: STATUS_DISPLAY_OFF = bit 6; false if nothing answered
        inline bool readStatus(uint8_t &status);

        // ---------- 1. fundamental (in command order) ----------

        // 0 ~ 255 (reset 0x7F): brighter with more
        inline void setContrast(uint8_t contrast);

        // true: every dot on (RAM kept, not shown); false: show the RAM (reset)
        inline void setEntireDisplayOn(bool on);

        // true: RAM 0 = dot on
        inline void setInverse(bool inverse);

        // false: sleep (reset), the RAM is kept
        inline void setDisplayOn(bool on);

        // ---------- 2. scrolling ----------
        // only pages start ~ end move; the RAM itself does not change.
        // set up while scrolling is off (deactivateScroll() first), then activateScroll()

        // left: false = to the right; interval: SCROLL_x_FRAMES; pages 0 ~ 7, end >= start
        inline void setupHorizontalScroll(bool left, uint8_t startPage, uint8_t interval, uint8_t endPage);

        // horizontal as above, and up by verticalOffset rows (1 ~ 63) per step
        // (inside the area of setVerticalScrollArea)
        inline void setupDiagonalScroll(bool left, uint8_t startPage, uint8_t interval, uint8_t endPage,
                                        uint8_t verticalOffset);

        // stop: rewrite the RAM afterwards (the scrolled picture is not kept)
        inline void deactivateScroll();
        inline void activateScroll();

        // rows fixed at the top, then rows that scroll vertically (top + rows <= multiplex ratio)
        inline void setVerticalScrollArea(uint8_t topFixedRows, uint8_t scrollRows);

        // ---------- 3. addressing ----------

        // page mode only: the column (0 ~ 127) where the next data byte goes (sends 00h~0Fh and 10h~1Fh)
        inline void setPageModeColumn(uint8_t column);

        // ADDRESSING_HORIZONTAL / ADDRESSING_VERTICAL / ADDRESSING_PAGE (reset)
        inline void setAddressingMode(uint8_t mode);

        // horizontal / vertical mode: the window the data stream fills, inclusive
        // columns 0 ~ 127 (reset 0, 127), pages 0 ~ 7 (reset 0, 7)
        inline void setColumnAddress(uint8_t start, uint8_t end);
        inline void setPageAddress(uint8_t start, uint8_t end);

        // page mode only: the page 0 ~ 7
        inline void setPageStart(uint8_t page);

        // ---------- 4. hardware configuration ----------

        // the RAM row shown on the top line, 0 ~ 63 (reset 0): scrolls the picture up
        inline void setStartLine(uint8_t line);

        // true: column 127 on the left (mirrored left / right)
        inline void setSegmentRemap(bool remap);

        // rows driven, 16 ~ 64 (reset 64): 64 for a 128x64 panel, 32 for 128x32
        inline void setMultiplexRatio(uint8_t rows);

        // true: COM[N-1] at the top (mirrored up / down)
        inline void setComScanRemap(bool remap);

        // the rows shift up by offset, 0 ~ 63 (reset 0)
        inline void setDisplayOffset(uint8_t offset);

        // how COM pins are wired to the panel (reset: alternative, no left / right remap):
        // 128x64 panels: alternative = true; 128x32: false
        inline void setComPins(bool alternative, bool leftRightRemap);

        // ---------- 5. timing & driving scheme ----------

        // divide 1 ~ 16 (reset 1), frequency 0 ~ 15 (reset 8): faster frames with a higher frequency
        inline void setClock(uint8_t divide, uint8_t frequency);

        // phase 1 / phase 2 in clocks, 1 ~ 15 each (reset 2, 2); internal charge pump: 1, 15
        inline void setPrecharge(uint8_t phase1, uint8_t phase2);

        // VCOMH_0_65 / VCOMH_0_77 (reset) / VCOMH_0_83
        inline void setVcomhDeselect(uint8_t level);

        inline void nop();

        // ---------- charge pump ----------

        // true for modules without an external 7 ~ 15 V supply (almost every 3.3 / 5 V module);
        // takes effect with the next setDisplayOn(true)
        inline void setChargePump(bool enable);

        // ---------- 6. advance graphic ----------

        // FADE_BLINK_OFF / FADE_OUT / BLINK; interval: 0 ~ 15, one contrast step every 8 (interval + 1) frames
        inline void setFadeBlink(uint8_t mode, uint8_t interval);

        // true: every row shown twice (the top half fills the panel); needs setComPins(true, …)
        inline void setZoomIn(bool zoom);

    private:
        uint8_t _address;
        bool _ok = false;
    };

    // ================= definitions =================

    template <class Bus>
    inline Datasheet<Bus>::Datasheet(uint8_t address)
        : _address(address) {
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::address() const {
        return _address;
    }

    template <class Bus>
    inline bool Datasheet<Bus>::ok() const {
        return _ok;
    }

    // ---------- low level ----------

    template <class Bus>
    inline void Datasheet<Bus>::beginCommands() {
        _ok = Bus::start(_address, false);
        writeByte(CONTROL_COMMANDS);
    }

    template <class Bus>
    inline void Datasheet<Bus>::beginData() {
        _ok = Bus::start(_address, false);
        writeByte(CONTROL_DATA);
    }

    template <class Bus>
    inline void Datasheet<Bus>::writeByte(uint8_t byte) {
        if (!Bus::write8(byte)) {
            _ok = false;
        }
    }

    template <class Bus>
    inline void Datasheet<Bus>::end() {
        Bus::stop();
    }

    template <class Bus>
    inline void Datasheet<Bus>::sendCommand(uint8_t command) {
        sendCommand(&command, 1);
    }

    template <class Bus>
    inline void Datasheet<Bus>::sendCommand(uint8_t command, uint8_t a) {
        const uint8_t bytes[2] = { command, a };
        sendCommand(bytes, 2);
    }

    template <class Bus>
    inline void Datasheet<Bus>::sendCommand(uint8_t command, uint8_t a, uint8_t b) {
        const uint8_t bytes[3] = { command, a, b };
        sendCommand(bytes, 3);
    }

    template <class Bus>
    inline void Datasheet<Bus>::sendCommand(const uint8_t *bytes, uint8_t count) {
        beginCommands();
        for (uint8_t i = 0; i < count; i++) {
            writeByte(bytes[i]);
        }
        end();
    }

    template <class Bus>
    inline bool Datasheet<Bus>::readStatus(uint8_t &status) {
        _ok = Bus::start(_address, true);
        if (_ok) {
            status = Bus::read8(false);
        }
        Bus::stop();
        return _ok;
    }

    // ---------- 1. fundamental ----------

    template <class Bus>
    inline void Datasheet<Bus>::setContrast(uint8_t contrast) {
        sendCommand(SET_CONTRAST, contrast);
    }

    template <class Bus>
    inline void Datasheet<Bus>::setEntireDisplayOn(bool on) {
        sendCommand(on ? ENTIRE_DISPLAY_ON : ENTIRE_DISPLAY_RESUME);
    }

    template <class Bus>
    inline void Datasheet<Bus>::setInverse(bool inverse) {
        sendCommand(inverse ? INVERSE_DISPLAY : NORMAL_DISPLAY);
    }

    template <class Bus>
    inline void Datasheet<Bus>::setDisplayOn(bool on) {
        sendCommand(on ? DISPLAY_ON : DISPLAY_OFF);
    }

    // ---------- 2. scrolling ----------

    template <class Bus>
    inline void Datasheet<Bus>::setupHorizontalScroll(bool left, uint8_t startPage, uint8_t interval,
                                                      uint8_t endPage) {
        // A dummy 00h, B start page, C interval, D end page, E dummy 00h, F dummy FFh
        const uint8_t bytes[7] = {
            left ? LEFT_HORIZONTAL_SCROLL : RIGHT_HORIZONTAL_SCROLL,
            0x00, (uint8_t)(startPage & 0x07), (uint8_t)(interval & 0x07), (uint8_t)(endPage & 0x07),
            0x00, 0xFF
        };
        sendCommand(bytes, 7);
    }

    template <class Bus>
    inline void Datasheet<Bus>::setupDiagonalScroll(bool left, uint8_t startPage, uint8_t interval,
                                                    uint8_t endPage, uint8_t verticalOffset) {
        // A dummy 00h, B start page, C interval, D end page, E vertical offset
        const uint8_t bytes[6] = {
            left ? VERTICAL_LEFT_HORIZONTAL_SCROLL : VERTICAL_RIGHT_HORIZONTAL_SCROLL,
            0x00, (uint8_t)(startPage & 0x07), (uint8_t)(interval & 0x07), (uint8_t)(endPage & 0x07),
            (uint8_t)(verticalOffset & 0x3F)
        };
        sendCommand(bytes, 6);
    }

    template <class Bus>
    inline void Datasheet<Bus>::deactivateScroll() {
        sendCommand(DEACTIVATE_SCROLL);
    }

    template <class Bus>
    inline void Datasheet<Bus>::activateScroll() {
        sendCommand(ACTIVATE_SCROLL);
    }

    template <class Bus>
    inline void Datasheet<Bus>::setVerticalScrollArea(uint8_t topFixedRows, uint8_t scrollRows) {
        sendCommand(SET_VERTICAL_SCROLL_AREA, (uint8_t)(topFixedRows & 0x3F), (uint8_t)(scrollRows & 0x7F));
    }

    // ---------- 3. addressing ----------

    template <class Bus>
    inline void Datasheet<Bus>::setPageModeColumn(uint8_t column) {
        sendCommand((uint8_t)(SET_LOWER_COLUMN | (column & 0x0F)), (uint8_t)(SET_HIGHER_COLUMN | ((column >> 4) & 0x07)));
    }

    template <class Bus>
    inline void Datasheet<Bus>::setAddressingMode(uint8_t mode) {
        sendCommand(SET_ADDRESSING_MODE, (uint8_t)(mode & 0x03));
    }

    template <class Bus>
    inline void Datasheet<Bus>::setColumnAddress(uint8_t start, uint8_t end) {
        sendCommand(SET_COLUMN_ADDRESS, (uint8_t)(start & 0x7F), (uint8_t)(end & 0x7F));
    }

    template <class Bus>
    inline void Datasheet<Bus>::setPageAddress(uint8_t start, uint8_t end) {
        sendCommand(SET_PAGE_ADDRESS, (uint8_t)(start & 0x07), (uint8_t)(end & 0x07));
    }

    template <class Bus>
    inline void Datasheet<Bus>::setPageStart(uint8_t page) {
        sendCommand((uint8_t)(SET_PAGE_START | (page & 0x07)));
    }

    // ---------- 4. hardware configuration ----------

    template <class Bus>
    inline void Datasheet<Bus>::setStartLine(uint8_t line) {
        sendCommand((uint8_t)(SET_START_LINE | (line & 0x3F)));
    }

    template <class Bus>
    inline void Datasheet<Bus>::setSegmentRemap(bool remap) {
        sendCommand(remap ? SEGMENT_REMAP_ON : SEGMENT_REMAP_OFF);
    }

    template <class Bus>
    inline void Datasheet<Bus>::setMultiplexRatio(uint8_t rows) {
        // parameter N - 1: 15 ~ 63
        rows = rows < 16 ? 16 : (rows > 64 ? 64 : rows);
        sendCommand(SET_MULTIPLEX_RATIO, (uint8_t)(rows - 1));
    }

    template <class Bus>
    inline void Datasheet<Bus>::setComScanRemap(bool remap) {
        sendCommand(remap ? COM_SCAN_REMAPPED : COM_SCAN_NORMAL);
    }

    template <class Bus>
    inline void Datasheet<Bus>::setDisplayOffset(uint8_t offset) {
        sendCommand(SET_DISPLAY_OFFSET, (uint8_t)(offset & 0x3F));
    }

    template <class Bus>
    inline void Datasheet<Bus>::setComPins(bool alternative, bool leftRightRemap) {
        // 0 0 A5 A4 0 0 1 0
        sendCommand(SET_COM_PINS, (uint8_t)(0x02 | (alternative ? 0x10 : 0) | (leftRightRemap ? 0x20 : 0)));
    }

    // ---------- 5. timing & driving scheme ----------

    template <class Bus>
    inline void Datasheet<Bus>::setClock(uint8_t divide, uint8_t frequency) {
        // A[3:0] = divide - 1, A[7:4] = frequency
        divide = divide < 1 ? 1 : (divide > 16 ? 16 : divide);
        sendCommand(SET_CLOCK, (uint8_t)(((frequency & 0x0F) << 4) | (divide - 1)));
    }

    template <class Bus>
    inline void Datasheet<Bus>::setPrecharge(uint8_t phase1, uint8_t phase2) {
        // A[3:0] = phase 1, A[7:4] = phase 2; 0 is invalid
        phase1 = phase1 < 1 ? 1 : (phase1 > 15 ? 15 : phase1);
        phase2 = phase2 < 1 ? 1 : (phase2 > 15 ? 15 : phase2);
        sendCommand(SET_PRECHARGE, (uint8_t)((phase2 << 4) | phase1));
    }

    template <class Bus>
    inline void Datasheet<Bus>::setVcomhDeselect(uint8_t level) {
        sendCommand(SET_VCOMH_DESELECT, (uint8_t)(level & 0x70));
    }

    template <class Bus>
    inline void Datasheet<Bus>::nop() {
        sendCommand(NOP);
    }

    // ---------- charge pump ----------

    template <class Bus>
    inline void Datasheet<Bus>::setChargePump(bool enable) {
        sendCommand(SET_CHARGE_PUMP, enable ? CHARGE_PUMP_ENABLE : CHARGE_PUMP_DISABLE);
    }

    // ---------- 6. advance graphic ----------

    template <class Bus>
    inline void Datasheet<Bus>::setFadeBlink(uint8_t mode, uint8_t interval) {
        sendCommand(SET_FADE_BLINK, (uint8_t)((mode & 0x30) | (interval & 0x0F)));
    }

    template <class Bus>
    inline void Datasheet<Bus>::setZoomIn(bool zoom) {
        sendCommand(SET_ZOOM_IN, zoom ? 0x01 : 0x00);
    }
}

#endif
