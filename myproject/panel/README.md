# Virtual panel for the MIDIbox SEQ V4+

- `mbseq_panel.py` – virtual front panel + HTTP API (port 8765); talks to the SEQ over USB remote (`RemotePort` in `MBSEQ_GC.V4`).
- `panel_oled.html` – the 4-OLED (256x64) front panel at `/oled`; shows the frames streamed by the firmware.

Usage: `pip3 install mido python-rtmidi`, then `python3 mbseq_panel.py --no-browser` and open http://127.0.0.1:8765/oled.
Stop it with Ctrl+C (SIGINT) so the SEQ leaves remote mode; it holds the MIDI port, so stop it before flashing.

## midiphy-style layout (`/oled`)
The 4-OLED panel follows the midiphy SEQ v4+ functional layout: 16 push encoders (press + turn = fast), 16 GP buttons,
**16 track buttons** directly below (LEDs: green = selection, red = playing position / selected tracks; they follow
Step View, Track Sel, Par/Trg/Ins Sel, Mute, Phrase and Bookmark like on the midiphy panel), two rows of function buttons
(Edit/Mute/Pattern/Song/Phrase/selection buttons/Utility/Solo/Fast/All, Copy/Paste/Clear/Move/Scroll/Undo/Live/Record/Loop/Follow/Metronome)
and a right block with datawheel, Menu/Select/Exit, Up/Down and transport.
The extra buttons reach the SEQ through the MIDI Remote Keyboard (notes 0x00-0x20 and 0x61-0x6E, see `SEQ_UI_REMOTE_MIDI_Keyboard`);
the track button LEDs come from SysEx `09 0A`. The SD card keeps Wilba's MBSEQ_HW.V4 (backup in `../sdcard_backup/`).
The exact physical positions of the midiphy panel are not in its hwcfg file – this is an approximation of its button set and grouping.
