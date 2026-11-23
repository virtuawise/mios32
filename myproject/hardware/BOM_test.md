# Test BOM – samples and one prototype set (1 column PCB + left PCB)

Quantities counted from the board files (`column/column.kicad_pcb`, `left/left.kicad_pcb`), with spares.
"VERIFY" = order to check against the 1:1 test prints (`print/`) before ordering the full set.

## A. Fit test with the printed sections (order first)
| # | Part | Exact type / part number | Qty | Why |
|---|---|---|---|---|
| A1 | Key switch, MX type | silent tactile, opaque housing with LED window, 5-pin PCB mount – order 2-3 kinds to compare the feel | 3 x 5 | plate fit, feel, LED window |
| A2 | Key cap 1U | 18 x 18 mm smoked translucent ("transparent black"), e.g. Elacgap XDA | 10 | height above the panel (VERIFY), light through the cap |
| A3 | RGB LED | SK6812MINI-E reverse mount (e.g. Adafruit 4960, 10 pcs) | 10 | fits the cut-out under the switch |
| A4 | Small key (no LED) | APEM Multimec **3FTH9** | 6 | feel; LED legs reach the PCB LED holes (VERIFY) |
| A5 | LED for the small key | 2x3x4 mm rectangular THT LED, warm white / yellow / red, pushed into the 3FTH9 (as midiphy) | 5 each | colour, brightness |
| A7 | Cap for 3F | APEM **1T**, 10.6 x 10.6 mm, transparent frosted white | 6 | 11 x 11 mm hole, 3.3 mm out (VERIFY) |
| A8 | Encoder | Alps **EC11E** with push switch, vertical, 20 mm shaft (H20) | 2 | shaft in the 7.5 mm hole, knob height |
| A9 | Encoder knob | for 6 mm shaft (D or knurled, matching the EC11E chosen) | 2 | look / height |
| A10 | OLED | Newhaven **NHD-3.12-25664UCY2** (yellow) | 1 | window, glass height, jumper R14 -> R15 |
| A11 | Standoff for the front panel | Würth **97730506330R** (WA-SMSI M1.6, bottom closed, 5 mm) | 10 | |
| A12 | Screws | M1.6 x 10 (through PCB + 3.5 mm spacer + plate) and M2 x 16 for the printed test stack | 10 each | |

## B. Prototype PCBs: 1 column + left (hand soldering or JLC assembly)
| # | Part | Value / type | Footprint | Column | Left | Total (+ spare) |
|---|---|---|---|---|---|---|
| B1 | Shift register in | 74HC165 | SOIC-16 | 3 | 3 | 6 (+2) |
| B2 | Shift register out | 74HC595 | SOIC-16 | – | 2 | 2 (+1) |
| B3 | Buffer (LED data) | 74AHCT1G125 | SOT-23-5 | – | 1 | 1 (+2) |
| B4 | LDO 3.3 V | AP2112K-3.3 | SOT-23-5 | – | 1 | 1 (+2) |
| B5 | Reset supervisor | MCP100T-300 (Microchip, 3.0 V) | SOT-23 | – | 1 | 1 (+2) |
| B6 | Resistor | 10k 1 % | 0603 | 25 | 25 | 50 (+20) |
| B7 | Resistor | 680 R | 0603 | – | 14 | 14 (+10) |
| B8 | Resistor | 33 R | 0603 | – | 1 | 1 (+5) |
| B9 | Capacitor | 100 nF X7R 16 V | 0603 | 15 | 12 | 27 (+20) |
| B10 | Capacitor | 1 uF X7R 16 V | 0603 | – | 2 | 2 (+5) |
| B11 | Capacitor | 10 uF X5R 10 V | 0805 | 2 | 1 | 3 (+5) |
| B12 | Capacitor | 22 uF X5R 10 V | 0805 | 1 | – | 1 (+5) |
| B13 | Electrolytic | 220 uF 10 V, 6.3 x 7.7 mm SMD | CP_Elec_6.3x7.7 | – | 1 | 1 (+1) |
| B14 | RGB LED | SK6812MINI-E | reverse mount | 12 | 5 | 17 (+5) |
| B15 | Key switch MX | chosen type from A1 | MX 5-pin | 12 | 5 | 17 (+3) |
| B16 | Key cap 18 mm | chosen type from A2 | – | 12 | 5 | 17 |
| B17 | Small key | 3FTH965 / 3FTH940 / 3FTH980 | own footprint | – | 8 / 4 / 1 | 13 |
| B18 | Cap 1T frosted white | APEM 1T | – | – | 13 | 13 |
| B19 | Encoder | EC11E with push, H20 | | 4 | 1 | 5 |
| B20 | LED 3 mm red (beat) + LED spacer 5 mm | | LED_D3.0mm | – | 1 | 1 |
| B21 | Box header 2x5, 2.54 mm, vertical (shrouded) | | IDC 2x5 | 4 | 4 | 8 |
| B22 | Box header 2x8, 2.54 mm, vertical (core J15A) | | IDC 2x8 | – | 1 | 1 |
| B23 | Pin header 1x3, 2.54 mm | | 1x03 | 2 | 1 | 3 |
| B24 | JST VH 2-pin header B2P-VH + housing VHR-2N + crimps | 5 V LED/OLED supply | VH 1x02 | – | 1 | 1 |
| B25 | Ribbon cable 10-way + IDC sockets 2x5 (and 1x 16-way + 2x8 sockets for J15A) | | | | | ~1 m, 10 + 2 sockets |
| B26 | Spacer for the Newhaven pins | 2.5 mm plastic pin spacer, 1x20 | | 1 | – | 1 |
| B27 | 5 V supply for LEDs + OLED (test) | 5 V >= 2 A (full panel later >= 4 A) | | | | 1 |

