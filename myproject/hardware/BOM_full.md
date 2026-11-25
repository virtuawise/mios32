# Full sequencer - DigiKey order (4 column PCBs + 1 left PCB)

Counts from `column/column.kicad_pcb` (x4) and `left/left.kicad_pcb` (x1) plus spares. Stock and prices checked on the DigiKey product pages on 2026-11-25 ("live"), or from search results ("snippet", can lag). Prices USD at the quantity shown; "est." = estimated.

Paste `digikey_bulk_full.txt` into DigiKey's Bulk Add.

| Qty | Part | What | Unit | Total | Stock check |
|---|---|---|---|---|---|
| 4 | NHD-3.12-25664UCY2-ND | OLED 3.12in 256x64 yellow | $38.2 | $152.68 | live 832 in stock |
| 18 | 74HC165D,653 | 74HC165 SOIC-16 (15 + 3 spare), Nexperia - the TI SN74HC165DR is out of stock | $0.223 | $4.01 | live 22737 |
| 3 | SN74HC595DR | 74HC595 (2 + 1) | $0.65 | $1.95 | live 39725 |
| 2 | SN74AHCT1G125DBVR | LED data buffer (1 + 1) | $0.14 | $0.28 | live 68462 |
| 2 | AP2112K-3.3TRG1 | 3.3 V LDO (1 + 1) | $0.29 | $0.58 | live 401213 |
| 2 | MCP100T-300I/TT | reset supervisor (1 + 1) | $0.42 | $0.84 | live 10320 |
| 140 | ERJ-3EKF1002V | 10k 0603 (125 + 15), Panasonic - Yageo RC0603FR-0710KL out of stock | $0.0195 | $2.73 | snippet ~857k |
| 20 | RC0603FR-07680RL | 680 R 0603 (14 + 6) | $0.028 | $0.56 | live 19340 |
| 5 | ERJ-3EKF33R0V | 33 R 0603 (1 + 4), Panasonic - Yageo out of stock | $0.1 | $0.50 | snippet 177k, price est. |
| 85 | CC0603KRX7R9BB104 | 100 nF 50 V X7R 0603 (72 + 13), Yageo - Samsung CL10B104 out of stock | $0.06 | $5.10 | snippet ~4.8M, price est. |
| 5 | CC0603KRX5R7BB105 | 1 uF 16 V X5R 0603 (2 + 3), Yageo - Samsung CL10A105 out of stock | $0.19 | $0.95 | snippet 484k |
| 12 | GRM21BR61E106KA73L | 10 uF 25 V X5R 0805 (9 + 3), Murata - Samsung CL21A106 out of stock | $0.17 | $2.04 | DigiKey substitute table 1.67M |
| 6 | CL21A226MPQNNNE | 22 uF 10 V X5R 0805 (4 + 2) - CL21A226MAQNNNE is obsolete | $0.19 | $1.14 | live 18796 |
| 2 | EEE-FK1A221XP | 220 uF 10 V, 6.3 mm can (1 + 1) - the -FK1A221P is 8 mm wide | $0.77 | $1.54 | live 25006 |
| 6 | 1528-4960-ND | SK6812MINI-E reverse mount, 10-pack (53 needed, 60 supplied) | $2.95 | $17.70 | live 358 |
| 6 | 1528-4954-ND | Kailh MX brown, pack (53 needed + spares) - CHECK pack size | $6.95 | $41.70 | live only 9 in stock |
| 6 | 1528-5008-ND | smoked DSA caps, 10-pack (53 + spares) | $5.95 | $35.70 | live 24 |
| 15 | 3FTH9 | MEC 3FTH9 small key (13 + 2) | $2.93 | $44.01 | live 7082 (679-3730-5-ND) |
| 10 | 679-2120-ND | MEC 1T09 black cap (8 select-mode keys + 2) | $0.754 | $7.54 | live 540 |
| 26 | 97730506330R | Wurth WA-SMSI M1.6 standoff 5 mm (22 stack holes + 4) - CHECK count against the plate stack | $0.81 | $21.06 | live 1428 (732-13685-1-ND) |
| 22 | 61201021621 | box header 2x5 (20 + 2) | $0.443 | $9.75 | live 58923 |
| 2 | 61201621621 | box header 2x8, core J15A (1 + 1) | $0.6 | $1.20 | live 6344 |
| 10 | 61300311121 | pin header 1x3 (9 + 1) | $0.107 | $1.07 | live 199350 |
| 5 | 61302011121 | pin header 1x20, OLED (4 + 1) | $1.16 | $5.80 | live 5819 |
| 2 | WM18823-ND | Molex KK396 header 0009652028 (1 + 1) | $0.24 | $0.48 | live |
| 2 | WM2100-ND | Molex KK396 housing 09-50-3021 (1 + 1) | $0.10 | $0.20 | live |
| 6 | WM2300-ND | Molex KK396 crimp 08-50-0106 (2 + spare) | $0.12 | $0.72 | live |
| 22 | 61201023021 | IDC socket 2x5 for ribbon cable (20 + 2) | $0.447 | $9.83 | live 39157 (732-2102-ND) |
| 2 | 61201623021 | IDC socket 2x8 for ribbon cable (1 + 1) | $0.75 | $1.50 | live 31730 (732-2103-ND) |
| 10 ft | 19-3365/10100-DS-ND | 3M 3365/10 ribbon cable 10-way gray (length assumed, adjust) | $0.64/ft | $6.40 | live 11680 ft |
| 10 ft | 19-3365/16100-DS-ND | 3M 3365/16 ribbon cable 16-way gray (length assumed, adjust) | $0.834/ft | $8.34 | live 222 ft |

**Total about $388** (excl. shipping and tax).

## Totals per function
- 15 x 74HC165 (3 per column PCB x 4 + 3 on the left), 2 x 74HC595, 53 x SK6812MINI-E, 53 x MX switch + cap, 17 x EC11E encoder (4 per column x 4 + datawheel), 13 x 3FTH9, 4 x OLED.

## Not available at DigiKey (order elsewhere)
| Qty | Part | Where / remark |
|---|---|---|
| 19 | Alps EC11E15244G1 (vertical, 20 mm shaft, push) + 19 knobs | DigiKey 0 in stock; TME had it |
| 5 | APEM 1T16 frosted white cap | RS 495-2501 (select keys S9-S13: Solo/Metr./Loop/Rec./Live) |
| 5 | LED 2x3x4 mm rectangular THT (4 yellow, 1 red) for the lit 3FTH9 keys | not sourced yet |
| 1 | 3 mm red LED (beat) + 5 mm spacer | any |
| 1 | 5 V supply, at least 4 A | for 53 LEDs + 4 OLEDs |
| 1 | switch plate (FR4 1.5 mm) and front plate (aluminium) | `panel/` files |
| - | M1.6 x 10 screws, 2.5 mm pin spacers for the OLED pins | |

## Things to check before ordering
- Kailh 4954: only 9 packs in stock; confirm it is a 10-pack before counting on 53 switches.
- Standoffs: 22 assumes one per stack hole (6 on the left PCB, 4 per column PCB).
- R26-R33 on the left PCB are left unfitted (select keys without LED), so the LED holes stay empty there.
