#ifndef ST7789_DATASHEET_H
#define ST7789_DATASHEET_H

#include <stdint.h>

// reads need a Bus with a read line (SDO, or SDA switched to input);
// a write-only Bus leaves the read functions empty and read8() returns 0
// reference: Sitronix ST7789V datasheet v1.3, chapter 9
//
// Bus: a struct providing these static functions (see lib/Uno/Uno_ST7789.h)
//   void init(uint8_t blk)        pins, SPI, backlight PWM
//   void setBacklight(uint8_t blk)
//   void delayMs(uint16_t ms)
//   uint8_t readFlash8(const uint8_t *)     PROGMEM read (used by graphics::Text)
//   uint16_t readFlash16(const uint16_t *)  PROGMEM read (used by graphics::Text)
//   uint32_t nowMs()                        ms since power on (used by video players)
//   void cmdMode()                DC low
//   void dataMode()               DC high
//   void select()                 CS low  (empty if no CS)
//   void deselect()               CS high (empty if no CS)
//   bool HAS_RES                  false: no reset pin, hwReset() does nothing
//   void rstHigh()                         (empty if no RES)
//   void rstLow()                          (empty if no RES)
//   void write8(uint8_t)          MSB first
//   void write16(uint16_t)        high byte first
//                                 may return before the byte is sent, then cmdMode / dataMode / deselect
//                                 must wait for it (DC / CS never change during a byte)
//   void readMode()               data line -> input  (empty if write-only)
//   void writeMode()              data line -> output (empty if write-only)
//   void readDummy()              serial: 1 dummy clock, parallel: 1 dummy read
//   uint8_t read8()               blocking, MSB first (return 0 if write-only)
namespace ST7789 {
    // ===== Table 1: system function commands =====
    constexpr uint8_t NOP      = 0x00;
    constexpr uint8_t SWRESET  = 0x01; // delay 120 ms
    constexpr uint8_t RDDID    = 0x04;
    constexpr uint8_t RDDST    = 0x09;
    constexpr uint8_t RDDPM    = 0x0A;
    constexpr uint8_t RDDMADCTL = 0x0B;
    constexpr uint8_t RDDCOLMOD = 0x0C;
    constexpr uint8_t RDDIM    = 0x0D;
    constexpr uint8_t RDDSM    = 0x0E;
    constexpr uint8_t RDDSDR   = 0x0F;
    constexpr uint8_t SLPIN    = 0x10; // delay 5 ms
    constexpr uint8_t SLPOUT   = 0x11; // delay 120 ms
    constexpr uint8_t PTLON    = 0x12;
    constexpr uint8_t NORON    = 0x13;
    constexpr uint8_t INVOFF   = 0x20;
    constexpr uint8_t INVON    = 0x21;
    constexpr uint8_t GAMSET   = 0x26;
    constexpr uint8_t DISPOFF  = 0x28;
    constexpr uint8_t DISPON   = 0x29;
    constexpr uint8_t CASET    = 0x2A;
    constexpr uint8_t RASET    = 0x2B;
    constexpr uint8_t RAMWR    = 0x2C;
    constexpr uint8_t RAMRD    = 0x2E;
    constexpr uint8_t PTLAR    = 0x30;
    constexpr uint8_t VSCRDEF  = 0x33;
    constexpr uint8_t TEOFF    = 0x34;
    constexpr uint8_t TEON     = 0x35;
    constexpr uint8_t MADCTL   = 0x36;
    constexpr uint8_t VSCSAD   = 0x37;
    constexpr uint8_t IDMOFF   = 0x38;
    constexpr uint8_t IDMON    = 0x39;
    constexpr uint8_t COLMOD   = 0x3A;
    constexpr uint8_t RAMWRC   = 0x3C; // a.k.a. WRMEMC
    constexpr uint8_t RDMEMC   = 0x3E;
    constexpr uint8_t STE      = 0x44;
    constexpr uint8_t GSCAN    = 0x45;
    constexpr uint8_t WRDISBV  = 0x51;
    constexpr uint8_t RDDISBV  = 0x52;
    constexpr uint8_t WRCTRLD  = 0x53;
    constexpr uint8_t RDCTRLD  = 0x54;
    constexpr uint8_t WRCACE   = 0x55;
    constexpr uint8_t RDCABC   = 0x56;
    constexpr uint8_t WRCABCMB = 0x5E;
    constexpr uint8_t RDCABCMB = 0x5F;
    constexpr uint8_t RDABCSDR = 0x68;
    constexpr uint8_t RDID1    = 0xDA;
    constexpr uint8_t RDID2    = 0xDB;
    constexpr uint8_t RDID3    = 0xDC;

