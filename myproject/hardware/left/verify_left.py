# Reads left.kicad_pcb back and checks every pad against expected_nets.json (from build_left.py) plus independent rules
# taken directly from the datasheets / core schematic (see ../README.md for the sources).
import json, os, sys, pcbnew
D = os.path.dirname(os.path.abspath(__file__))
b = pcbnew.LoadBoard(os.path.join(D, "left.kicad_pcb"))
exp = json.load(open(os.path.join(D, "expected_nets.json")))
got = {"%s.%s" % (f.GetReference(), p.GetNumber()): p.GetNetname() for f in b.GetFootprints() for p in f.Pads() if p.GetNumber()}
err = 0
for k, v in exp.items():
    if got.get(k) != v:
        print("MISMATCH", k, "expected", v, "got", got.get(k)); err += 1
rules = {}
for u in ("U1", "U2", "U3"):     # 74HC165 (NXP): 1 /PL, 2 CP, 8 GND, 15 /CE, 16 VCC
    rules.update({u + ".1": "RC2", u + ".2": "SC", u + ".8": "GND", u + ".15": "GND", u + ".16": "+5V"})
for u in ("U4", "U5"):           # 74HC595 (TI SCLS041): 8 GND, 10 /SRCLR high, 11 SRCLK, 12 RCLK, 13 /OE low, 16 VCC
    rules.update({u + ".8": "GND", u + ".10": "+5V", u + ".11": "SC", u + ".12": "RC1", u + ".13": "GND", u + ".16": "+5V"})
for i in range(1, 6):            # SK6812MINI-E REV02: 1 VDD, 3 GND
    rules.update({"D%d.1" % i: "VLED", "D%d.3" % i: "GND"})
for s in ("J1", "J2"):           # core J8/J9: 1/2 Vs, 3/4 Vd, 5 SI, 6 SO, 7/8 SC, 9 RC2, 10 RC1
    rules.update({s + ".1": "GND", s + ".2": "GND", s + ".3": "+5V", s + ".4": "+5V", s + ".7": "SC", s + ".8": "SC",
                  s + ".9": "RC2", s + ".10": "RC1"})
rules.update({"J1.5": "SI_UP", "J1.6": "SO", "J2.5": "SI_DN", "J2.6": "SO_DN"})
# core J15A: 1 Vs, 2 Vd (open on purpose), 3 V0, 4 RS = D/C, 5 RW = SDIN, 6 E = SCLK, 7-10 D0-D3 = CS0-CS3
rules.update({"J3.1": "GND", "J3.2": "", "J3.3": "", "J3.4": "OLED_DC", "J3.5": "OLED_SDIN", "J3.6": "OLED_SCLK",
              "J3.7": "CS0", "J3.8": "CS1", "J3.9": "CS2", "J3.10": "CS3", "J3.15": "", "J3.16": ""})
# core J4B: 6 SC = PB6, 10 Vd not used, odd pins + 2/4 Vs
rules.update({"J5.6": "LED_DATA_CORE", "J5.10": "", "J5.8": "", "J5.1": "GND", "J5.2": "GND"})
# OLED bus = column J3: 1 GND, 2 +3V3, 3 SCLK, 4 SDIN, 5 D/C, 6 /RES, 7-10 CS0-3
for n, v in enumerate(["GND", "+3V3", "OLED_SCLK", "OLED_SDIN", "OLED_DC", "OLED_RES", "CS0", "CS1", "CS2", "CS3"]):
    rules["J4.%d" % (n + 1)] = v
rules.update({"U8.1": "+5V", "U8.2": "GND", "U8.3": "+5V", "U8.5": "+3V3",             # AP2112K: 1 VIN 2 GND 3 EN 5 VOUT
              "U7.1": "OLED_RES", "U7.2": "+3V3", "U7.3": "GND",                       # MCP100 SOT-23: 1 RST 2 VDD 3 VSS
              "U6.1": "GND", "U6.2": "LED_DATA_CORE", "U6.3": "GND", "U6.5": "VLED",    # 74AHCT1G125: 1 /OE 2 A 3 GND 4 Y 5 VCC
              "J6.1": "VLED", "J6.2": "LED_OUT", "J6.3": "GND", "J7.1": "VLED", "J7.2": "GND", "D6.1": "GND"})
for i in range(1, 14):           # MEC 3FTH9: 3/4 contact to GND, 5 LED cathode GND
    rules.update({"S%d.3" % i: "GND", "S%d.4" % i: "GND", "S%d.5" % i: "GND"})
    if got["S%d.1" % i] != got["S%d.2" % i]:
        print("MEC pins 1/2 differ", i); err += 1
for k, v in rules.items():
    if got.get(k, "") != v:
        print("RULE FAIL", k, "expected", repr(v), "got", repr(got.get(k))); err += 1
cur = "LED_IN"                   # RGB chain: U6 -> 33R -> D1..D5 -> LED_OUT
if not any(got.get("R%d.1" % i) == "LED_BUF" and got.get("R%d.2" % i) == "LED_IN" for i in range(1, 60)) or got["U6.4"] != "LED_BUF":
    print("LED buffer path wrong"); err += 1
for i in range(1, 6):
    if got["D%d.4" % i] != cur:
        print("CHAIN BREAK D%d" % i); err += 1
    cur = got["D%d.2" % i]
if cur != "LED_OUT": print("CHAIN END", cur); err += 1
# DIN chain core <- U1 <- U2 <- U3 <- column ; DOUT chain core -> U4 -> U5 -> column
if not (got["U1.9"] == "SI_UP" and got["U1.10"] == got["U2.9"] and got["U2.10"] == got["U3.9"] and got["U3.10"] == "SI_DN"):
    print("DIN CHAIN WRONG"); err += 1
if not (got["U4.14"] == "SO" and got["U4.9"] == got["U5.14"] and got["U5.9"] == "SO_DN"):
    print("DOUT CHAIN WRONG"); err += 1
# every DIN input: one pull-up + (key, encoder or spare); every key reaches a DIN input
ins = [got["%s.%s" % (u, p)] for u in ("U1", "U2", "U3") for p in ("3", "4", "5", "6", "11", "12", "13", "14")]
for n in ins:
    users = [k for k, v in got.items() if v == n]
    if not any(k.startswith("R") for k in users) or not (n.startswith("DIN_SPARE") or any(k[0] in "SE" for k in users)):
        print("INPUT", n, users); err += 1
keys = {v for k, v in got.items() if v.startswith("KEY_")}
for kname in keys:
    if kname not in ins: print("KEY NOT ON DIN", kname); err += 1
# every LED anode: resistor from a 595 output
outs = [got["%s.%s" % (u, p)] for u in ("U4", "U5") for p in ("15", "1", "2", "3", "4", "5", "6", "7")]
anodes = {v for k, v in got.items() if v.startswith("LEDA_")}
for a in anodes:
    q = "LEDQ_" + a[5:]
    if q not in outs or not any(got.get(r + ".1") == q and got.get(r + ".2") == a for r in ["R%d" % i for i in range(1, 60)]):
        print("LED drive wrong", a); err += 1
print("checked", len(exp), "pads +", len(rules), "datasheet rules, inputs", len(ins), "LEDs", len(anodes), "->", "OK" if err == 0 else "%d ERRORS" % err)
sys.exit(1 if err else 0)
