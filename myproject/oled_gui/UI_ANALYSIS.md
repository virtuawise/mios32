# MBSEQ V4+ pages translated to four screens of blocks (proposal)

Source: `apps/sequencers/midibox_seq_v4/core/seq_ui_*.c` (each page documents its LCD layout at the top).
Starting point: 4 screens x 4 columns = 16 columns; under every column one encoder (GP1-16) and one button. A page can therefore show 16 parameters (4 groups of 4, one group per screen).

## Block types
value (label small, value large, bar) · choice list (current option large) · switch (filled = on) · picker over 2-3 columns (grid of options, selected one highlighted) · graph (groove curve, LFO shape, euclid ring, morph, loop range).

## Page inventory (from the firmware)
| Page | Parameters |
|---|---|
| TRKEVNT (Event) | track (GxTy), layer configuration (event type + steps + par/trg layers, applied with INIT), port, channel, instrument, layer A-P, layer type, CC number, presets, INIT |
| TRKMODE | mode (Off/Normal/Transpose/Arpeggiator), bus, note stack: Hold, Sort, ReSt., STrg, FTS, Sustain |
| TRKDIR | direction (Forward/Backward/PingPong/Pendulum/Rand.Dir/Rand.Step/Rand.D+S), Fwd., Back, Replay, Repeat, Skip, Itv., SyncM |
| TRKDIV | clock divider (1,2,4,8,8T,16,16T,32), sync to measure, manual |
| TRKLEN | length (1-256), loop, quick selection |
| TRKTRAN | scale, octave, semitone |
| TRKGRV | groove style, intensity, global, sync step, delay/length/velocity, number of steps |
| TRKMORPH / TRKRND / TRKEUCLID | morph, random generator, euclid |
| TRKJAM (Live) | step / live / pattern / MIDI / misc |
| FX | echo, humanize, robotize, limit, LFO, duplicate, loop, scale |
| MANUAL, BPM, OPT, MIDI, SYSEX, CV, ETH, DISK, INFO | global pages |
| SONG, PATTERN, MUTE, MIXER | done: dashboard, track overview, mixer |

## Proposed pages (16 columns each)
1. TRACK (Event+Mode+Inst) – **Event page done**.
2. TIMING (Divider + Length + Direction + Repeat) – mock-up in `mock_timing.c`.
3. GROOVE & TRANSPOSE · 4. GENERATE (random, euclid, morph) · 5. FX · 6. LIVE / MANUAL · 7. GLOBAL (BPM, options, MIDI, SysEx, CV, Ethernet, disk, info).

## What it means for the firmware
Real UI pages: read/write the track settings with `SEQ_CC_Get/Set` and `SEQ_UI_CC_Inc` using the same limits as the original pages, for all selected tracks; map encoders/buttons/LEDs per page (as done in `seq_ui_trkevnt.c`). The LCD keeps working. Testable without hardware through the virtual panel.
