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
| Left PCB (100.3 x 126 mm) | 1 | datawheel EC11E with push, 13 MEC 3FTH9 small keys (LED fitted by APEM), 5 MX transport keys + 5 SK6812MINI-E, beat LED; interface to the core (SRIO, J15A, J4B) and the 5 V LED/OLED supply input | 2 + 1 + 13 + 5 = 21 -> 3 x 74HC165; LEDs 13 + 1 -> 2 x 74HC595 |
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

## Grounding (no ground loops)
- **On the column PCB:** one GND net, poured on both layers (F.Cu + B.Cu, 0.3 mm clearance, islands removed), tied together
  by a 7.5 mm grid of stitching vias plus extra vias in every plane piece that had fewer than 2 links (64 stitching vias).
  Pieces that still hang on one point are small slivers between tracks. Connector and encoder GND pins have a full
  (not thermal) connection, so return current between boards sees low impedance. A plane is one node, so a loop *on* the board does not exist.
- **Between boards:** star from the core, no ring. Each module gets GND only through its own SRIO cable (2 GND pins),
  the OLED cable (GND next to every clock) and the LED cable (GND next to VLED, as a twisted pair from the LED supply). The
  module-to-module chaining cables (J2 -> next J1) also carry GND: that is a loop between modules, but its area is the
  cable itself (GND runs right next to the signals), so no field can couple into it – this is the normal MIDIbox DINX4 practice.
  **Do not** add extra ground wires between modules or to the case beyond the cables.
- **LED supply:** separate 5 V for the SK6812 rail; its GND joins the logic GND at **one point** (the core / power input), so
  the ~3 A LED current does not flow through the SRIO ribbons.
- **Case:** FR4 front panel and switch plate are insulators (their standoffs tie nothing). With a metal case: connect the case to
  GND at one point only, at the power inlet. MIDI IN is opto-isolated (no ground connection to other gear); USB brings the
  computer's ground – if hum appears through audio gear, use a USB isolator, not a ground lift.

## Left PCB – status (built, routed, verified)
`left/build_left.py` -> `left/left.kicad_pcb`, routed with `./route.sh left` (same pipeline as the column: build -> Specctra ->
Freerouting -> `route_board.py` (GND planes, stitching, no one-layer vias) -> `verify_left.py` -> DRC).
Result: **366 pads + 140 datasheet rules OK**, DRC 0 unconnected, only the 5 intended LED-under-switch courtyard overlaps.
- **Small keys: APEM Multimec 3FTH9xx** – the LED is fitted by APEM, the colour is in the part number (65 white, 40 yellow
  for the amber keys, 80 red for Rec.). Own footprint `lib/seq4oled.pretty/SW_MEC_3FTH9_LED` from the datasheet
  (MEC-MULTIMEC3F-2203 p. 3, "TH illuminated"): contact 1/2 to 3/4 on a 10.16 x 7.62 mm grid, LED K in the left column 2.54 mm
  below pin 2, LED A in the right column 2.54 mm above pin 3. **VERIFY** the LED pin positions with a real switch (read from
  the drawing, no table) and the PCB thickness (datasheet 1.4 +- 0.2 mm -> order 1.6 mm).
- **DIN (first in the chain, SR 1-3)**: U1 D0-D7 = Step, Track, Param, Trigger, Instr., Mute, Phrase, Bookm.; U2 = Solo, Metr.,
  Loop, Rec., Live, Rew, Fwd, Stop; U3 = datawheel A/B (D0/D1, same register as MIOS32 requires), datawheel push, Play,
  Pause, 3 spare (pulled up). Column modules follow as SR 4-6, 7-9, 10-12, 13-15.
- **DOUT (only here, SR 1-2)**: U4 QA-QH = LEDs Step ... Bookm.; U5 QA-QF = Solo, Metr., Loop, Rec., Live, Beat. As MBHP_DOUTX4:
  output -> 680 R -> LED -> GND (~4 mA, 14 LEDs on 2 registers).
