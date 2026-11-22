# Virtual panel for the MIDIbox SEQ V4+

- `mbseq_panel.py` – virtual front panel + HTTP API (port 8765); talks to the SEQ over USB remote (`RemotePort` in `MBSEQ_GC.V4`).
- `panel_oled.html` – the 4-OLED (256x64) front panel at `/oled`; shows the frames streamed by the firmware.

Usage: `pip3 install mido python-rtmidi`, then `python3 mbseq_panel.py --no-browser` and open http://127.0.0.1:8765/oled.
Stop it with Ctrl+C (SIGINT) so the SEQ leaves remote mode; it holds the MIDI port, so stop it before flashing.
