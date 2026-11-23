# Hardware – front panel PCBs (design, step 1: block diagram and connections)

Status: **design, nothing ordered or routed yet.** Every connection below is checked against a named source;
items marked **VERIFY** still have to be checked before the schematic is final.

## Principle (as Wilba / MIDIbox)
Wilba's MB-SEQ front panel (hwcfg `apps/sequencers/midibox_seq_v4/hwcfg/wilba/MBSEQ_HW.V4`) uses the MIDIbox SRIO chain:
one 8x8 button matrix on 1 DIN register, one 8x8 LED matrix on 2 DOUT registers (select + LEDs), encoders directly on
DIN registers (2 pins each, `DETENTED3`), 23 shift registers in one chain on J8/J9 of the core. The LCDs hang on J15.

We keep the same building blocks (74HC165 DIN / 74HC595 DOUT on J8/J9, the same `MBSEQ_HW.V4` mechanism, displays on
J15), with two differences, both for modularity:
- **No button matrix across modules.** Each module has its own 74HC165 registers in the chain (like MBHP_DINX4): no
  diodes, no ghosting, identical modules. (Wilba's matrix saves chips but needs all 8 select lines on every board.)
- **Big-key LEDs are SK6812 (WS2812 compatible)** on one data line instead of an LED matrix (MIOS32 `modules/ws2812`,
  TIM4 + DMA on **PB6 = J4B.SC**; used in NG, CV v2, SEQ v4 Lite – not yet in the SEQ v4 firmware).

## Modules
| PCB | Qty | Content | DIN inputs | 
|---|---|---|---|
| Column PCB (~91 x 110 mm) | 4 (identical) | 1 OLED display (Newhaven NHD-3.12-25664UC: display glass on its own small PCB, plugged in with its 1x20 header and screwed on with its 4 M2.5 holes – one part, no extra board), 4 EC11E with push, 12 MX keys (GP, select, function row), 12 SK6812MINI-E | 4 x 2 (A/B) + 4 push + 12 keys = 24 -> **3 x 74HC165** |
| Left PCB | 1 | datawheel EC11E with push, 13 MEC 3FTH9 small keys (one-colour 3 mm LED), 5 MX transport keys + 5 SK6812MINI-E, beat LED | 2 + 1 + 13 + 5 = 21 -> 3 x 74HC165; LEDs 13 + 1 -> 2 x 74HC595 |
| Switch plate | 1 | 1.5 mm FR4 or aluminium, 14 x 14 mm MX cut-outs, holds the switches and the encoders' alignment | – |
| Front plate | 1 | aluminium 2-3 mm, one cut-out per key row, holes for small keys / encoders / OLED windows | – |

Totals: 15 DIN + 2 DOUT registers (SEQ limit `SRIO_NUM_SR` 23 – OK). 53 RGB LEDs (48 + 5) on one WS2812 line.

## Connections (verified)
**Core J8/J9 (SRIO, 2x5 box header)** – source: `ucapps.de/mbhp/mbhp_core_stm32f4.pdf`, drawing of J8/9:
| Row | left column = **J9 (DIN chain)** | right column = **J8 (DOUT chain)** |
|---|---|---|
| 1 | Vs (GND) | Vs (GND) |
| 2 | Vd (+5 V) | Vd (+5 V) |
| 3 | **SI** (serial in from the DIN chain = QH of the last 74HC165) | **SO** (serial out to DS of the first 74HC595) |
| 4 | SC (shift clock) | SC (shift clock) |
| 5 | **RC2** (load of the 74HC165s) | **RC1** (latch of the 74HC595s) |
5 V level. **VERIFY**: which corner is pin 1 on the core PCB (silkscreen) before drawing the connector footprint.
Chaining as MBHP_DINX4 (`ucapps.de/mbhp/mbhp_dinx4.pdf`): QH of each 74HC165 to SER of the previous one, 10 k pull-ups on
all inputs, 100 nF per IC.

**OLEDs on J15** – source: `modules/app_lcd/ssd1322/app_lcd.c`, `mios32/STM32F4xx/mios32_board.c`:
| Signal | Core | Remark |
|---|---|---|
| SCLK | J15 E1 (PC8) and E2 (PC9) | `J15_PIN_SERLCD_SCLK` drives both |
| SDIN | J15 RW (PC11) | `J15_PIN_SERLCD_DATAOUT` |
| D/C | J15 RS (PA8) | `MIOS32_BOARD_J15_RS_Set` |
| CS0..CS3 | J15 D0..D3 | outputs of a 74HC595 on the core (RW also drives its OE#) |
3.3 V push-pull (`APP_LCD_OUTPUT_MODE 0`). Firmware: the driver currently always selects CS0 (`u8 cs=0; // TODO`) –
has to be extended for 4 displays. **VERIFY**: SSD1322 serial mode pins on the Newhaven module (BS0/BS1 jumpers for
4-wire SPI), reset line.

**SK6812 chain** – source: `modules/ws2812/ws2812.c`: data from **PB6 (J4B.SC)**, 800 kHz. 3.3 V data into a 5 V LED
(VIH 0.7 x VDD = 3.5 V): use a **74AHCT125 level shifter** at the start of the chain.
**SK6812MINI-E pinout (datasheet REV02, Adafruit 4960): 1 = VDD, 2 = DOUT, 3 = GND, 4 = DIN.** The KiCad symbol
`SK6812MINI-E` inherits the older SK6812 numbering (1 VSS, 2 DIN, 3 VDD, 4 DOUT) – **do not use it**; pads are wired
from the datasheet. **VERIFY** pad positions of the reverse-mount footprint against the datasheet drawing (mirrored).

**OLED (Newhaven NHD-3.12-25664UCY2, yellow) in 4-wire SPI** – source: datasheet rev. 7, p. 4-8: BS1 = BS0 = GND
(pins 19/20); 7 = SCLK (D0), 8 = SDIN (D1), 9 (D2) open, 4 = D/C, 16 = /RES, 17 = /CS, 5-6 and 10-14 GND, 2 = VDD 3.3 V,
1 = VSS, 3/15/18 not connected.

**Encoders** – EC11E footprint `RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm`: pads A, B, C (common), S1/S2 (push),
MP (mounting). As Wilba: A and B on two DIN inputs, C to GND, push on a third DIN input.

## Power
53 SK6812 at full white ~3.2 A – limit brightness in firmware and/or feed the LED rail from a separate 5 V supply, not
through the ribbon cables. OLEDs ~3.3 V, Newhaven module current per datasheet (**VERIFY**).

## Mechanics
- **Stack (bottom to top):** key PCB (switches, LEDs, encoders, ICs) -> **switch plate** 5 mm above it (plain FR4 1.5/1.6 mm,
  not visible, holds the MX switches) -> **front panel** = printed FR4 PCB, matte black solder mask with white silkscreen
  legends (printed, not engraved), at about the height of the top of the switch housings -> caps.
  **Only the caps come through the front panel**: one cut-out per key row (caps move down into it when pressed), windows for
  the OLEDs, holes for encoder shafts and the small MEC caps. **VERIFY** with the chosen switch: housing height above the plate
  (~6.6 mm for Cherry MX), cap clearance over the 4 mm travel, encoder shaft length through the panel + knob, OLED glass
  height behind the window.
- **Fastening without visible screws:** SMD threaded standoffs (e.g. Würth SMSN, M2.5/M3) reflowed on the back of the front
  panel PCB, spacers down to the switch plate and the key PCB, screws from below; the case is fixed the same way. Spread around each screen group
  and between the rows. Encoders without a nut on the front.
- **No visible fasteners on the front.** The OLED displays (Newhaven, glass on its own small PCB) are plugged into and screwed to their column PCB; switches clip into the switch
  plate; encoders are held by the PCB/plate; the front plate is fixed from behind (press-in threaded studs / bushings).
- The rigidity comes from the switch plate + front plate; every module PCB is fixed with >= 4 standoffs.
- Board interconnect: keyed, latching ribbon (IDC) for SRIO + OLED bus; separate wires for the LED 5 V rail.

## Next steps
1. Schematic of the column PCB in KiCad (symbols with the checked pinouts), ERC.
2. Placement on the panel grid (91 mm groups, 19.05 mm keys), board outline, mounting holes; render and compare with the
   virtual panel.
3. Routing (Freerouting), DRC, review against this list; then the left PCB; Gerbers/BOM/CPL.

## Column PCB – status (step 2: parts, connections, placement)
`column/build_column.py` builds `column/column.kicad_pcb` (KiCad 10, Python API): 93 footprints, 58 nets, placement on the
panel grid (keys 19.05 mm, rows 24 mm, OLED header and M2.5 holes per the Newhaven drawing, M3 holes for the panel stack).
`column/verify_column.py` reads the saved board back and checks **every pad** against the expected nets plus independent
datasheet rules (74HC165 power/control, SK6812MINI-E VDD/GND, Newhaven serial pins, core J8/J9 pinout), the unbroken LED
chain and the DIN chain -> **293 pads + 72 rules OK**. KiCad DRC: no errors except 12 intended courtyard overlaps
(LED under its switch). Not routed yet (235 unconnected items).
Findings while checking: the KiCad symbol `SK6812MINI-E` has the wrong pin numbering for this part (we wire per datasheet);
KiCad's reverse-mount footprint has only 0.17 mm pad-to-cut-out -> own footprint `lib/seq4oled.pretty/LED_SK6812MINI-E_ReverseMount_Cut3.5x3.1`
(0.30 mm; **VERIFY** the manufacturer's inner router radius <= 0.5 mm). Renders: `column/render_top.png`, `column/render_bottom.png`.
Input mapping (for MBSEQ_HW.V4): U1 D0-3 GP keys 1-4, D4-7 select keys 1-4; U2 D0-3 function keys 1-4, D4-7 encoder pushes 1-4;
U3 D0/D1 encoder 1 A/B ... D6/D7 encoder 4 A/B. CS select: solder one of JP1..JP4 per board position.