    // ===== Table 2: panel function commands =====
    constexpr uint8_t RAMCTRL    = 0xB0;
    constexpr uint8_t RGBCTRL    = 0xB1;
    constexpr uint8_t PORCTRL    = 0xB2;
    constexpr uint8_t FRCTRL1    = 0xB3;
    constexpr uint8_t PARCTRL    = 0xB5;
    constexpr uint8_t GCTRL      = 0xB7;
    constexpr uint8_t GTADJ      = 0xB8;
    constexpr uint8_t DGMEN      = 0xBA;
    constexpr uint8_t VCOMS      = 0xBB;
    constexpr uint8_t LCMCTRL    = 0xC0;
    constexpr uint8_t IDSET      = 0xC1;
    constexpr uint8_t VDVVRHEN   = 0xC2;
    constexpr uint8_t VRHS       = 0xC3;
    constexpr uint8_t VDVS       = 0xC4;
    constexpr uint8_t VCMOFSET   = 0xC5;
    constexpr uint8_t FRCTRL2    = 0xC6;
    constexpr uint8_t CABCCTRL   = 0xC7;
    constexpr uint8_t REGSEL1    = 0xC8;
    constexpr uint8_t REGSEL2    = 0xCA;
    constexpr uint8_t PWMFRSEL   = 0xCC;
    constexpr uint8_t PWCTRL1    = 0xD0;
    constexpr uint8_t VAPVANEN   = 0xD2;
    constexpr uint8_t CMD2EN     = 0xDF;
    constexpr uint8_t PVGAMCTRL  = 0xE0;
    constexpr uint8_t NVGAMCTRL  = 0xE1;
    constexpr uint8_t DGMLUTR    = 0xE2;
    constexpr uint8_t DGMLUTB    = 0xE3;
    constexpr uint8_t GATECTRL   = 0xE4;
    constexpr uint8_t SPI2EN     = 0xE7;
    constexpr uint8_t PWCTRL2    = 0xE8;
    constexpr uint8_t EQCTRL     = 0xE9;
    constexpr uint8_t PROMCTRL   = 0xEC;
    constexpr uint8_t PROMEN     = 0xFA;
    constexpr uint8_t NVMSET     = 0xFC;
    constexpr uint8_t PROMACT    = 0xFE;

    // MADCTL bits
    constexpr uint8_t MADCTL_MY  = 0x80; // row address order
    constexpr uint8_t MADCTL_MX  = 0x40; // column address order
    constexpr uint8_t MADCTL_MV  = 0x20; // row/column exchange
    constexpr uint8_t MADCTL_ML  = 0x10; // vertical refresh order
    constexpr uint8_t MADCTL_BGR = 0x08; // 0: RGB, 1: BGR
    constexpr uint8_t MADCTL_MH  = 0x04; // horizontal refresh order

    // COLMOD values
    constexpr uint8_t COLMOD_12BIT = 0x53; // RGB444
    constexpr uint8_t COLMOD_16BIT = 0x55; // RGB565
    constexpr uint8_t COLMOD_18BIT = 0x66; // RGB666

    // GAMSET values (predefined gamma curves)
    constexpr uint8_t GAMSET_2_2 = 0x01; // G2.2
    constexpr uint8_t GAMSET_1_8 = 0x02; // G1.8
    constexpr uint8_t GAMSET_2_5 = 0x04; // G2.5
    constexpr uint8_t GAMSET_1_0 = 0x08; // G1.0

