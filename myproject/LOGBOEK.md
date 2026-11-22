# Logboek myproject (branch four-screens-usb)

Doel: SEQ V4+ met 4 x SSD1322 (256x64) i.p.v. 2 tekst-LCD's; eerst in software testen.

- 2026-11-22 Fork gemaakt (virtuawise/mios32), branch `four-screens-usb`.
- 2026-11-22 Paneel `/oled` (browser) met 4 OLED-vlakken in `panel/` (benadering uit LCD-tekst).
- 2026-11-22 Toolchain zonder sudo geïnstalleerd; ongewijzigde SEQ V4 bouwt (zie CLAUDE.md in de Midibox-map).
- 2026-11-22 `oled_gui/`: tekenlaag + Mac-test; eerste render lijkt op het ontwerp.
- Volgende: tekenlaag koppelen aan de firmware (ssd1322-driver), daarna framebuffer via USB-SysEx naar `/oled`.
- 2026-11-22 Firmware: `core/seq_oled.c/.h` (stap-weergave van de geselecteerde track), nieuw remote-SysEx-commando 0x05 (OLED-framebuffer, 7-bit packed, 2341 bytes per scherm) in `seq_midi_sysex.c`, hook in `SEQ_TASK_Period1mS_LowPrio` (app.c), Makefile haalt `oled_gui` en glcd_font erbij. Bouwt zonder fouten (RAM +19 kB). NOG NIET GEFLASHT of op hardware getest. Paneel decodeert 0x05 en toont het echte beeld op /oled (valt terug op de benadering zonder firmware-frames).
- 2026-11-22 Firmware geflasht met `tools/mios32_flash.py` (Python, zelfde protocol als MIOS Studio). SEQ meldt nu V4+.098. 4 echte OLED-frames komen binnen en worden in /oled getoond.
