#ifndef AUDIO_H
#define AUDIO_H

// sounds and their players, entry point: include this file only
//   note.h                   audio::noteHz, midiHz, Pitch, REST, beatMs          notes by hand
//   format.h                 audio::KIND_*, encodings, TARGET_*, Adpcm           the exchange formats
//   player.h                 audio::Player<Sound>                                 tone / beep / slide / notes / rhythm (flash)
//   stream_player.h          audio::StreamPlayer<Sound, Stream>                   notes / rhythm from the computer
//   sample_player.h          audio::SamplePlayer<SampleOut>                       PCM samples (flash)
//   stream_sample_player.h   audio::StreamSamplePlayer<SampleOut, Stream>         PCM samples from the computer
//   output/selected.h        melody::*                                            sounds made from audio files
//   asset.h            MELODY("any/path", "options"): a file converted before the build into output/, like convert.py
// a Sound makes a pitch (the modules of lib/buzzer), a SampleOut plays samples (Uno::UnoPcmOut),
// a Stream is the link to the computer (lib/stream: stream::Puller<Uno::UnoSerial<BAUD>, CHUNK>)
//
// tools (lib/audio/source):
//   convert.py  .wav / .flac / .mp3 / .m4a / .ogg ... -> output/*.h (--sound active / passive / pcm, --target flash / stream)
//   check.py    checks output/selected.h
//   stream_handler.py  what lib/stream/source/stream.py does for sounds ('A')

#include "note.h"
#include "format.h"
#include "player.h"
#include "stream_player.h"
#include "sample_player.h"
#include "stream_sample_player.h"
#include "output/selected.h"
#include "asset.h"

#endif