    // WRCTRLD bits
    constexpr uint8_t WRCTRLD_BCTRL = 0x20; // brightness control on
    constexpr uint8_t WRCTRLD_DD    = 0x08; // display dimming on
    constexpr uint8_t WRCTRLD_BL    = 0x04; // backlight control on

    // WRCACE values: CABC mode | color enhancement
    constexpr uint8_t WRCACE_CABC_OFF    = 0x00;
    constexpr uint8_t WRCACE_CABC_UI     = 0x01;
    constexpr uint8_t WRCACE_CABC_STILL  = 0x02;
    constexpr uint8_t WRCACE_CABC_MOVING = 0x03;
    constexpr uint8_t WRCACE_CE_LOW      = 0x00;
    constexpr uint8_t WRCACE_CE_MEDIUM   = 0x10;
    constexpr uint8_t WRCACE_CE_HIGH     = 0x30;
    constexpr uint8_t WRCACE_CE_ON       = 0x80;

    template <class Bus>
    class Datasheet {
    public:
        // the Bus this screen runs on, for layers above (e.g. graphics::Text reads flash with it)
        using BusType = Bus;

        inline Datasheet(uint16_t width = 240, uint16_t height = 240);

        // call before drawing
        inline void setOffset(uint16_t xOffset, uint16_t yOffset);

        inline uint16_t width() const;
        inline uint16_t height() const;
        inline uint16_t xOffset() const;
        inline uint16_t yOffset() const;
        inline void setBacklight(uint8_t blk);
        inline void hwReset();

        // ---------- low level ----------

        inline void startWrite();
        inline void endWrite();
        inline void writeCommand(uint8_t cmd);
        inline void writeData8(uint8_t data);
        inline void writeData16(uint16_t data);
        inline uint8_t readData8();

        // ends a read started by ramRd() / rdMemC()
        inline void endRead();

        // ---------- Table 1: system function commands (in command order) ----------

        inline void nop();
        inline void swReset();

        // ID1 << 16 | ID2 << 8 | ID3
        inline uint32_t rddid();

        // 32-bit display status
        inline uint32_t rddst();

        // BSTON | IDMON | PTLON | SLPOUT | NORON | DISON
        inline uint8_t rddpm();

        // MY | MX | MV | ML | RGB | MH
        inline uint8_t rddMadctl();

        inline uint8_t rddColmod();

        // VSSON | INVON | GC[2:0]
        inline uint8_t rddim();

        // TEON | TEM
        inline uint8_t rddsm();

        // self-diagnostic result
        inline uint8_t rddsdr();

        inline void slpIn();
        inline void slpOut();
        inline void ptlOn();
        inline void norOn();
        inline void invOff();
        inline void invOn();

        // gc: GAMSET_2_2 / GAMSET_1_8 / GAMSET_2_5 / GAMSET_1_0
        inline void gamSet(uint8_t gc);

        inline void dispOff();
        inline void dispOn();
        inline void caset(uint16_t xs, uint16_t xe);
        inline void raset(uint16_t ys, uint16_t ye);

        // starts a write transaction: stream data with writeData8/16(), then endWrite()
        inline void ramWr();

        // starts a read transaction: read pixels with readData8(), then endRead()
        inline void ramRd();

        inline void ptlar(uint16_t ps, uint16_t pe);

        // tfa + vsa + bfa = 320
        inline void vscrdef(uint16_t tfa, uint16_t vsa, uint16_t bfa);

        inline void teOff();

        // mode: 0 = V-blank only, 1 = V-blank + H-blank
        inline void teOn(uint8_t mode = 0);

        inline void madctl(uint8_t value);
        inline void vscsad(uint16_t vsp);
        inline void idmOff();
        inline void idmOn();
        inline void colmod(uint8_t value);

        // starts a write transaction: stream data with writeData8/16(), then endWrite()
        inline void ramWrC();

        // starts a read transaction: read pixels with readData8(), then endRead()
        inline void rdMemC();

        // tear effect output at scanline n
        inline void ste(uint16_t n);

        // current scanline
        inline uint16_t gscan();