- **Core cables**: J1 = core J8/J9 (2x5), J3 = core **J15A** (2x8: 1 Vs, 2 Vd, 3 V0, 4 RS, 5 RW, 6 E, 7-14 D0-D7, 15/16 B+/B-,
  from the MBHP_CORE_STM32F4 schematic) – only RS/RW/E/D0-D3 and Vs are used, **Vd is not connected**; J5 = core **J4B**
  (2x5: pin 6 = SC = PB6, odd pins + 2/4 = Vs) for the RGB LED data.
- **OLED reset**: MCP100-300 supervisor (SOT-23: 1 /RST, 2 VDD, 3 VSS, Microchip DS11187F) holds /RES of all 4 displays low
  until 3.3 V is stable + 350 ms (J15 has no reset line). **VERIFY** SOT-23 pin 1 = /RST against the current datasheet.
- **LED data**: 74AHCT1G125 buffer (TTL input takes the open-drain PB6 with the core's 2.2 k pull-up to 5 V, `modules/ws2812`),
  33 R series resistor, then D1-D5 and on to column 1 (J6 -> column J5).

## Power (revised after reading the Newhaven datasheet, p. 6/7)
- **Newhaven in jumper option #1**: on every display move the 0 R from **R14 to R15**. Then the boost converter runs from
  BC_VDD (pin 3) = 5 V: **150 mA typ / 170 mA max at 100 % on**, and VDD (pin 2) 3.3 V only feeds the logic (200 uA).
  Default jumper would need 310/340 mA *from 3.3 V* per display (1.4 A for four) – too much for the core.
  The column PCB connects pin 3 to VLED; with the default jumper pin 3 is open on the module, so nothing breaks.
- **One 5 V supply for LEDs + OLED boost** (J7 on the left PCB, JST VH): 58 SK6812 + 4 x 170 mA; >= 4 A PSU, brightness
  limited in firmware. Its GND meets the logic GND only on the left PCB (single point, see Grounding).
- **OLED logic 3.3 V**: AP2112K-3.3 on the left PCB from the core's 5 V (a few mA).
- **Core J15_S jumper must be set to 3.3 V**: the core's 74HC595 that drives CS0-3 is powered from it; at 5 V the CS lines
  would exceed the SSD1322's VIH max (= VDD).

## Firmware for the new hardware (built, host-tested; not yet on real OLED/RGB hardware)
- **Hardware config**: `hardware/MBSEQ_HW.V4`, generated by `hardware/make_hwcfg.py` from the PCB mapping (every DIN pin used
  once, checked by the script). Copy it to the SD card only when the new front panel is connected – on the current Wilba
  hardware it would switch J15 to the OLEDs and remap all buttons.
  New keywords: `OLED_SSD1322 <0..4>`, `OLED_CONTRAST <0..255>`, `RGBLED_ENABLED <0/1>`, `RGBLED_BRIGHTNESS <1..255>`.
- **OLEDs** (`core/seq_oled_hw.c`): the same 64x64 cells that go to the virtual panel are written to the 4 SSD1322 (column
  window of 16 x 4 pixels per cell, CS0-3 on J15 D0-D3). Own bit-bang at ~8 MHz (SSD1322 SCLK cycle >= 100 ns) over the
  ribbon cables. With `OLED_SSD1322` set, the character LCD output to J15 (`SEQ_LCD_Update`, special chars, logo) is off.
  Host test `tools/oled_hw_test/run.sh`: an SSD1322 emulator receives the SPI bytes and checks init + all 16 cells -> OK.
  Init sequence = Newhaven's NHD-3.12 example values (**VERIFY** on the first display: nibble order (remap 0x14), contrast).
- **RGB key LEDs** (`core/seq_rgbled.c`, `modules/ws2812`, PB6 = J4B.SC): the SEQ keeps driving its LEDs as before; in the
  hwcfg the big-key LEDs sit on DOUT registers 3-9 that are not fitted, and seq_rgbled reads them back every 20 ms and
  colours the chain: GP amber (play position warm white), select keys amber/warm white as the virtual panel, function row
  amber, Play sage green, other transport keys warm white; brightness from the hwcfg (default 64/255 = 25 %).
- **Note on J15**: the core's J15 RW line is also OE# of its 74HC595 that drives CS0-3, so the CS lines float while a "1"
  bit is clocked. The OLED bus keeps CS0-3 away from SCLK in the ribbon (pins 7-10 vs. 3) so they do not pick up the clock.

## Front panel + switch plate (built, verified against the PCBs)
`panel/build_panel.py` -> `panel/front.kicad_pcb` (474.7 x 144.4 mm, = the virtual panel) and `panel/plate.kicad_pcb`, both
from the panel_oled.html geometry. `panel/verify_panel.py` checks them **independently against the routed PCBs**: 53 MX keys
centred in 14.0 mm plate cut-outs with >= 0.9 mm free around every 18 mm cap in the front cut-outs, 13 small keys centred in
11.0 x 11.0 mm front holes (APEM cut-out for the 1T cap), 17 encoder shafts centred in their holes, 4 OLED active areas
inside their windows, 22 PCB stack holes with a plate hole and a front standoff above them, every tall top-side part under a
plate opening, every standoff >= 1 mm from cut-outs/holes/edge -> **OK**. DRC of both plates clean.
- **Front panel**: FR4 1.6 mm, **black solder mask, white silkscreen** (printed legends: select-mode/left-block keys, function
  row, transport symbols, DATA, BEAT, logo; no GP/select labels). OLED windows 80.0 x 21.0 mm (bezel opening 82 x 22).
- **No visible fasteners**: 30 Würth WA-SMSI **M1.6 bottom-closed** SMT spacers 97730506330R (L 5 mm, OD 3.3 mm, Würth only
  makes the closed version in M1.6; the M3 versions have a through hole or a locating hole) reflowed on the back.
  22 of them sit above the PCB stack holes: **one M1.6 screw per point from the back of the PCB** through the PCB, a 3.5 mm
  spacer tube, the switch plate, into the standoff – clamps PCB, plate and front panel at once. 8 more in the free margins
  (screwed from below the plate). PCB stack holes are unplated 2.2 mm with no copper within 2.5 mm: the screws never touch
  GND, so the mechanics add no ground connection between boards.
- **Switch plate**: FR4 1.5 mm, MX 14.0 x 14.0, 3F 10.8 x 10.8, one opening per column for the Newhaven module + encoders,
  datawheel 13 x 14, beat LED 3.6, 2.2 mm stack holes.

### Stack heights (z = 0 at the top of the key PCBs)
| Item | z (mm) | Source | Sticks out above the panel |
|---|---|---|---|
| Switch plate | 3.5 – 5.0 | MX plate mount: plate top 5.0 mm above the PCB (Cherry MX drawing) | – |
| Front panel | 10.0 – 11.6 | plate top + 5.0 mm standoff (97730506330R) + 1.6 mm FR4 | – |
| 3F + 1T cap (small keys) | top 14.9, travel 1 | APEM: 1T h = 14.9 mm, 3F travel 1 mm | **3.3 mm** (pressed 2.3) |
| MX housing | top 11.6 | plate + 6.6 mm (Cherry MX) | flush with the panel top |
| MX + 18 mm cap (big keys) | top ~19-20, travel 4 | cap height depends on the profile – **VERIFY with the sample caps** | ~8 mm (pressed ~4) |
| Newhaven glass | 2.5 – 8.5 | module 6.0 mm thick (datasheet), soldered on its pins over a 2.5 mm spacer | 1.5 mm below the panel back |
| EC11E shaft (H20) | top 20.0 | footprint / part H = 20 mm | 8.4 mm shaft for the knob |
| Beat LED 3 mm | top ~10.3 | 5 mm LED spacer + 5.3 mm LED body – **VERIFY** (or a light pipe) | in the 3.2 mm hole |
Consequences already applied: the Newhaven is soldered directly (a 1x20 socket would put the glass at 14.5 mm, above the
panel), the 220 uF capacitor of the left PCB is on the back (7.7 mm tall). If the big caps stick out too far with the chosen
profile, use a lower profile (DSA/flat) rather than raising the panel – the small keys only have 3.3 mm.
