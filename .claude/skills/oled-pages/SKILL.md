---
name: oled-pages
description: Rules and workflow for the 4 x OLED (256x64) page layouts and the (virtual) front panel design of the MIDIbox SEQ V4+ fork (branch four-screens-usb). Use when creating or changing any OLED page, the panel layout (buttons, switches, caps, encoders), box layout, renderer (myproject/oled_gui/oled_gui2.c) or page fill function (SEQ_UI_*_OledGet, seq_oled.c), and when the user reports that a page "klopt niet" / looks wrong.
---

# OLED pages – rules and workflow

The user (Dutch speaking, answers in Dutch) wants a calm, consistent UI and expects me to build, flash and check on the real SEQ myself.

## Layout rules (all pages)
1. 4 screens x 4 columns = 16 columns; column k sits above GP encoder/button k+1. Every box sits above the GP controls that change it.
2. **A box never runs across a screen border** (columns 0-3, 4-7, 8-11, 12-15). Design layouts so boxes stay within one screen. Where the original GP mapping forces a span across a border, the renderer splits it: the largest part shows label + value, smaller parts repeat them only if they fit, else show « / ».
3. **The first item starts at GP1.** If the original page leaves GP1/GP2 unused or the order is odd, give the page its own encoder/button mapping (see `seq_ui_fx_scale.c`, `seq_ui_trktran.c`, `seq_ui_timing.c`) instead of copying the gaps.
4. Header per screen: small title top-left + dotted line. Box label top-left in the normal 6x8 font (never the 4x5 small font).
5. Values: 1x bold, upper case, written out in full, all at the same height (`VALY`). No 2x values, no per-box size switching.
6. No bars under values (meters/graphs/keyboards are separate box types and stay within one screen).
7. On / active choice = filled box; last touched / selected = double frame; not applicable = dimmed.
8. Lists: a row of GP buttons under one title = ROW box (one text above each button, chosen one filled); separate options per column = separate boxes.
9. Labels max. 8 characters in a 1-column box (abbreviate sensibly: "JMP BACK", "RND VAL."); avoid long info text blocks – one small box with the essential info is better.

## Front panel (virtual panel `myproject/panel/panel_oled.html` = the hardware design)
The virtual panel is the design for real hardware, so **buttons, switches and caps must be buildable and consistent**:
1. **One switch type and one cap size for all buttons**: MX-style key switches (silent tactile, LED window) with an SK6812 (WS2812-compatible) RGB LED below each key – all on one data line, MIOS32 has `modules/ws2812` (used in NG, CV v2, SEQ v4 Lite; not yet in the SEQ v4); smoked translucent 1U caps, 18 x 18 mm (e.g. Elacgap XDA "transparent black"): off almost black, on the whole cap glows; no pudding caps (the skirt leaks light to neighbouring keys). Opaque switch housing with LED window against bleeding. Front plate with one cutout per row of keys (only ~1 mm between 18 mm caps on 19.05 mm – no panel material fits between keys of a row). Not midiphy's caps (the user does not want their L4 design; Matias/Alps caps in black with light are not available). Switches on a PCB, 1.5 mm plate (14 x 14 mm cutouts, FR4 is fine) that also holds the encoders. No round buttons, no mixed cap sizes – each size/shape means another part. Order a few switches/caps to test feel before committing.
0. **The drawing must always match the chosen parts**: draw every part at its real size (caps 18 mm, OLED PCB/bezel/active area, encoder, cutouts) and update the drawing in the same step as a part decision. Never draw a part you just advised against.
2. **Everything on one grid**: 19.05 mm (= 62 px, 1 mm = 3.255 px) between keys within a group of 4; the left block uses the same grid and the same row heights as the GP / select / function rows.
   **OLED = Newhaven NHD-3.12-25664UC (3.12" SSD1322 256x64, desktop case)**, datasheet rev. 7 (2024): PCB 89.2 x 44.0 x 6.0 mm, bezel 89.0 x 29.6 mm (6.6 mm above PCB bottom), opening 82.0 x 22.0 mm, active 76.78 x 19.18 mm centred and 12.62 mm above the PCB bottom (12.2 mm below the top, 1x20 header along the top edge, M2.5 holes 83.2 mm apart). Groups 91.0 mm apart (89.2 + 1.8 mm), active area ~ the 4 keys (76.2 mm); encoders below the OLED PCB. Panel ~475 mm wide. (2.8" glass 84 x 25.8 mm, active 69.1 x 17.26 mm was rejected: smaller and offset from the keys.)