        // display brightness (needs WRCTRLD_BCTRL)
        inline void wrDisBv(uint8_t brightness);

        inline uint8_t rdDisBv();

        // value: WRCTRLD_BCTRL | WRCTRLD_DD | WRCTRLD_BL
        inline void wrCtrlD(uint8_t value);

        inline uint8_t rdCtrlD();

        // value: WRCACE_CABC_xxx | WRCACE_CE_xxx | WRCACE_CE_ON
        inline void wrCace(uint8_t value);

        inline uint8_t rdCabc();

        // CABC minimum brightness
        inline void wrCabcMb(uint8_t value);

        inline uint8_t rdCabcMb();
        inline uint8_t rdAbcSdr();
        inline uint8_t rdId1();
        inline uint8_t rdId2();
        inline uint8_t rdId3();

        // ---------- Table 2: panel function commands (in command order) ----------
        // values differ between panels: use the init table from your module vendor

        inline void ramCtrl(uint8_t p1, uint8_t p2);
        inline void rgbCtrl(uint8_t p1, uint8_t p2, uint8_t p3);

        // porch: e.g. 0x0C, 0x0C, 0x00, 0x33, 0x33
        inline void porCtrl(uint8_t bpa, uint8_t fpa, uint8_t psen, uint8_t bpbFpb, uint8_t bpcFpc);

        inline void frCtrl1(uint8_t p1, uint8_t p2, uint8_t p3);
        inline void parCtrl(uint8_t value);

        // VGH / VGL: e.g. 0x35
        inline void gCtrl(uint8_t value);

        // gta: gate on timing, gofrGof: GOFR[7:4] | GOF[3:0]
        inline void gtAdj(uint8_t gta, uint8_t gofrGof);

        // enable digital gamma (DGMLUTR / DGMLUTB)
        inline void dgmEn(bool en);

        // VCOM: e.g. 0x19 ~ 0x35
        inline void vcomS(uint8_t value);

        // e.g. 0x2C
        inline void lcmCtrl(uint8_t value);

        inline void idSet(uint8_t id1, uint8_t id2, uint8_t id3);

        // en: VDV/VRH from command (true) or NVM (false)
        inline void vdvVrhEn(bool en);

        // e.g. 0x12
        inline void vrhS(uint8_t value);

        // e.g. 0x20
        inline void vdvS(uint8_t value);

        inline void vcmOfSet(uint8_t value);

        // frame rate in normal mode: 0x0F = 60 Hz
        inline void frCtrl2(uint8_t value);

        inline void cabcCtrl(uint8_t value);
        inline void regSel1();
        inline void regSel2();
        inline void pwmFrSel(uint8_t value);

        // AVDD[7:6] | AVCL[5:4] | VDS[1:0]: e.g. 0xA1
        inline void pwCtrl1(uint8_t value);

        inline void vapVanEn();
        inline void cmd2En(bool en);

        // positive voltage gamma, 14 bytes
        inline void pvGamCtrl(const uint8_t (&values)[14]);

        // negative voltage gamma, 14 bytes
        inline void nvGamCtrl(const uint8_t (&values)[14]);

        // digital gamma look-up table for red, 64 bytes (needs dgmEn)
        inline void dgmLutR(const uint8_t (&values)[64]);

        // digital gamma look-up table for blue, 64 bytes (needs dgmEn)
        inline void dgmLutB(const uint8_t (&values)[64]);

        // nl: gate line count, scn: start line, smGs: SM(bit2) | GS(bit0)
        inline void gateCtrl(uint8_t nl, uint8_t scn, uint8_t smGs);

        // SPI2EN(bit4) | SPIRD(bit0)
        inline void spi2En(uint8_t value);

        // SBCLK[5:4] | STP14CK[1:0]
        inline void pwCtrl2(uint8_t value);

        inline void eqCtrl(uint8_t seq, uint8_t spret, uint8_t geq);

        // ---------- NVM (one-time programmable) ----------
        // WARNING: writes to OTP memory are PERMANENT. Do not use unless you know why.

