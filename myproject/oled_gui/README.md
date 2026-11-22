# oled_gui – drawing layer for 4 x SSD1322 (256x64)

`oled_gui2.c/.h` renders **64x64 cells** (4 bit grey, 2048 bytes: 32 bytes per row, first pixel = high nibble). It only needs `<mios32.h>` types and the `glcd_font` module, so it runs on the Mac (`host_test2.c`, `host_stub/mios32.h`) and in the firmware.

Cell types
- `OG2_RenderCellBoxed` – step cell (note / layer value in a box, length bar, three small boxes, mute hatching). Older renderers (`cell_note`, `cell_layer`) are kept but unused.
- `OG2_RenderTrackTile`, `OG2_RenderMixerTile` – track overview and mixer.
- `OG2_RenderDashCell` – dashboard (draws a whole 256-px screen and shows one cell via `ox_shift`).
- `OG2_RenderTextGroupCell`, `OG2_RenderPlainCell` – LCD text as boxes / as plain text.
- `OG2_RenderEventCell` – Event page layout.

Tests: see `../README.md`. `pgm2png_gray.py` converts the 4-bit PGM output to a PNG (no extra libraries). `mock_*.c` + `mock_ui.h` draw design mock-ups of new pages. `DESIGN.md` holds the design ideas, `UI_ANALYSIS.md` the analysis of all MBSEQ pages.