3. Layout follows the midiphy SEQ v4+ (photo/hwcfg `hwcfg/midiphy_lh`): 16 push encoders, GP row, 16 select buttons directly below (no labels), function row Copy..Utility in hwcfg order; left block: datawheel on its own (nothing around it), select-mode buttons, Solo/Metr./Loop/Rec./Live, transport.
4. Check real parts before claiming something fits: OLED module outline vs. the 76.2 mm column group, switch depth (key switches need their own PCB + front plate), encoder spacing. Say what is not verified.
5. Every button in the panel must reach the SEQ (remote keyboard note in `SEQ_UI_REMOTE_MIDI_Keyboard` + `BUTTON_NOTES` in `mbseq_panel.py`); a release without a press must not trigger anything.
6. Check the layout with a static headless-Chrome render (`file://` copy of the page; the live page keeps connections open) and tell the user to reload with Cmd+Shift+R.

## Box types (`og2_col_t.kind`)
VALUE, TOGGLE, CHOICE, ACTION, TEXT (one per page, `pg.text`, lines with '|'), GRAPH, METER, KEYS (note blocks, `meter` = root), ROW, STEPS (track box: 16 gates + playing step, used on Mute), SPAN.
The user likes seeing *where and how a track runs* (gates + play position) – keep such live information when restyling.

## Where a page gets its layout
- Own renderer + mapping: Event (`seq_ui_trkevnt.c`), Mode, Divider/Length/Direction (`seq_ui_timing.c`).
- Column page filled by the page: `SEQ_UI_<PAGE>_OledGet(og2_page_t *)` in the page source, registered in `page_fill()` in `seq_oled.c`.
- Column page from LCD columns in `seq_oled.c`: FX table (`lcd_pages[]`), MIDI, Instrument, Euclid, MIDI monitor, Disk, messages.
- Everything else: generic converter `lcd_generic_fill()`.
- MIOS32 `sprintf` has no `%+d`; RAM is tight (check the link step for "RAM overflowed"); keep stack buffers small.
- MIOS32 sources use CRLF line endings: edit with Python/perl in binary mode.

## Workflow (always do all steps)
1. Host preview where possible: `cd myproject/oled_gui && cc -I host_stub -I ../../modules/glcd_font -I . -o host_test2 host_test2.c oled_gui2.c ../../modules/glcd_font/glcd_font_normal.c ../../modules/glcd_font/glcd_font_small.c`.
2. Build: `cd apps/sequencers/midibox_seq_v4 && export PATH=$HOME/toolchains/arm-pkg/Payload/bin:$PATH MIOS32_PATH=<repo> MIOS32_FAMILY=STM32F4xx MIOS32_PROCESSOR=STM32F407VG MIOS32_BOARD=MBHP_CORE_STM32F4 MIOS32_LCD=universal && make` (no -j).
3. Flash: stop the panel (`pkill -INT -f mbseq_panel.py`), `python3 -u myproject/tools/mios32_flash.py flash project.hex`, restart the panel in the background (`cd myproject/panel && nohup python3 -u mbseq_panel.py --no-browser > /tmp/panel.log 2>&1 &`).
4. Check on the device: `myproject/tools/oled_walk.py out.png "label|b:MENU+GP10;b:GP10" ...` (b: button, e:ENC:d encoder) or `oled_walkmenu.py` for all 42 pages; look at every PNG. EXIT toggles to the Page Menu (MENU is only a hold button).
5. Do not trigger actions that change the user's data (Save, Delete, Clear, Copy, mutes) – or undo them right away and say so.
6. Commit per fix (English message, Co-Authored-By line), update `myproject/LOGBOOK.md` / `oled_gui/DESIGN.md` when rules or structure change.
