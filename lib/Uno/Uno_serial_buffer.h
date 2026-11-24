#ifndef UNO_SERIAL_BUFFER_H
#define UNO_SERIAL_BUFFER_H

#ifdef UNO_SERIAL_H
#error "Uno_serial_buffer.h: include it before Uno_serial.h, or instead of it"
#endif

#include "Uno_serial.h"
#include <avr/interrupt.h>

// Uno::UnoSerial<BAUD, RX_SIZE, TX_SIZE> with buffers: the UART interrupts that fill and empty them
// (Uno_serial.h without this file has none, so a program without buffers pays nothing for them)
//   USART_RX    each received byte into the buffer (a broken one dropped, a full buffer counted)
//   USART_UDRE  the next byte of the send buffer out; stops itself when the buffer is empty
// the interrupts are defined in this header: include it in one .cpp file only
// Arduino Serial must not be used at the same time (it defines the same interrupts)

ISR(USART_RX_vect) {
    Uno::serial_state::receive();
}

ISR(USART_UDRE_vect) {
    Uno::serial_state::sendNext();
}

#endif
