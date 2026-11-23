# Design ideas for the 4 x OLED (256x64) front panel – MIDIbox SEQ V4+

The SEQ has far more information than two 2x40 character LCDs can show (16 tracks, per step note/velocity/length/probability/delay/roll/glide/accent, chord bus, arp, patterns, song, mixer, 4 groups).
With four graphic screens we can show it as pictures instead of text. Rules: everything as a block, every encoder gets its own parameter name + value, the SSD1322 supports 16 grey levels (use depth, not just on/off).

1. **Step view with several layers at once** (done): value box, length bar, velocity number, glide / roll / skip / probability symbols, mute hatching.
2. **Track overview** (done, Mute page): one tile per track with gate grid, length, play position, mute/solo.
3. **Harmony / chord bus** (todo): piano strip with the sounding notes, chord name, next chord countdown.
4. **Mixer** (done): one box per channel showing the selected mixer page (port, channel, program, volume, pan, ...), data wheel switches page.
5. **Song / pattern chain** and **dashboard** (done as dashboard for BPM/Pattern/Song/Save).
6. **Pop-over on encoder turn** (todo): value shown large above the column for ~1 s.
7. **Own layouts for every page** (in progress): see `UI_ANALYSIS.md`; the Event page is the first one with its own encoder mapping.

Decisions taken with the user: boxes in "Elektron style"; velocity as a number in a small box below the main box; the vertical bar / fill styles were rejected (fill is reserved for mute); the third small box content is still open; all UI text in English.

## UI rules for the boxed pages (Event, Timing, Mode, and every new page)
1. Header per screen: small title top-left with a dotted line.
2. Every box: label top-left in the normal 6x8 font (never the 4x5 small font).
3. Every value: **1x bold, upper case, written out in full, always at the same height** (`VALY`). No 2x values, no per-box size switching.
4. **No bars under values.** The only bar is the INIT hold progress.
5. On / active choice = filled box; last touched = double frame; not applicable = dimmed.
6. Lists (layers A-P, presets, timebase, direction) = chips in one box; one column per option = separate boxes.
7. A box never runs across a screen border; the first item starts at GP1 (own mapping when the original page leaves gaps). See `.claude/skills/oled-pages/SKILL.md`.

## How a page gets its layout
- Hand-made layout + own mapping: Event, Mode, Divider/Length/Direction (`seq_ui_timing.c`), Transpose.
- Column page (`og2_page_t`) filled by the page itself (`SEQ_UI_xxx_OledGet`): Groove, Trigger assign, Morph, Manual, FX menu.
- Column page from a table in `seq_oled.c` (GP column masks + label, value from the LCD): FX sub-pages; MIDI (sections).
- Column page filled in `seq_oled.c` from LCD columns + fixed labels: Track Instrument, Euclid, MIDI monitor, Disk, messages.
- Anything else (and dialogs): generic LCD converter `lcd_generic_fill()` – boxes per GP column, joined where a word crosses a column border; a title over several single-word columns becomes a ROW box (one text above each GP button, marked choice filled); a value that blinks on the LCD is the selection.
- Box types: VALUE, TOGGLE, CHOICE, ACTION, TEXT, GRAPH, METER, KEYS, ROW.
