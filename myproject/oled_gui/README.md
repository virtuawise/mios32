# oled_gui – tekenlaag voor 4 x SSD1322 (256x64)

Tekent de stap-weergave van de SEQ (16 stappen = 4 per scherm) in een 1-bit framebuffer.
- Per stap: kop (stapnummer + noot; gevuld als de stap getriggerd is), ring van 13 punten met de waarde, waarde als getal, labels ACC/SLD, wit kader voor de afspeelpositie.
- API: zie `oled_gui.h` (`OLEDGUI_RenderSteps`).
- Mac-test (geen SEQ nodig): `cc -I host_stub -I ../../modules/glcd_font -I . -o host_test host_test.c oled_gui.c ../../modules/glcd_font/glcd_font_normal.c ../../modules/glcd_font/glcd_font_small.c && ./host_test 5 && python3 pgm2png.py frame.pgm frame.png 1` (argument = afspeelstap 0..15).
- Beeldformaat: per scherm 2048 bytes, rij na rij, 32 bytes per rij, bit 0 = meest linkse pixel. Het SSD1322-framebuffer van de driver heeft een ander formaat: converteren komt bij de koppeling.
