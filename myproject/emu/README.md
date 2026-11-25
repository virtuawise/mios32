# MIDIbox SEQ V4+ emulator for macOS

Runs the **real SEQ firmware** (`apps/sequencers/midibox_seq_v4/core`, all pages, sequencer, files) and the real MIOS32
common code (MIDI, SRIO, DIN/DOUT/ENC, LCD) on the Mac, so the hardware can stay untouched while developing.
No JUCE, no Xcode project: plain C + pthreads + CoreMIDI.

## Use
Requirements: Xcode command line tools (`clang`, `make`) and `pip3 install mido python-rtmidi`.
```
cd myproject/emu
make                 # ~1 min the first time
./run.sh             # emulator + its own virtual panel: http://127.0.0.1:8766/oled  (Ctrl-C stops both)
```
- First start creates `sdcard.img` (256 MB FAT32) with the files of the newest `../sdcard_backup/<date>/` and lets the
  SEQ create a session `EMU` ("new EMU", as on the real SEQ). With your real card: `rm sdcard.img; ./make_sdcard.sh /Volumes/<card>`.
- Look into / edit the SD card: `hdiutil attach -imagekey diskimage-class=CRawDiskImage sdcard.img` (eject before starting).
- The panel of the real hardware stays on port 8765; the emulator panel is 8766. The tools work with both:
  `MBSEQ_PANEL=http://127.0.0.1:8766 python3 ../tools/oled_walk.py out.png "bpm|b:MENU+GP13;w:1"`.
- Firmware changes: edit the sources as usual, `make`, restart `./run.sh` - no flashing.
- SEQ debug messages (DEBUG_MSG) are printed in the terminal as `[SEQ] ...`.

## MIDI
- 4 CoreMIDI ports **"MBSEQ EMU Port 1..4"** = USB1..4 of the core: connect a DAW, synth or MIDI monitor.
- The virtual panel uses a direct link (TCP 127.0.0.1:8767, `mbseq_panel.py --emu`) instead of CoreMIDI: the macOS MIDI
  server stops delivering from a virtual source after ~100 KB of large SysEx (verified with a standalone test), and the
  OLED screens are exactly that. While the panel is connected, the screen/LED SysEx (`F0 00 00 7E 4D .. 09`) only goes
  over the link; normal MIDI goes to the CoreMIDI ports as usual.
- Output is paced like USB full speed MIDI (16 packages per ms, "buffer full" otherwise), as the firmware expects.

## How it works (host/)
| File | What |
|---|---|
| `emu_os.c` | one global lock = one CPU; 1 ms loops = `programming_models/traditional/main.c` hooks (SRIO scan, DIN/ENC/MIDI handlers) and the SEQ tasks (`mios32/tasks.c`); MIOS32 timers (BPM clock) as threads; `APP_Background` idle thread; CoreFoundation run loop for CoreMIDI |
| `emu_mios32.c` | SYS, DELAY, STOPWATCH, BOARD (J5/J15/LED: no-ops), SPI = SRIO chain (DIN all released), UART (none), SD card = `sdcard.img`, APP_LCD (no local display), WS2812/OSC stubs, newlib-compatible `strtok_r` |
| `emu_coremidi.c` | USB MIDI ports <-> CoreMIDI, MIDI byte parser, SysEx collected into whole messages, 1 ms batching |
| `emu_socket.c` | panel link (TCP 8767) |
| `emu_prelude.h`, `FreeRTOS.h`, `mios32_config.h` | what the MIOS32 build normally gets from FreeRTOS / the Makefile; the SEQ's own mios32_config.h |

Compiled with `MIOS32_FAMILY_EMULATION` (the SEQ sources already have emulation paths for tasks/semaphores/terminal).
Buttons and encoders come from the panel via the remote keyboard (as with the real SEQ); in the emulator they also reset
the screensaver (on the real SEQ only physical buttons do).

Not emulated: Ethernet/OSC, AOUT/CV, physical MIDI DIN ports, the real OLED/RGB hardware drivers (J15/J4B are no-ops).

Debug switches: `EMU_STATS=1` (task rates, remote state every ~5 s), `EMU_MIDILOG=1` (incoming MIDI), `EMU_FSTEST=1`
(FatFs write test on the image, then exit).

## 64-bit findings in the firmware (fixed / worked around)
- `seq_file_presets.c`: `sizeof(array_of_pointers) / 4` -> `sizeof(a) / sizeof(a[0])` (crashed on the Mac; same result on the STM32).
- `strtok_r`: macOS sets the rest pointer to NULL after the last token, newlib to ""; the terminal relies on newlib -> emulator wrapper.
- `seq_file_c.c` reads `MIDI_OUT_MClock_Ports 0xffffffff` with `strtol`: on the STM32 this saturates to 0x7fffffff
  (bit 31 lost), on the Mac it becomes -1 and the line is ignored. Harmless, left as is.

## Branches
The same emulator is in branch `emulator` (official firmware only, worktree `../mios32_emulator`). The Makefile adds the
OLED renderer only when `myproject/oled_gui` exists. Keep emulator fixes in sync in both branches; run one emulator at a time.