        inline void promCtrl();
        inline void promEn(bool en);
        inline void nvmSet(uint8_t address, uint8_t value);
        inline void promAct();

    private:
        uint16_t _width;
        uint16_t _height;
        uint16_t _xOffset;
        uint16_t _yOffset;

        inline void sendCmd(uint8_t cmd);
        inline void sendCmd(uint8_t cmd, uint8_t data);
        inline void sendCmd(uint8_t cmd, const uint8_t *data, uint8_t length);

        // dummy: 24/32-bit and multi-byte reads need a dummy cycle first
        inline void startRead(uint8_t cmd, bool dummy);

        // reads 1 ~ 4 bytes, first byte is the most significant
        inline uint32_t readCmd(uint8_t cmd, uint8_t length, bool dummy);
    };

    // ================= definitions =================

    template <class Bus>
    inline Datasheet<Bus>::Datasheet(uint16_t width, uint16_t height)
        : _width(width), _height(height),
          _xOffset(0), _yOffset(0) {
    }

    template <class Bus>
    inline void Datasheet<Bus>::setOffset(uint16_t xOffset, uint16_t yOffset) {
        _xOffset = xOffset;
        _yOffset = yOffset;
    }

    template <class Bus>
    inline uint16_t Datasheet<Bus>::width() const {
        return _width;
    }

    template <class Bus>
    inline uint16_t Datasheet<Bus>::height() const {
        return _height;
    }

    template <class Bus>
    inline uint16_t Datasheet<Bus>::xOffset() const {
        return _xOffset;
    }

    template <class Bus>
    inline uint16_t Datasheet<Bus>::yOffset() const {
        return _yOffset;
    }

    template <class Bus>
    inline void Datasheet<Bus>::setBacklight(uint8_t blk) {
        Bus::setBacklight(blk);
    }

    template <class Bus>
    inline void Datasheet<Bus>::hwReset() {
        if (!Bus::HAS_RES) {
            return;
        }
        Bus::rstHigh();
        Bus::delayMs(10);
        Bus::rstLow();
        Bus::delayMs(20);
        Bus::rstHigh();
        Bus::delayMs(120);
    }

    // ---------- low level ----------

    template <class Bus>
    inline void Datasheet<Bus>::startWrite() {
        Bus::select();
    }

    template <class Bus>
    inline void Datasheet<Bus>::endWrite() {
        Bus::deselect();
    }

    template <class Bus>
    inline void Datasheet<Bus>::writeCommand(uint8_t cmd) {
        Bus::cmdMode();
        Bus::write8(cmd);
        Bus::dataMode();
    }

    template <class Bus>
    inline void Datasheet<Bus>::writeData8(uint8_t data) {
        Bus::write8(data);
    }

