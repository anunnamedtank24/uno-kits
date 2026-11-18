#ifndef ANALOG_JOYSTICK_H
#define ANALOG_JOYSTICK_H

#include <stdint.h>

namespace analog {
    // where the stick points (direction())
    enum Direction : uint8_t { CENTER, UP, DOWN, LEFT, RIGHT };

    // a thumb joystick: two potentiometers (X, Y) and a push button (press the stick), e.g. KY-023
    //
    // XPin, YPin: structs as for Input (lib/Uno/Uno_analog_pin.h, UnoAnalogPin)
    // ButtonPin:  a digital input with the pull-up (Uno::UnoInputPin<PIN, true>: the button closes to GND)
    // INVERT_X, INVERT_Y: the stick's right / up give the lower reading (it depends on how it is mounted)
    //
    // x(), y(): -100 ~ 100 from the centre it had at init() (the stick must be let go then);
    // a dead zone around the centre gives 0 (the stick never comes back exactly)
    template <class XPin, class YPin, class ButtonPin, bool INVERT_X = false, bool INVERT_Y = false>
    class Joystick {
    public:
        // reads the centre (let the stick go)
        inline void init();

        // reads both axes and the button (~ 230 µs); true if the direction or the button changed
        inline bool update();

        // -100 (left / down) ~ 100 (right / up), 0 inside the dead zone
        inline int8_t x() const;
        inline int8_t y() const;

        // the stronger axis, when it is at least 50 % out (for menus); CENTER otherwise
        inline Direction direction() const;

        // true once when the stick is pushed out to a direction (cleared by the call): one menu step
        inline bool wasMoved(Direction &direction);

        // the button (debounced)
        inline bool isPressed() const;
        inline bool wasPressed();

        // the dead zone, % of the way out (default 8)
        inline void setDeadZone(uint8_t percent);

        // reads the centre again
        inline void recenter();

    private:
        static constexpr uint8_t DEBOUNCE_MS = 20;
        static constexpr int8_t DIRECTION_ON = 50;
        static constexpr int8_t DIRECTION_OFF = 30;

        uint16_t _centerX = 512;
        uint16_t _centerY = 512;
        int8_t _x = 0;
        int8_t _y = 0;
        uint8_t _deadZone = 8;
        Direction _direction = CENTER;
        bool _moved = false;
        Direction _movedDirection = CENTER;
        bool _pressed = false;
        bool _pressedEvent = false;
        bool _lastRaw = false;
        uint32_t _rawSinceMs = 0;

        inline int8_t axis(uint16_t reading, uint16_t center, uint16_t max, bool invert) const;
    };

    // ================= definitions =================

    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline void Joystick<XPin, YPin, ButtonPin, IX, IY>::init() {
        XPin::init();
        YPin::init();
        ButtonPin::init();
        recenter();
        _lastRaw = !ButtonPin::read();
        _pressed = _lastRaw;
        _rawSinceMs = XPin::nowMs();
    }

    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline void Joystick<XPin, YPin, ButtonPin, IX, IY>::recenter() {
        _centerX = XPin::readAnalog();
        _centerY = YPin::readAnalog();
        _x = 0;
        _y = 0;
        _direction = CENTER;
    }

    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline bool Joystick<XPin, YPin, ButtonPin, IX, IY>::update() {
        _x = axis(XPin::readAnalog(), _centerX, XPin::MAX, IX);
        _y = axis(YPin::readAnalog(), _centerY, YPin::MAX, IY);

        // direction with hysteresis: out at 50 %, back at 30 %
        // (from one direction straight to another: as soon as the other one is out 50 %)
        Direction next = _direction;
        int8_t ax = _x < 0 ? (int8_t)-_x : _x;
        int8_t ay = _y < 0 ? (int8_t)-_y : _y;
        if (ax >= DIRECTION_ON || ay >= DIRECTION_ON) {
            Direction strongest = ax >= ay ? (_x > 0 ? RIGHT : LEFT) : (_y > 0 ? UP : DOWN);
            if (strongest != _direction) {
                next = strongest;
            }
        } else if (_direction != CENTER && ax < DIRECTION_OFF && ay < DIRECTION_OFF) {
            next = CENTER;
        }
        bool changed = next != _direction;
        if (changed && next != CENTER) {
            _moved = true;
            _movedDirection = next;
        }
        _direction = next;

        // the button: LOW = pressed, debounced
        bool raw = !ButtonPin::read();
        uint32_t now = XPin::nowMs();
        if (raw != _lastRaw) {
            _lastRaw = raw;
            _rawSinceMs = now;
        } else if (raw != _pressed && now - _rawSinceMs >= DEBOUNCE_MS) {
            _pressed = raw;
            if (_pressed) {
                _pressedEvent = true;
            }
            changed = true;
        }
        return changed;
    }

    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline int8_t Joystick<XPin, YPin, ButtonPin, IX, IY>::x() const {
        return _x;
    }

    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline int8_t Joystick<XPin, YPin, ButtonPin, IX, IY>::y() const {
        return _y;
    }

    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline Direction Joystick<XPin, YPin, ButtonPin, IX, IY>::direction() const {
        return _direction;
    }

    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline bool Joystick<XPin, YPin, ButtonPin, IX, IY>::wasMoved(Direction &direction) {
        if (!_moved) {
            return false;
        }
        _moved = false;
        direction = _movedDirection;
        return true;
    }

    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline bool Joystick<XPin, YPin, ButtonPin, IX, IY>::isPressed() const {
        return _pressed;
    }

    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline bool Joystick<XPin, YPin, ButtonPin, IX, IY>::wasPressed() {
        bool pressed = _pressedEvent;
        _pressedEvent = false;
        return pressed;
    }

    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline void Joystick<XPin, YPin, ButtonPin, IX, IY>::setDeadZone(uint8_t percent) {
        _deadZone = percent > 90 ? 90 : percent;
    }

    // -100 ~ 100 from the centre, each side scaled to its own length; 0 inside the dead zone
    template <class XPin, class YPin, class ButtonPin, bool IX, bool IY>
    inline int8_t Joystick<XPin, YPin, ButtonPin, IX, IY>::axis(uint16_t reading, uint16_t center, uint16_t max,
                                                             bool invert) const {
        int32_t value;
        if (reading >= center) {
            uint16_t side = (uint16_t)(max - center);
            value = side == 0 ? 0 : (int32_t)(reading - center) * 100 / side;
        } else {
            value = center == 0 ? 0 : -(int32_t)(center - reading) * 100 / center;
        }
        if (invert) {
            value = -value;
        }
        int32_t size = value < 0 ? -value : value;
        if (size <= _deadZone) {
            return 0;
        }
        // the rest of the way, stretched to 0 ~ 100 again
        int32_t stretched = (size - _deadZone) * 100 / (100 - _deadZone);
        if (stretched > 100) {
            stretched = 100;
        }
        return (int8_t)(value < 0 ? -stretched : stretched);
    }
}

#endif
