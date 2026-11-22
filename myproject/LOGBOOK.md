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
- Event page drum variant (layers A/B, drum, note, velocity N/A); BEAT LED and menu-button LED on the panel.
- **Timing page**: the Divider, Length and Direction pages share one layout and mapping (`seq_ui_timing.c`): LENGTH, LOOP, length PRESETS | TIMEBASE presets, DIVIDER (raw, button = triplet), SYNC BAR, MANUAL | DIRECTION, FORWARD, JUMP BACK | REPLAY, REPEAT, SKIP, INTERVAL. Datawheel and Up/Down change the last touched column.
- Labels and headers in the normal 6x8 font; the panel page draws the OLEDs 4x oversampled (pixelated downscaling dropped pixel rows).
- **Mode page**: MODE as 4 chips (turn/press = choose), BUS, NOTE first/last, HOLD, SORT, RESTART, STEP TRG, FORCE TO SCALE, SUSTAIN, info box with a short explanation of the last touched item (`seq_ui_trkmode.c`).
- Divider, Length and Direction each have their own page again (one shared Timing screen for three menu buttons was confusing); columns follow the GP layout of the original pages (GP1 = track, GP9-16 = presets/timebase, GP2-8 = directions).
- GP LEDs on menu pages only light the menu button of the page; boxed pages follow fixed UI rules (`oled_gui/DESIGN.md`).
- **All menu pages in boxes**: generic column page (`og2_page_t`, `OG2_RenderPageCell`: value, toggle, choice, action, text, graph, meter, keyboard). Transpose (own mapping: octave, semitone, keyboard, scale steps), Groove (with delay curve), Trigger assign, Morph (meter), Manual, FX menu: fill functions in the page sources. FX sub-pages (Echo, Humanize, Robotize, Limit, LFO, Duplicate, Loop, Scale) from a table in `seq_oled.c` (GP columns + label, value read from the LCD). MIDI page: sections left, settings right (selection from the page's own LED pattern). Pages without a description (SysEx, Options, ...) use the LCD box view in the same style.
- SysEx, Mixer, Mute (level meters, MUTE boxes, layer view while MUTE is held), Pattern (per group: category + pattern, pending switch) and Song (LCD columns + mode, position, playing patterns) as column pages; per-cell change detection for column pages.
- Next (small points): step view / BPM / Save dashboard in the same style; Transpose, Generate, FX, Live/Manual, Global pages (see `oled_gui/UI_ANALYSIS.md`); real SSD1322 driver hookup.
