# Logboek myproject (branch four-screens-usb)

Doel: SEQ V4+ met 4 x SSD1322 (256x64) i.p.v. 2 tekst-LCD's; eerst in software testen.

- 2026-11-22 Fork gemaakt (virtuawise/mios32), branch `four-screens-usb`.
- 2026-11-22 Paneel `/oled` (browser) met 4 OLED-vlakken in `panel/` (benadering uit LCD-tekst).
- 2026-11-22 Toolchain zonder sudo geïnstalleerd; ongewijzigde SEQ V4 bouwt (zie CLAUDE.md in de Midibox-map).
- 2026-11-22 `oled_gui/`: tekenlaag + Mac-test; eerste render lijkt op het ontwerp.
- Volgende: tekenlaag koppelen aan de firmware (ssd1322-driver), daarna framebuffer via USB-SysEx naar `/oled`.
