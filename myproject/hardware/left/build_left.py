# Left PCB (left of the 4 column PCBs): datawheel, beat LED, 13 small keys (MEC 3FTH9 with LED), 5 MX transport keys with
# SK6812MINI-E, and the interface to the core: SRIO (J8/J9), OLED bus (J15A), LED data (J4B.SC = PB6), LED/OLED 5 V supply.
# It is the first board in the DIN chain (SR 1-3) and holds the only DOUT registers (SR 1-2).
# Run with KiCad's Python (see ../route.sh). Every pin map: see ../README.md (sources named there).
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from boardlib import Board

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "left.kicad_pcb")
W, H = 100.3, 126.0              # right edge 0.4 mm from column 1; same height/top as the column PCBs
KX = [13.0 + 19.05 * k for k in range(5)]            # 5 key columns on the 19.05 mm grid (panel LX(k))
Y_S = [59.36, 75.95, 92.54]                          # small key rows (panel SROW + 29 px)
Y_T = 116.04                                         # transport row = function row of the columns
DW = (52.02, 20.20)                                  # datawheel shaft (panel 178/76 px)
BEAT = (81.81, 19.90)                                # beat LED (panel 275/75 px)

B = Board(W, H)
P = B.place

# ---- small keys: MEC 3FTH9xx with LED. pads 1/2 = contact A (-> DIN input), 3/4 = contact B (GND), 5 = LED K, 6 = LED A ----
small = [["STEP", "TRACK", "PARAM", "TRIGGER", "INSTR"], ["MUTE", "PHRASE", "BOOKM", "SOLO", "METRO"], ["LOOP", "REC", "LIVE"]]
colour = {"SOLO": "3FTH940", "METRO": "3FTH940", "LOOP": "3FTH940", "LIVE": "3FTH940", "REC": "3FTH980"}   # amber ~ yellow 40, red 80, else white 65
k = 0
for r, row in enumerate(small):
    for c, name in enumerate(row):
        k += 1
        P("seq4oled", "SW_MEC_3FTH9_LED", "S%d" % k, KX[c], Y_S[r],
          padnets={"1": "KEY_" + name, "2": "KEY_" + name, "3": "GND", "4": "GND", "5": "GND", "6": "LEDA_" + name},
          value=colour.get(name, "3FTH965"))

# ---- transport keys: MX + SK6812MINI-E (1 VDD, 2 DOUT, 3 GND, 4 DIN - datasheet REV02), chain LED_IN -> D1..D5 -> LED_OUT ----
trans = ["REW", "FWD", "STOP", "PLAY", "PAUSE"]
for c, name in enumerate(trans):
    P("Button_Switch_Keyboard", "SW_Cherry_MX_1.00u_PCB", "SW%d" % (c + 1), KX[c] + 2.54, Y_T - 5.08,
      padnets={"1": "KEY_" + name, "2": "GND"}, value="KEY_" + name)
    din = "LED_IN" if c == 0 else "LED_D%d" % c
    dout = "LED_OUT" if c == 4 else "LED_D%d" % (c + 1)
    P("seq4oled", "LED_SK6812MINI-E_ReverseMount_Cut3.5x3.1", "D%d" % (c + 1), KX[c], Y_T + 5.08,
      padnets={"1": "VLED", "2": dout, "3": "GND", "4": din}, value="SK6812MINI-E")
    P("Capacitor_SMD", "C_0603_1608Metric", "CL%d" % (c + 1), KX[c] + 9.525, Y_T + 5.08, rot=90,
      padnets={"1": "VLED", "2": "GND"}, value="100nF")
    B.keepout(KX[c], Y_T + 5.08, 2.05, 1.85)

# ---- datawheel (EC11E with push; origin = pad A, shaft at +7.5/+2.5) and beat LED (3 mm, pad 1 = K) ----
P("Rotary_Encoder", "RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm", "EN1", DW[0] - 7.5, DW[1] - 2.5,
  padnets={"A": "DW_A", "B": "DW_B", "C": "GND", "S1": "DW_P", "S2": "GND", "MP": "GND"}, value="EC11E")
P("LED_THT", "LED_D3.0mm", "D6", BEAT[0] - 1.27, BEAT[1], padnets={"1": "GND", "2": "LEDA_BEAT"}, value="3mm red")

