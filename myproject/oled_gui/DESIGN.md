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
