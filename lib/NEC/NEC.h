#ifndef NEC_H
#define NEC_H

// the NEC infrared remote protocol: receiver, keys, parts
//   receiver.h              Receiver<InterruptPin, ACTIVE_LOW, QUEUE>: decodes NEC in an interrupt,
//                           Code = address + command + repeats
//   key.h                   Key: KEY_0 ~ KEY_9, arrows, OK, *, # (the same for every remote)
//   module/HX1838.h         HX1838, VS1838B: the receiver (KY-022 ...)
//   module/remote_17_key.h  Remote17KeyNumbersTop, Remote17KeyArrowsTop: the 17-key kit remotes, command -> Key
#include "receiver.h"
#include "key.h"
#include "module/HX1838.h"
#include "module/remote_17_key.h"

#endif
