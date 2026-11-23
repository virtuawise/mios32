# SD card backups

Copies of the configuration files on the SEQ's SD card, read over USB-MIDI with the MIOS file browser
(`curl "http://127.0.0.1:8765/api/fb?cmd=read%20/MBSEQ_HW.V4&quiet=2&maxwait=60"` while `panel/mbseq_panel.py` runs).

| Folder | Content |
|---|---|
| `2026-11-23/` | `MBSEQ_HW.V4` (Wilba front panel, 2015 version with MIDI_REMOTE_KEY 96 – needed by the virtual panel), `MBSEQ_GC.V4` (global config) – taken before switching the virtual panel to the midiphy layout |

To restore: write the file back to the root of the SD card (card reader, or MIOS Studio file browser) and restart the SEQ.
