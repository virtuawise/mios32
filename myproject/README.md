# myproject – MBSEQ V4+ with four 256x64 OLED screens

Branch `four-screens-usb` of this MIOS32 fork. Goal: run the MIDIbox SEQ V4+ firmware with **four SSD1322 OLEDs (256x64)** instead of the two 2x40 character LCDs,
with a new page layout built from blocks ("boxes"), and test everything in software first (no OLED hardware needed).

| Path | What |
|---|---|
| `oled_gui/` | Platform-independent drawing layer (`oled_gui2.c/.h`, 4-bit grey, 64x64 cells) + Mac test programs and design mock-ups |
| `panel/` | Virtual front panel (`mbseq_panel.py`, browser UI at `/oled`) that receives the OLED frames from the firmware over USB-MIDI |
| `tools/mios32_flash.py` | Uploads a `.hex` to the core over USB-MIDI (same protocol as MIOS Studio) |
| `LOGBOOK.md` | Chronological log of what was done |

Firmware changes live in `apps/sequencers/midibox_seq_v4/core/`: `seq_oled.c/.h` (collects the screen data and streams cells), `seq_midi_sysex.c` (SysEx command `09 07`), `seq_ui_trkevnt.c` (new Event page mapping), `seq_ui_mixer.c`, `seq_lcd.c` (small getters).

## How it works
- Each screen is split into four 64x64 **cells** (one per GP column; 16 cells in total). A cell is rendered only when its input changes, run-length encoded (PackBits), 7-bit packed and sent as SysEx `F0 00 00 7E 4D <id> 09 07 <screen> <cell> <data> F7` in remote-server mode.
- `panel/mbseq_panel.py` decodes the cells into 4-bit framebuffers (`/oled_raw?d=N`); `panel_oled.html` draws them.
- The firmware chooses a view from the current UI page: step/layer view (Edit), track overview (Mute), mixer, dashboard (BPM/Pattern/Song/Save), Event page (new layout), plain text for the other pages (until they get a layout of their own).

## Build and flash (macOS)
The Homebrew `arm-none-eabi-gcc` has no newlib; use the Arm GNU Toolchain (extract the `.pkg` without sudo, e.g. into `~/toolchains/arm-pkg`).
```
export PATH=$HOME/toolchains/arm-pkg/Payload/bin:$PATH MIOS32_PATH=<this repo> MIOS32_FAMILY=STM32F4xx MIOS32_PROCESSOR=STM32F407VG MIOS32_BOARD=MBHP_CORE_STM32F4 MIOS32_LCD=universal
cd apps/sequencers/midibox_seq_v4 && make          # do not use -j
python3 -u myproject/tools/mios32_flash.py flash project.hex    # stop mbseq_panel.py first (it holds the MIDI port)
```
The bootloader waits only ~2 s after the reset, so the flash tool keeps resending the first block until it is acknowledged.

## Mac tests (no hardware)
`cd myproject/oled_gui && cc -I host_stub -I ../../modules/glcd_font -I . -o host_test2 host_test2.c oled_gui2.c ../../modules/glcd_font/glcd_font_normal.c ../../modules/glcd_font/glcd_font_small.c`
then `./host_test2 boxed|boxedlen|boxedmute|tiles|dash|text0|velstyles` and `python3 pgm2png_gray.py <in>.pgm <out>.png 2`.
Mock-ups of new pages: `mock_event.c`, `mock_timing.c` (shared grid in `mock_ui.h`).

## Conventions
- All text shown on the screens and all documentation is **English**.
- LCD line endings in the MIOS32 sources are CRLF – edit them in binary mode.