# ---- DIN: 3 x 74HC165 (1 /PL, 2 CP, 3-6 D4-D7, 7 /Q7, 8 GND, 9 Q7, 10 DS, 11-14 D0-D3, 15 /CE, 16 VCC) ----
# encoder A/B on D0/D1 of one register (MIOS32 needs both pins of an encoder in the same SR, pin pair 0/1)
din = {
    "U1": ["KEY_STEP", "KEY_TRACK", "KEY_PARAM", "KEY_TRIGGER", "KEY_INSTR", "KEY_MUTE", "KEY_PHRASE", "KEY_BOOKM"],
    "U2": ["KEY_SOLO", "KEY_METRO", "KEY_LOOP", "KEY_REC", "KEY_LIVE", "KEY_REW", "KEY_FWD", "KEY_STOP"],
    "U3": ["DW_A", "DW_B", "DW_P", "KEY_PLAY", "KEY_PAUSE", "DIN_SPARE1", "DIN_SPARE2", "DIN_SPARE3"],
}
chain = {"U1": ("SI_UP", "SR_12"), "U2": ("SR_12", "SR_23"), "U3": ("SR_23", "SI_DN")}      # (Q7, DS)
ux = {"U1": 16.5, "U2": 26.0, "U3": 35.5}
r = 0
for u, d in din.items():
    q7, ds = chain[u]
    P("Package_SO", "SOIC-16_3.9x9.9mm_P1.27mm", u, ux[u], 12.5, value="74HC165",
      padnets={"1": "RC2", "2": "SC", "3": d[4], "4": d[5], "5": d[6], "6": d[7], "8": "GND", "9": q7, "10": ds,
               "11": d[0], "12": d[1], "13": d[2], "14": d[3], "15": "GND", "16": "+5V"})
    P("Capacitor_SMD", "C_0603_1608Metric", "C" + u[1], ux[u], 20.0, padnets={"1": "+5V", "2": "GND"}, value="100nF")
    for i, sig in enumerate(d):                      # 10k pull-up on every input (MBHP_DINX4)
        r += 1
        P("Resistor_SMD", "R_0603_1608Metric", "R%d" % r, ux[u] - 3.9 + (i % 4) * 2.6, 25.5 + (i // 4) * 4.0, rot=90,
          padnets={"1": "+5V", "2": sig}, value="10k")

# ---- DOUT: 2 x 74HC595 (1 QB, 2-7 QC-QH, 8 GND, 9 QH', 10 /SRCLR, 11 SRCLK, 12 RCLK, 13 /OE, 14 SER, 15 QA, 16 VCC) ----
# LEDs as MBHP_DOUTX4: output -> resistor -> LED anode, cathode to GND (output high = LED on)
dout = {
    "U4": ["STEP", "TRACK", "PARAM", "TRIGGER", "INSTR", "MUTE", "PHRASE", "BOOKM"],
    "U5": ["SOLO", "METRO", "LOOP", "REC", "LIVE", "BEAT", None, None],
}
dchain = {"U4": ("SO", "SR_45"), "U5": ("SR_45", "SO_DN")}                                    # (SER, QH')
qpin = ["15", "1", "2", "3", "4", "5", "6", "7"]                                               # QA..QH
ux.update({"U4": 66.0, "U5": 75.5})
for u, d in dout.items():
    ser, qh = dchain[u]
    pn = {"8": "GND", "9": qh, "10": "+5V", "11": "SC", "12": "RC1", "13": "GND", "14": ser, "16": "+5V"}
    for q, name in zip(qpin, d):
        if name:
            pn[q] = "LEDQ_" + name
    P("Package_SO", "SOIC-16_3.9x9.9mm_P1.27mm", u, ux[u], 12.5, padnets=pn, value="74HC595")
    P("Capacitor_SMD", "C_0603_1608Metric", "C" + u[1], ux[u], 20.0, padnets={"1": "+5V", "2": "GND"}, value="100nF")
    for i, name in enumerate(d):
        if name:
            r += 1
            P("Resistor_SMD", "R_0603_1608Metric", "R%d" % r, ux[u] - 3.9 + (i % 4) * 2.6, 25.5 + (i // 4) * 4.0, rot=90,
              padnets={"1": "LEDQ_" + name, "2": "LEDA_" + name}, value="680")       # ~4 mA: 14 LEDs on 2 registers
r += 1
P("Resistor_SMD", "R_0603_1608Metric", "R%d" % r, 88.0, 30.0, rot=90, padnets={"1": "+5V", "2": "SI_DN"}, value="10k")

# ---- power: OLED logic 3.3 V (LDO, ~1 mA with Newhaven jumper option #1), OLED reset, LED data buffer ----
# AP2112K-3.3 (SOT-23-5): 1 VIN, 2 GND, 3 EN, 4 NC, 5 VOUT
P("Package_TO_SOT_SMD", "SOT-23-5", "U8", 48.0, 38.0, padnets={"1": "+5V", "2": "GND", "3": "+5V", "5": "+3V3"}, value="AP2112K-3.3")
P("Capacitor_SMD", "C_0603_1608Metric", "C8", 44.5, 38.0, rot=90, padnets={"1": "+5V", "2": "GND"}, value="1uF")
P("Capacitor_SMD", "C_0603_1608Metric", "C9", 51.5, 38.0, rot=90, padnets={"1": "+3V3", "2": "GND"}, value="1uF")
# MCP100-300 (SOT-23-3): 1 /RST, 2 VDD, 3 VSS - push-pull, low until 3.3 V is stable + 350 ms: resets all 4 SSD1322
P("Package_TO_SOT_SMD", "SOT-23", "U7", 57.0, 38.0, padnets={"1": "OLED_RES", "2": "+3V3", "3": "GND"}, value="MCP100T-300")
P("Capacitor_SMD", "C_0603_1608Metric", "C7", 60.5, 38.0, rot=90, padnets={"1": "+3V3", "2": "GND"}, value="100nF")
# 74AHCT1G125 (SOT-23-5): 1 /OE, 2 A, 3 GND, 4 Y, 5 VCC - TTL input takes the open-drain PB6 signal (2.2k to 5 V on the core),
# drives the 5 V LED data line with clean edges; 33 R series resistor against ringing on the cable
P("Package_TO_SOT_SMD", "SOT-23-5", "U6", 68.0, 38.0, padnets={"1": "GND", "2": "LED_DATA_CORE", "3": "GND", "4": "LED_BUF", "5": "VLED"},
  value="74AHCT1G125")
P("Capacitor_SMD", "C_0603_1608Metric", "C6", 71.5, 38.0, rot=90, padnets={"1": "VLED", "2": "GND"}, value="100nF")
r += 1
P("Resistor_SMD", "R_0603_1608Metric", "R%d" % r, 64.5, 38.0, rot=90, padnets={"1": "LED_BUF", "2": "LED_IN"}, value="33")
P("Capacitor_SMD", "C_0805_2012Metric", "C10", 24.0, 36.0, rot=90, padnets={"1": "+5V", "2": "GND"}, value="10uF")
P("Capacitor_SMD", "CP_Elec_6.3x7.7", "C11", 40.0, 45.0, padnets={"1": "VLED", "2": "GND"}, value="220uF 10V")

# ---- connectors (bottom side; a flipped IDC header has pad 1 at the origin and its pins going up) ----
srio = lambda si, so: {"1": "GND", "2": "GND", "3": "+5V", "4": "+5V", "5": si, "6": so, "7": "SC", "8": "SC", "9": "RC2", "10": "RC1"}
P("Connector_IDC", "IDC-Header_2x05_P2.54mm_Vertical", "J1", 5.0, 48.0, padnets=srio("SI_UP", "SO"), back=True, value="CORE_J8J9")
P("Connector_IDC", "IDC-Header_2x05_P2.54mm_Vertical", "J2", 93.0, 22.0, padnets=srio("SI_DN", "SO_DN"), back=True, value="SRIO_TO_COLUMN")
# core J15A (LCD1, 2x8): 1 Vs, 2 Vd, 3 V0, 4 RS, 5 RW, 6 E, 7-14 D0-D7, 15/16 backlight. Vd is NOT used (OLED 3.3 V comes from U8).
P("Connector_IDC", "IDC-Header_2x08_P2.54mm_Vertical", "J3", 5.0, 24.0, back=True, value="CORE_J15A",
  padnets={"1": "GND", "4": "OLED_DC", "5": "OLED_SDIN", "6": "OLED_SCLK", "7": "CS0", "8": "CS1", "9": "CS2", "10": "CS3"})
obus = {"1": "GND", "2": "+3V3", "3": "OLED_SCLK", "4": "OLED_SDIN", "5": "OLED_DC", "6": "OLED_RES",
        "7": "CS0", "8": "CS1", "9": "CS2", "10": "CS3"}
P("Connector_IDC", "IDC-Header_2x05_P2.54mm_Vertical", "J4", 93.0, 48.0, padnets=obus, back=True, value="OLED_BUS_OUT")
# core J4B (2x5): 1-5, 7, 9 Vs; 6 SC = PB6 (WS2812 data); 8 SD; 10 Vd
P("Connector_IDC", "IDC-Header_2x05_P2.54mm_Vertical", "J5", 16.0, 48.0, back=True, value="CORE_J4B",
  padnets={"1": "GND", "2": "GND", "3": "GND", "4": "GND", "5": "GND", "6": "LED_DATA_CORE", "7": "GND", "9": "GND"})
P("Connector_PinHeader_2.54mm", "PinHeader_1x03_P2.54mm_Vertical", "J6", 78.0, 45.0, rot=90, back=True,
  padnets={"1": "VLED", "2": "LED_OUT", "3": "GND"}, value="LED_OUT")
# 5 V supply for LEDs + OLED boost converters (separate PSU, >= 4 A); its GND meets the logic GND here (single point)
P("Connector_JST", "JST_VH_B2P-VH_1x02_P3.96mm_Vertical", "J7", 28.0, 45.0, back=True, padnets={"1": "VLED", "2": "GND"}, value="5V_LED_IN")

# ---- mounting: M3 standoffs (front panel / switch plate stack) ----
for i, (x, y) in enumerate([(24.0, 3.5), (86.0, 3.5), ((KX[0] + KX[1]) / 2, 84.25), ((KX[3] + KX[4]) / 2, 84.25), (4.5, 103.5), (W - 4.5, 103.5)]):
    P("MountingHole", "MountingHole_3.2mm_M3", "H%d" % (i + 1), x, y)

B.gnd_fanout()
B.save(OUT)
