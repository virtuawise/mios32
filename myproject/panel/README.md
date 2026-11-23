# Virtual panel for the MIDIbox SEQ V4+

- `mbseq_panel.py` – virtual front panel + HTTP API (port 8765); talks to the SEQ over USB remote (`RemotePort` in `MBSEQ_GC.V4`).
- `panel_oled.html` – the 4-OLED (256x64) front panel at `/oled`; shows the frames streamed by the firmware.

Usage: `pip3 install mido python-rtmidi`, then `python3 mbseq_panel.py --no-browser` and open http://127.0.0.1:8765/oled.
Stop it with Ctrl+C (SIGINT) so the SEQ leaves remote mode; it holds the MIDI port, so stop it before flashing.

## midiphy SEQ v4+ layout (`/oled`)
Arranged after a photo of the real (left-handed) midiphy SEQ v4+, with 4 OLEDs instead of its 2 LCDs:
left block with the datawheel (push) on its own at the top and below it, in a grid aligned with the right rows, the 8 select-mode
buttons (Track, Param, Trigger, Instr. / Step, Mute, Phrase, Bookm.; the active one lights up), Solo/Metr./Loop/Rec./Live and transport.
One switch type for every button: identical square 1U caps on a 19.05 mm grid (62 px), drawn to scale (1 mm = 3.255 px).
OLEDs: Newhaven NHD-3.12-25664UC (3.12" SSD1322 256x64): PCB 89.2 x 44 mm (dashed), bezel 89 x 29.6 mm, active area
76.78 x 19.18 mm (almost exactly above the 4 keys of a group), groups 91 mm apart, encoders below the OLED PCB.
Panel about 475 mm wide (desktop case). The left
block uses the same grid (5 columns) and row heights. No round buttons, nothing around the datawheel; on the right 16 push encoders, 16 GP buttons,
16 select buttons (labels and LEDs follow the select mode: tracks, steps, layers, instruments, mutes, phrases, bookmarks) and
16 round buttons Copy/Paste/Clear/Move/Scroll/Fast/All/Shift(=SELECT) | </>(=DOWN/UP)/Exit/Menu/Edit/Pattern/Song/Utility
(same order as the shift-register assignment in `hwcfg/midiphy_lh`).
The extra buttons reach the SEQ through the MIDI Remote Keyboard (notes 0x00-0x20 and 0x61-0x6E, see `SEQ_UI_REMOTE_MIDI_Keyboard`);
the select button LEDs come from SysEx `09 0A`. The SD card keeps Wilba's MBSEQ_HW.V4 (backup in `../sdcard_backup/`).