    template <class Bus>
    inline void Datasheet<Bus>::writeData16(uint16_t data) {
        Bus::write16(data);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::readData8() {
        return Bus::read8();
    }

    template <class Bus>
    inline void Datasheet<Bus>::endRead() {
        Bus::writeMode();
        endWrite();
    }

    // ---------- Table 1: system function commands (in command order) ----------

    template <class Bus>
    inline void Datasheet<Bus>::nop() {
        sendCmd(NOP);
    }

    template <class Bus>
    inline void Datasheet<Bus>::swReset() {
        sendCmd(SWRESET);
        Bus::delayMs(120);
    }

    template <class Bus>
    inline uint32_t Datasheet<Bus>::rddid() {
        return readCmd(RDDID, 3, true);
    }

    template <class Bus>
    inline uint32_t Datasheet<Bus>::rddst() {
        return readCmd(RDDST, 4, true);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rddpm() {
        return readCmd(RDDPM, 1, false);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rddMadctl() {
        return readCmd(RDDMADCTL, 1, false);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rddColmod() {
        return readCmd(RDDCOLMOD, 1, false);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rddim() {
        return readCmd(RDDIM, 1, false);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rddsm() {
        return readCmd(RDDSM, 1, false);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rddsdr() {
        return readCmd(RDDSDR, 1, false);
    }

    template <class Bus>
    inline void Datasheet<Bus>::slpIn() {
        sendCmd(SLPIN);
        Bus::delayMs(5);
    }

    template <class Bus>
    inline void Datasheet<Bus>::slpOut() {
        sendCmd(SLPOUT);
        Bus::delayMs(120);
    }

    template <class Bus>
    inline void Datasheet<Bus>::ptlOn() {
        sendCmd(PTLON);
    }

    template <class Bus>
    inline void Datasheet<Bus>::norOn() {
        sendCmd(NORON);
    }

    template <class Bus>
    inline void Datasheet<Bus>::invOff() {
        sendCmd(INVOFF);
    }

    template <class Bus>
    inline void Datasheet<Bus>::invOn() {
        sendCmd(INVON);
    }

    template <class Bus>
    inline void Datasheet<Bus>::gamSet(uint8_t gc) {
        sendCmd(GAMSET, gc);
    }

    template <class Bus>
    inline void Datasheet<Bus>::dispOff() {
        sendCmd(DISPOFF);
    }

    template <class Bus>
    inline void Datasheet<Bus>::dispOn() {
        sendCmd(DISPON);
    }

    template <class Bus>
    inline void Datasheet<Bus>::caset(uint16_t xs, uint16_t xe) {
        startWrite();
        writeCommand(CASET);
        writeData16(xs);
        writeData16(xe);
        endWrite();
    }

    template <class Bus>
    inline void Datasheet<Bus>::raset(uint16_t ys, uint16_t ye) {
        startWrite();
        writeCommand(RASET);
        writeData16(ys);
        writeData16(ye);
        endWrite();
    }

    template <class Bus>
    inline void Datasheet<Bus>::ramWr() {
        startWrite();
        writeCommand(RAMWR);
    }

    template <class Bus>
    inline void Datasheet<Bus>::ramRd() {
        startRead(RAMRD, true);
    }

    template <class Bus>
    inline void Datasheet<Bus>::ptlar(uint16_t ps, uint16_t pe) {
        startWrite();
        writeCommand(PTLAR);
        writeData16(ps);
        writeData16(pe);
        endWrite();
    }

    template <class Bus>
    inline void Datasheet<Bus>::vscrdef(uint16_t tfa, uint16_t vsa, uint16_t bfa) {
        startWrite();
        writeCommand(VSCRDEF);
        writeData16(tfa);
        writeData16(vsa);
        writeData16(bfa);
        endWrite();
    }

    template <class Bus>
    inline void Datasheet<Bus>::teOff() {
        sendCmd(TEOFF);
    }

    template <class Bus>
    inline void Datasheet<Bus>::teOn(uint8_t mode) {
        sendCmd(TEON, mode & 0x01);
    }

    template <class Bus>
    inline void Datasheet<Bus>::madctl(uint8_t value) {
        sendCmd(MADCTL, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::vscsad(uint16_t vsp) {
        startWrite();
        writeCommand(VSCSAD);
        writeData16(vsp);
        endWrite();
    }

    template <class Bus>
    inline void Datasheet<Bus>::idmOff() {
        sendCmd(IDMOFF);
    }

    template <class Bus>
    inline void Datasheet<Bus>::idmOn() {
        sendCmd(IDMON);
    }

    template <class Bus>
    inline void Datasheet<Bus>::colmod(uint8_t value) {
        sendCmd(COLMOD, value);
        Bus::delayMs(10);
    }

    template <class Bus>
    inline void Datasheet<Bus>::ramWrC() {
        startWrite();
        writeCommand(RAMWRC);
    }

    template <class Bus>
    inline void Datasheet<Bus>::rdMemC() {
        startRead(RDMEMC, true);
    }

    template <class Bus>
    inline void Datasheet<Bus>::ste(uint16_t n) {
        startWrite();
        writeCommand(STE);
        writeData16(n);
        endWrite();
    }

    template <class Bus>
    inline uint16_t Datasheet<Bus>::gscan() {
        return readCmd(GSCAN, 2, true);
    }

    template <class Bus>
    inline void Datasheet<Bus>::wrDisBv(uint8_t brightness) {
        sendCmd(WRDISBV, brightness);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rdDisBv() {
        return readCmd(RDDISBV, 1, false);
    }

    template <class Bus>
    inline void Datasheet<Bus>::wrCtrlD(uint8_t value) {
        sendCmd(WRCTRLD, value);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rdCtrlD() {
        return readCmd(RDCTRLD, 1, false);
    }

    template <class Bus>
    inline void Datasheet<Bus>::wrCace(uint8_t value) {
        sendCmd(WRCACE, value);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rdCabc() {
        return readCmd(RDCABC, 1, false);
    }

    template <class Bus>
    inline void Datasheet<Bus>::wrCabcMb(uint8_t value) {
        sendCmd(WRCABCMB, value);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rdCabcMb() {
        return readCmd(RDCABCMB, 1, false);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rdAbcSdr() {
        return readCmd(RDABCSDR, 1, false);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rdId1() {
        return readCmd(RDID1, 1, false);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rdId2() {
        return readCmd(RDID2, 1, false);
    }

    template <class Bus>
    inline uint8_t Datasheet<Bus>::rdId3() {
        return readCmd(RDID3, 1, false);
    }

    // ---------- Table 2: panel function commands (in command order) ----------

    template <class Bus>
    inline void Datasheet<Bus>::ramCtrl(uint8_t p1, uint8_t p2) {
        const uint8_t data[] = {p1, p2};
        sendCmd(RAMCTRL, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::rgbCtrl(uint8_t p1, uint8_t p2, uint8_t p3) {
        const uint8_t data[] = {p1, p2, p3};
        sendCmd(RGBCTRL, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::porCtrl(uint8_t bpa, uint8_t fpa, uint8_t psen, uint8_t bpbFpb, uint8_t bpcFpc) {
        const uint8_t data[] = {bpa, fpa, psen, bpbFpb, bpcFpc};
        sendCmd(PORCTRL, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::frCtrl1(uint8_t p1, uint8_t p2, uint8_t p3) {
        const uint8_t data[] = {p1, p2, p3};
        sendCmd(FRCTRL1, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::parCtrl(uint8_t value) {
        sendCmd(PARCTRL, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::gCtrl(uint8_t value) {
        sendCmd(GCTRL, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::gtAdj(uint8_t gta, uint8_t gofrGof) {
        const uint8_t data[] = {0x2A, 0x2B, (uint8_t)(gta & 0x3F), gofrGof};
        sendCmd(GTADJ, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::dgmEn(bool en) {
        sendCmd(DGMEN, en ? 0x04 : 0x00);
    }

    template <class Bus>
    inline void Datasheet<Bus>::vcomS(uint8_t value) {
        sendCmd(VCOMS, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::lcmCtrl(uint8_t value) {
        sendCmd(LCMCTRL, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::idSet(uint8_t id1, uint8_t id2, uint8_t id3) {
        const uint8_t data[] = {id1, id2, id3};
        sendCmd(IDSET, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::vdvVrhEn(bool en) {
        const uint8_t data[] = {(uint8_t)(en ? 0x01 : 0x00), 0xFF};
        sendCmd(VDVVRHEN, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::vrhS(uint8_t value) {
        sendCmd(VRHS, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::vdvS(uint8_t value) {
        sendCmd(VDVS, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::vcmOfSet(uint8_t value) {
        sendCmd(VCMOFSET, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::frCtrl2(uint8_t value) {
        sendCmd(FRCTRL2, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::cabcCtrl(uint8_t value) {
        sendCmd(CABCCTRL, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::regSel1() {
        sendCmd(REGSEL1, 0x08);
    }

    template <class Bus>
    inline void Datasheet<Bus>::regSel2() {
        sendCmd(REGSEL2, 0x0F);
    }

    template <class Bus>
    inline void Datasheet<Bus>::pwmFrSel(uint8_t value) {
        sendCmd(PWMFRSEL, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::pwCtrl1(uint8_t value) {
        const uint8_t data[] = {0xA4, value};
        sendCmd(PWCTRL1, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::vapVanEn() {
        sendCmd(VAPVANEN, 0x4C);
    }

    template <class Bus>
    inline void Datasheet<Bus>::cmd2En(bool en) {
        const uint8_t data[] = {0x5A, 0x69, 0x02, (uint8_t)(en ? 0x01 : 0x00)};
        sendCmd(CMD2EN, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::pvGamCtrl(const uint8_t (&values)[14]) {
        sendCmd(PVGAMCTRL, values, 14);
    }

    template <class Bus>
    inline void Datasheet<Bus>::nvGamCtrl(const uint8_t (&values)[14]) {
        sendCmd(NVGAMCTRL, values, 14);
    }

    template <class Bus>
    inline void Datasheet<Bus>::dgmLutR(const uint8_t (&values)[64]) {
        sendCmd(DGMLUTR, values, 64);
    }

    template <class Bus>
    inline void Datasheet<Bus>::dgmLutB(const uint8_t (&values)[64]) {
        sendCmd(DGMLUTB, values, 64);
    }

    template <class Bus>
    inline void Datasheet<Bus>::gateCtrl(uint8_t nl, uint8_t scn, uint8_t smGs) {
        const uint8_t data[] = {(uint8_t)(nl & 0x3F), (uint8_t)(scn & 0x3F), (uint8_t)(0x10 | (smGs & 0x05))};
        sendCmd(GATECTRL, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::spi2En(uint8_t value) {
        sendCmd(SPI2EN, value);
    }

    template <class Bus>
    inline void Datasheet<Bus>::pwCtrl2(uint8_t value) {
        sendCmd(PWCTRL2, (uint8_t)(0x80 | (value & 0x33)));
    }

    template <class Bus>
    inline void Datasheet<Bus>::eqCtrl(uint8_t seq, uint8_t spret, uint8_t geq) {
        const uint8_t data[] = {(uint8_t)(seq & 0x1F), (uint8_t)(spret & 0x1F), (uint8_t)(geq & 0x0F)};
        sendCmd(EQCTRL, data, sizeof(data));
    }

    // ---------- NVM (one-time programmable) ----------

    template <class Bus>
    inline void Datasheet<Bus>::promCtrl() {
        sendCmd(PROMCTRL, 0x01);
    }

    template <class Bus>
    inline void Datasheet<Bus>::promEn(bool en) {
        const uint8_t data[] = {0x5A, 0x69, 0xEE, (uint8_t)(en ? 0x04 : 0x00)};
        sendCmd(PROMEN, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::nvmSet(uint8_t address, uint8_t value) {
        const uint8_t data[] = {address, value};
        sendCmd(NVMSET, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::promAct() {
        const uint8_t data[] = {0x19, 0xA5};
        sendCmd(PROMACT, data, sizeof(data));
    }

    template <class Bus>
    inline void Datasheet<Bus>::sendCmd(uint8_t cmd) {
        startWrite();
        writeCommand(cmd);
        endWrite();
    }

    template <class Bus>
    inline void Datasheet<Bus>::sendCmd(uint8_t cmd, uint8_t data) {
        startWrite();
        writeCommand(cmd);
        writeData8(data);
        endWrite();
    }

    template <class Bus>
    inline void Datasheet<Bus>::sendCmd(uint8_t cmd, const uint8_t *data, uint8_t length) {
        startWrite();
        writeCommand(cmd);
        for (uint8_t i = 0; i < length; i++) {
            writeData8(data[i]);
        }
        endWrite();
    }

    template <class Bus>
    inline void Datasheet<Bus>::startRead(uint8_t cmd, bool dummy) {
        startWrite();
        writeCommand(cmd);
        Bus::readMode();
        if (dummy) {
            Bus::readDummy();
        }
    }

    template <class Bus>
    inline uint32_t Datasheet<Bus>::readCmd(uint8_t cmd, uint8_t length, bool dummy) {
        startRead(cmd, dummy);
        uint32_t value = 0;
        for (uint8_t i = 0; i < length; i++) {
            value = (value << 8) | readData8();
        }
        endRead();
        return value;
    }
}

#endif
