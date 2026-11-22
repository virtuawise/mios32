# Logbook (branch `four-screens-usb`)

Goal: SEQ V4+ with 4 x SSD1322 (256x64) instead of two 2x40 LCDs; develop and test in software first.

- Fork created (virtuawise/mios32), branch `four-screens-usb`; toolchain installed without sudo; unchanged SEQ V4 builds.
- Drawing layer `oled_gui` (host-testable) and virtual panel page `/oled`.
- Firmware streams the OLED content to the panel over USB-MIDI (SysEx `09 05`, later `09 06`, now `09 07`: per 64x64 cell, 4-bit, run-length encoded).
- `tools/mios32_flash.py`: flashing from Python (bootloader window is ~2 s, so the first block is resent until acknowledged).
- Views: step view with pitch/length/velocity/probability/delay, glide as a symbol, parameter-layer views (length, probability, delay, roll, ...), track overview (Mute page), mixer, dashboard (BPM/Pattern/Song/Save).
- Step cell redesigned as a "box" (Elektron style): big value, length bar, three small boxes below (velocity number / glide / roll-skip-probability); mute = dimming + hatching + M badge.
- Text pages (Mode, Direction, Divider, ...) show the LCD text enlarged; the Event page has merged boxes.
- **Event page (menu -> Event) has its own layout and its own encoder/button/LED mapping** (`seq_ui_trkevnt.c`: `Encoder_HandlerMap`, table `evt_map`), with an INIT hold progress bar.
- Next: Timing page, Groove/Transpose, Generate, FX, Live/Manual, Global pages (see `oled_gui/UI_ANALYSIS.md`); drum variant of the Event page; real SSD1322 driver hookup.
