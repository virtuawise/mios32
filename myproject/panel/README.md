# Virtueel paneel voor MIDIbox SEQ V4+

- `mbseq_panel.py` – virtueel frontpaneel + HTTP-API (poort 8765), praat met de SEQ via USB-remote.
- `panel_oled.html` – ontwerp met 4 OLED-vlakken (256x64), bereikbaar op `/oled`. OLED-inhoud is een benadering uit de LCD-tekst; echte waarden volgen met de firmware in deze branch.

Gebruik: `pip3 install mido python-rtmidi`, dan `python3 mbseq_panel.py --no-browser` en open http://127.0.0.1:8765/oled
