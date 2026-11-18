#ifndef NEC_H
#define NEC_H

// the NEC infrared remote protocol, both ways: receiver, transmitter, keys, parts
//   protocol.h              the timings (shared by both ways)
//   receiver.h              Receiver<InterruptPin, ACTIVE_LOW, QUEUE>: decodes NEC in an interrupt,
//                           Code = address + command + repeats
//   transmitter.h           Transmitter<Carrier>: sends codes with an infrared LED (acts as a remote)
//   key.h                   Key: KEY_0 ~ KEY_9, arrows, OK, *, # (the same for every remote)
//   module/HX1838.h         HX1838, VS1838B: the receiver (KY-022 ...)
//   module/IR_LED.h         IRLED<Carrier>: a bare infrared LED (+ resistor), to send
//   module/KY_005.h         KY005<Carrier>: the infrared LED board
//   module/remote_17_key.h  Remote17KeyNumbersTop, Remote17KeyArrowsTop: the 17-key kit remotes, command <-> Key
#include "protocol.h"
#include "receiver.h"
#include "transmitter.h"
#include "key.h"
#include "module/HX1838.h"
#include "module/IR_LED.h"
#include "module/KY_005.h"
#include "module/remote_17_key.h"

#endif