Notes:
- Column CS jumper: close exactly one of JP1..JP4 (solder bridge) per board position.
- Newhaven: move the 0 R on the display from **R14 to R15** (jumper option #1) before connecting it.
- Core: **J15_S jumper on 3.3 V**.
- JLC assembly: all SMD parts of B1-B14 are standard parts; THT parts (switches, encoders, headers) by hand.

## Order numbers (checked 2026-11-23)
DigiKey's BOM Manager / "Bulk add" accepts the manufacturer part numbers (MPN) below directly.

| Item | MPN | DigiKey | Remark |
|---|---|---|---|
| OLED | Newhaven NHD-3.12-25664UCY2 | NHD-3.12-25664UCY2-ND | in stock |
| RGB LED (10 pcs) | Adafruit 4960 (SK6812MINI-E reverse mount) | 1528-4960-ND | in stock |
| Encoder | Alps EC11E15244G1 (vertical, 20 mm flat shaft, 30 detents, push) | 4809-EC11E15244G1-ND | DigiKey 0 in stock (20 wk); **TME in stock** |
| Standoff | Würth 97730506330R | (search the MPN; 3/4 mm siblings are 732-13681/13683) | |
| Small key | APEM 3FTH965 / 3FTH940 / 3FTH980 | not at DigiKey | **Mouser 642-3FTH965: not stocked, MOQ 100, 20 weeks** |
| Cap small key | APEM 1T16 (1T, frosted white) | 679-2121-ND | in stock |
| 74HC165 | Nexperia 74HC165D,653 | MPN | |
| 74HC595 | Nexperia 74HC595D,118 | MPN | |
| Buffer | TI SN74AHCT1G125DBVR | MPN | |
| LDO | Diodes AP2112K-3.3TRG1 | MPN | |
| Supervisor | Microchip MCP100T-300I/TT | MPN | |
| 10k / 680 R / 33 R 0603 | Yageo RC0603FR-0710KL / RC0603FR-07680RL / RC0603FR-0733RL | MPN | |
| 100 nF 0603 | Samsung CL10B104KB8NNNC | MPN | |
| 1 uF 0603 | Samsung CL10A105KB8NNNC | MPN | |
| 10 uF 0805 | Samsung CL21A106KAYNNNE | MPN | |
| 22 uF 0805 | Samsung CL21A226MAQNNNE | MPN | |
| 220 uF 10 V 6.3x7.7 | Panasonic EEE-FK1A221P | MPN | |
| Box header 2x5 / 2x8 | Würth 61201021621 / 61201621621 | MPN | |
| Pin header 1x3 | Würth 61300311121 | MPN | |
| JST VH | B2P-VH(LF)(SN) + VHR-2N + SVH-21T-P1.1 | MPN | |

## Small keys: plain 3FTH9 + inserted LED (decided 2026-11-23)
The factory-lit 3FTH965/940/980 are not stocked anywhere (Mouser 642-3FTH965: MOQ 100, 20 weeks). midiphy uses the plain
**3FTH9** with a standard **2x3x4 mm rectangular LED** pushed into the switch before soldering (long leg into the round
hole; check the switch still clicks). The PCB footprint stays the same (LED holes of the illuminated 3F layout) – **VERIFY**
with the test print that the legs of the inserted LED come out at those two holes.
LEDs: warm white 2x3x4 mm clear, e.g. Component Shop UK RL234U-WW; yellow and red 2x3x4 mm are standard parts (eBay/
AliExpress/Component Shop). With 680 R: white ~2.7 mA, red ~4.4 mA – match brightness by changing the resistor later.

## Where to order (snapshot 2026-11-23, check live stock)
| Part | DigiKey | Mouser | TME (PL, EU) | LCSC (CN) |
|---|---|---|---|---|
| OLED NHD-3.12-25664UCY2 | yes, ~$38 | yes, 234 pcs, $42.59 | not found | not found |
| Encoder EC11E15244G1 | listed (stock varies, 0-491) | yes, 753 pcs, $5.02 | yes, ~1291 pcs | yes, ~1676 pcs, ~$1.31 |
| SK6812MINI-E | Adafruit 4960 10-pack (1528-4960-ND) ~$0.30/LED | not found | not found | **C5149201**, ~$0.05-0.08/LED |
| APEM 3FTH9 | yes (large stock) | yes, ~1467 pcs, $3.41 | not found | not found |
| APEM 1T16 cap | **679-2121-ND**, ~439 pcs, ~$0.88 | not confirmed | listed, stock unknown | – |
| Würth 97730506330R | yes, ~$0.88 | not found | not found | – (Bürklin 18H8953, €0.98) |
| ICs / passives / headers | yes | yes | yes | yes, cheapest |
| 2x3x4 mm LEDs | – | – | – | generic (also eBay / Component Shop UK) |

**Advice**
- **Test order (samples): DigiKey** – has the most parts in one order (OLED, 3FTH9, SK6812MINI-E, Würth standoff, all
  ICs/passives, EC11E when in stock). Shipping to NL is free above a minimum order value, prices include import duties.
- **Full build: LCSC + JLCPCB** for the PCBs, SMD assembly and the cheap parts (SK6812MINI-E ~6x cheaper, EC11E ~4x),
  DigiKey or Mouser only for the OLEDs, 3FTH9 and the Würth standoffs.
- **1T16 cap: DigiKey 679-2121-ND** (in stock; Newark/Sager only MOQ 100). Fallbacks that fit the 3F, also frosted
  white at DigiKey: 1U16 (round 10.6 mm, 679-2125-ND) or 1KS1116 (square 14.3 mm, 679-3950-ND) – both would need a
  different panel hole.
