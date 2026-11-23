# Reads column.kicad_pcb back and checks every pad against expected_nets.json (written by build_column.py from the
# datasheet pin maps), plus independent rules: every 74HC165 / LED / OLED pin as in the datasheets, no pad on two nets.
import json, os, sys, pcbnew
D = os.path.dirname(os.path.abspath(__file__))
b = pcbnew.LoadBoard(os.path.join(D, "column.kicad_pcb"))
exp = json.load(open(os.path.join(D, "expected_nets.json")))
got = {}
for f in b.GetFootprints():
    for p in f.Pads():
        if p.GetNumber():
            got["%s.%s" % (f.GetReference(), p.GetNumber())] = p.GetNetname()
err = 0
for k, v in exp.items():
    if got.get(k) != v:
        print("MISMATCH", k, "expected", v, "got", got.get(k)); err += 1
# independent datasheet rules
rules = {}
for u in ("U1", "U2", "U3"):                 # 74HC165: 8 GND, 16 VCC, 15 /CE GND, 1 /PL = RC2, 2 CP = SC
    rules.update({u + ".8": "GND", u + ".16": "+5V", u + ".15": "GND", u + ".1": "RC2", u + ".2": "SC"})
for i in range(1, 13):                        # SK6812MINI-E REV02: 1 VDD, 3 GND
    rules.update({"D%d.1" % i: "VLED", "D%d.3" % i: "GND"})
rules.update({"J7.1": "GND", "J7.2": "+3V3", "J7.7": "OLED_SCLK", "J7.8": "OLED_SDIN", "J7.4": "OLED_DC", "J7.16": "OLED_RES",
              "J7.17": "OLED_CS", "J7.19": "GND", "J7.20": "GND", "J7.9": "", "J7.3": "", "J7.15": "", "J7.18": ""})
for s in ("J1", "J2"):                        # core J8/J9 pinout
    rules.update({s + ".1": "GND", s + ".2": "GND", s + ".3": "+5V", s + ".4": "+5V", s + ".6": "SO", s + ".7": "SC", s + ".8": "SC",
                  s + ".9": "RC2", s + ".10": "RC1"})
rules.update({"J1.5": "SI_UP", "J2.5": "SI_DN"})
for k, v in rules.items():
    if got.get(k, "") != v:
        print("RULE FAIL", k, "expected", repr(v), "got", repr(got.get(k))); err += 1
# LED data chain must be one unbroken chain LED_IN -> D1 ... D12 -> LED_OUT
cur = "LED_IN"
for i in range(1, 13):
    if got["D%d.4" % i] != cur:
        print("CHAIN BREAK at D%d" % i, got["D%d.4" % i], "!=", cur); err += 1
    cur = got["D%d.2" % i]
if cur != "LED_OUT":
    print("CHAIN END", cur); err += 1
# DIN chain: SI_UP <- U1 <- U2 <- U3 <- SI_DN
if not (got["U1.9"] == "SI_UP" and got["U1.10"] == got["U2.9"] and got["U2.10"] == got["U3.9"] and got["U3.10"] == "SI_DN"):
    print("DIN CHAIN WRONG"); err += 1
# every key / encoder net reaches exactly one 74HC165 input and one pull-up
inputs = [n for k, n in got.items() if k[:2] in ("U1", "U2", "U3") and k.split(".")[1] in ("3", "4", "5", "6", "11", "12", "13", "14")]
for n in inputs:
    users = [k for k, v in got.items() if v == n]
    has_r = any(k.startswith("R") for k in users)
    has_src = any(k.startswith("SW") or k.startswith("EN") for k in users)
    if len(users) != 3 or not has_r or not has_src:
        print("INPUT", n, users); err += 1
print("checked", len(exp), "pads +", len(rules), "datasheet rules, inputs", len(inputs), "->", "OK" if err == 0 else "%d ERRORS" % err)
sys.exit(1 if err else 0)
