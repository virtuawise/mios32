# Column PCB (one per OLED group): parts, connections per pad and placement in one place.
# Run with KiCad's Python:  /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/3.9/bin/python3 build_column.py
# Sources of every pin mapping: see ../README.md (core schematic J8/J9, MIOS32 drivers, Newhaven + SK6812MINI-E datasheets).
import os, pcbnew

FP = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints"
LIB = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "lib")      # own footprints (seq4oled.pretty)
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "column.kicad_pcb")
OX, OY = 100.0, 100.0                  # board origin on the KiCad sheet (mm)
W, H = 90.6, 126.0                     # board size: 91.0 mm group pitch - 0.4 mm gap; top = top edge of the OLED PCB

# ---- geometry (board coordinates, mm; from the virtual panel / Newhaven drawing) ----
KX = [45.3 - 28.575, 45.3 - 9.525, 45.3 + 9.525, 45.3 + 28.575]   # key / encoder columns, 19.05 mm pitch
Y_ENC, Y_GP, Y_SEL, Y_FN = 51.22, 68.12, 92.09, 116.04            # encoder shafts and key rows (24 mm row pitch)
OLED_L = 45.3 - 44.6                                               # left edge of the 89.2 mm Newhaven PCB
OLED_PIN1 = (OLED_L + 20.47 + 48.26, 2.5)                          # 1x20 header, pin 1 at the right (front view), 2.5 mm from top
OLED_HOLES = [(OLED_L + 3.0, 3.0), (OLED_L + 86.2, 3.0), (OLED_L + 3.0, 41.0), (OLED_L + 86.2, 41.0)]  # 83.2 x 38.0 mm
STACK_HOLES = [(3.5, 80.0), (W - 3.5, 80.0), (3.5, 104.0), (W - 3.5, 104.0)]                       # M1.6 through-bolts: PCB -> switch plate -> front panel

board = pcbnew.BOARD()
nets = {}
def net(name):
    if name is None:
        return None
    if name not in nets:
        n = pcbnew.NETINFO_ITEM(board, name)
        board.Add(n)
        nets[name] = n
    return nets[name]

EXPECT = {}     # (ref, pad) -> net, used by verify_column.py
def place(lib, name, ref, x, y, rot=0.0, padnets=None, back=False, value=""):
    libdir = os.path.join(LIB, lib + ".pretty") if lib == "seq4oled" else os.path.join(FP, lib + ".pretty")
    f = pcbnew.FootprintLoad(libdir, name)
    f.SetReference(ref)
    f.SetValue(value or name)
    board.Add(f)
    if "0603" in name:          # dense 0603 blocks: reference on the assembly (Fab) layer, not on the silkscreen
        f.Reference().SetLayer(pcbnew.B_Fab if back else pcbnew.F_Fab)
    if back:
        f.Flip(f.GetPosition(), pcbnew.FLIP_DIRECTION_TOP_BOTTOM if hasattr(pcbnew, "FLIP_DIRECTION_TOP_BOTTOM") else False)
    f.SetOrientationDegrees(rot)
    f.SetPosition(pcbnew.VECTOR2I_MM(OX + x, OY + y))
    for p in f.Pads():
        num = p.GetNumber()
        if padnets and num in padnets and padnets[num] is not None:
            p.SetNet(net(padnets[num]))
            EXPECT[(ref, num)] = padnets[num]
    return f

# ---- outline ----
def edge(x1, y1, x2, y2):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(pcbnew.VECTOR2I_MM(OX + x1, OY + y1)); s.SetEnd(pcbnew.VECTOR2I_MM(OX + x2, OY + y2))
    s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(pcbnew.FromMM(0.1))
    board.Add(s)
edge(0, 0, W, 0); edge(W, 0, W, H); edge(W, H, 0, H); edge(0, H, 0, 0)

# ---- keys (MX, pad 1 = input, pad 2 = GND) and LEDs (SK6812MINI-E: 1 VDD, 2 DOUT, 3 GND, 4 DIN - datasheet REV02) ----
rows = [("GP", Y_GP), ("SEL", Y_SEL), ("FN", Y_FN)]
leds = []
k = 0
for rname, ry in rows:
    for c in range(4):
        k += 1
        key = "KEY_%s%d" % (rname, c + 1)
        # MX footprint origin is pad 1; switch centre is at (-2.54, +5.08) from it
        place("Button_Switch_Keyboard", "SW_Cherry_MX_1.00u_PCB", "SW%d" % k, KX[c] + 2.54, ry - 5.08,
              padnets={"1": key, "2": "GND"}, value=key)
        leds.append(("D%d" % k, KX[c], ry + 5.08))          # LED window of an MX switch: 5.08 mm south of the centre
for i, (ref, x, y) in enumerate(leds):
    din = "LED_IN" if i == 0 else "LED_D%d" % i
    dout = "LED_OUT" if i == len(leds) - 1 else "LED_D%d" % (i + 1)
    place("seq4oled", "LED_SK6812MINI-E_ReverseMount_Cut3.5x3.1", ref, x, y,
          padnets={"1": "VLED", "2": dout, "3": "GND", "4": din}, value="SK6812MINI-E")
    place("Capacitor_SMD", "C_0603_1608Metric", "CL%d" % (i + 1), x + 9.525, y + 0.0, rot=90,   # in the gap between two switches
          padnets={"1": "VLED", "2": "GND"}, value="100nF")

# ---- encoders (EC11E with switch): A/B to DIN, C = GND, S1 = push, S2 = GND (as Wilba: A/B on two DIN pins) ----
for c in range(4):
    # footprint origin is pad A; shaft centre at (+7.5, +2.5)
    place("Rotary_Encoder", "RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm", "EN%d" % (c + 1), KX[c] - 7.5, Y_ENC - 2.5,
          padnets={"A": "ENC%d_A" % (c + 1), "B": "ENC%d_B" % (c + 1), "C": "GND", "S1": "ENC%d_P" % (c + 1), "S2": "GND", "MP": "GND"},
          value="EC11E")

# ---- 74HC165 x 3 (SOIC-16): 1 /PL, 2 CP, 3-6 D4-D7, 7 /Q7, 8 GND, 9 Q7, 10 DS, 11-14 D0-D3, 15 /CE, 16 VCC ----
# chain as MBHP_DINX4: U1.Q7 -> SI towards the core, U1.DS <- U2.Q7, U2.DS <- U3.Q7, U3.DS <- SI from the next board
inputs = {
    "U1": ["KEY_GP1", "KEY_GP2", "KEY_GP3", "KEY_GP4", "KEY_SEL1", "KEY_SEL2", "KEY_SEL3", "KEY_SEL4"],
    "U2": ["KEY_FN1", "KEY_FN2", "KEY_FN3", "KEY_FN4", "ENC1_P", "ENC2_P", "ENC3_P", "ENC4_P"],
    "U3": ["ENC1_A", "ENC1_B", "ENC2_A", "ENC2_B", "ENC3_A", "ENC3_B", "ENC4_A", "ENC4_B"],
}
chain = {"U1": ("SI_UP", "SR_12"), "U2": ("SR_12", "SR_23"), "U3": ("SR_23", "SI_DN")}   # (Q7 net, DS net)
ux = {"U1": 30.0, "U2": 45.3, "U3": 60.6}
r = 0
for u, d in inputs.items():
    q7, ds = chain[u]
    pn = {"1": "RC2", "2": "SC", "3": d[4], "4": d[5], "5": d[6], "6": d[7], "7": None, "8": "GND",
          "9": q7, "10": ds, "11": d[0], "12": d[1], "13": d[2], "14": d[3], "15": "GND", "16": "+5V"}
    place("Package_SO", "SOIC-16_3.9x9.9mm_P1.27mm", u, ux[u], 20.0, padnets=pn, value="74HC165")
    place("Capacitor_SMD", "C_0603_1608Metric", "C%s" % u[1], ux[u], 27.5, padnets={"1": "+5V", "2": "GND"}, value="100nF")
    for i, sig in enumerate(d):          # 10k pull-up on every input (as MBHP_DINX4)
        r += 1
        place("Resistor_SMD", "R_0603_1608Metric", "R%d" % r, ux[u] - 4.5 + (i % 4) * 3.0, 31.0 + (i // 4) * 4.0, rot=90,
              padnets={"1": "+5V", "2": sig}, value="10k")
r += 1
place("Resistor_SMD", "R_0603_1608Metric", "R%d" % r, 60.6, 12.5, rot=90, padnets={"1": "+5V", "2": "SI_DN"}, value="10k")
place("Capacitor_SMD", "C_0805_2012Metric", "C4", 18.0, 20.0, rot=90, padnets={"1": "+5V", "2": "GND"}, value="10uF")
place("Capacitor_SMD", "C_0805_2012Metric", "C5", 72.0, 9.0, rot=90, padnets={"1": "+3V3", "2": "GND"}, value="10uF")
place("Capacitor_SMD", "C_0805_2012Metric", "C6", 76.0, 40.0, rot=90, padnets={"1": "VLED", "2": "GND"}, value="22uF")

# ---- connectors (bottom side) ----
# SRIO, same pinout as core J8/J9: 1/2 GND, 3/4 +5V, 5 SI (left col = J9), 6 SO, 7/8 SC, 9 RC2, 10 RC1
srio = lambda si: {"1": "GND", "2": "GND", "3": "+5V", "4": "+5V", "5": si, "6": "SO", "7": "SC", "8": "SC", "9": "RC2", "10": "RC1"}
place("Connector_IDC", "IDC-Header_2x05_P2.54mm_Vertical", "J1", 9.0, 17.0, padnets=srio("SI_UP"), back=True, value="SRIO_UP")
place("Connector_IDC", "IDC-Header_2x05_P2.54mm_Vertical", "J2", 79.0, 17.0, padnets=srio("SI_DN"), back=True, value="SRIO_DN")
# OLED bus from core J15 (passed through): 1 GND, 2 +3V3, 3 SCLK, 4 SDIN, 5 D/C, 6 /RES, 7-10 CS0..CS3
obus = {"1": "GND", "2": "+3V3", "3": "OLED_SCLK", "4": "OLED_SDIN", "5": "OLED_DC", "6": "OLED_RES",
        "7": "CS0", "8": "CS1", "9": "CS2", "10": "CS3"}
place("Connector_IDC", "IDC-Header_2x05_P2.54mm_Vertical", "J3", 9.0, 39.5, padnets=obus, back=True, value="OLED_BUS_IN")
place("Connector_IDC", "IDC-Header_2x05_P2.54mm_Vertical", "J4", 79.0, 39.5, padnets=obus, back=True, value="OLED_BUS_OUT")
# LED chain + LED supply: 1 VLED, 2 data, 3 GND
place("Connector_PinHeader_2.54mm", "PinHeader_1x03_P2.54mm_Vertical", "J5", 24.0, 38.0, rot=90,
      padnets={"1": "VLED", "2": "LED_IN", "3": "GND"}, back=True, value="LED_IN")
place("Connector_PinHeader_2.54mm", "PinHeader_1x03_P2.54mm_Vertical", "J6", 62.0, 38.0, rot=90,
      padnets={"1": "VLED", "2": "LED_OUT", "3": "GND"}, back=True, value="LED_OUT")
# CS select: solder exactly one of JP1..JP4 (board position 1..4)
for i in range(4):
    place("Jumper", "SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm", "JP%d" % (i + 1), 35.0 + i * 5.0, 9.0, rot=90,
          padnets={"1": "CS%d" % i, "2": "OLED_CS"}, value="CS%d" % i)

# ---- OLED: Newhaven NHD-3.12-25664UCY2 soldered on its 1x20 pins, 4-wire SPI (datasheet p. 4-8) ----
# Power: jumper option #1 (datasheet p. 6/7): logic VDD (pin 2) 3.3 V (200 uA), boost converter BC_VDD (pin 3) from the 5 V LED
# supply (150/170 mA at 100 % on) instead of 310/340 mA from 3.3 V. With the default jumper pin 3 is open on the module (harmless).
oled = {"1": "GND", "2": "+3V3", "3": "VLED",   # 3 = BC_VDD: boost converter from 5 V (jumper option #1: R14 open, R15 closed)
         "4": "OLED_DC", "5": "GND", "6": "GND", "7": "OLED_SCLK", "8": "OLED_SDIN",
        "9": None, "10": "GND", "11": "GND", "12": "GND", "13": "GND", "14": "GND", "15": None, "16": "OLED_RES",
        "17": "OLED_CS", "18": None, "19": "GND", "20": "GND"}
sock = None
for rot in (0, 90, 180, 270):            # orientation: pin 1 at the right, pins going to the left
    if sock:
        board.Remove(sock)
    # the module's own 1x20 pins are soldered straight into the board on a 2.5 mm spacer (a socket would lift the glass to
    # 14.5 mm, above the back of the front panel at 10 mm - see README, stack heights)
    sock = place("Connector_PinHeader_2.54mm", "PinHeader_1x20_P2.54mm_Vertical", "J7", OLED_PIN1[0], OLED_PIN1[1], rot=rot,
                 padnets=oled, value="NHD-3.12-25664UCY2")
    p = {pd.GetNumber(): pd.GetPosition() for pd in sock.Pads()}
    if p["2"].x < p["1"].x and abs(p["2"].y - p["1"].y) < 1000:
        break
for i, (x, y) in enumerate(OLED_HOLES + STACK_HOLES):
    lib, nm = ("MountingHole", "MountingHole_2.7mm_M2.5") if i < 4 else ("MountingHole", "MountingHole_2.2mm_M2")
    place(lib, nm, "H%d" % (i + 1), x, y)
    if i >= 4:        # M1.6 through-bolt of the panel stack: no copper within 2.5 mm (the screw never touches GND)
        import math
        z = pcbnew.ZONE(board)
        z.SetIsRuleArea(True); z.SetDoNotAllowTracks(True); z.SetDoNotAllowVias(True); z.SetDoNotAllowZoneFills(True)
        z.SetDoNotAllowPads(False); z.SetDoNotAllowFootprints(False); z.SetLayerSet(pcbnew.LSET.AllCuMask())
        ol = z.Outline(); ol.NewOutline()
        for a in range(16):
            ol.Append(pcbnew.FromMM(OX + x + 2.5 * math.cos(a * math.pi / 8)), pcbnew.FromMM(OY + y + 2.5 * math.sin(a * math.pi / 8)))
        board.Add(z)

# ---- keep tracks 0.3 mm away from the LED cut-outs (Freerouting does not treat footprint cut-outs as board edge) ----
for i, (ref, x, y) in enumerate(leds):
    z = pcbnew.ZONE(board)
    z.SetIsRuleArea(True); z.SetDoNotAllowTracks(True); z.SetDoNotAllowVias(True)
    z.SetDoNotAllowZoneFills(False); z.SetDoNotAllowPads(False); z.SetDoNotAllowFootprints(False)
    z.SetLayerSet(pcbnew.LSET.AllCuMask())
    ol = z.Outline(); ol.NewOutline()
    for dx, dy in ((-2.05, -1.85), (2.05, -1.85), (2.05, 1.85), (-2.05, 1.85)):
        ol.Append(pcbnew.FromMM(OX + x + dx), pcbnew.FromMM(OY + y + dy))
    board.Add(z)

# ---- GND fan-out: every SMD GND pad gets its own short track + via to the GND planes (decoupling best practice) ----
def pad_free(pt, r, own):
    for f in board.GetFootprints():
        for pd in f.Pads():
            if pd is own: continue
            if pd.HitTest(pt, pcbnew.FromMM(r)) or (pd.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH and pd.GetBoundingBox().Inflate(pcbnew.FromMM(r)).Contains(pt)):
                return False
        for g in f.GraphicalItems():
            if g.GetLayer() == pcbnew.Edge_Cuts and g.GetBoundingBox().Inflate(pcbnew.FromMM(r + 0.3)).Contains(pt):
                return False
    for t in board.GetTracks():
        if t.HitTest(pt, pcbnew.FromMM(r + 0.25)):
            return False
    return True
gnd = nets["GND"]
fan = 0
for f in board.GetFootprints():
    for pd in f.Pads():
        if pd.GetNetname() != "GND" or pd.GetAttribute() != pcbnew.PAD_ATTRIB_SMD:
            continue
        c = pd.GetPosition()
        for dx, dy in ((0, 1.3), (0, -1.3), (1.3, 0), (-1.3, 0), (1.0, 1.0), (-1.0, 1.0), (1.0, -1.0), (-1.0, -1.0), (0, 1.8), (0, -1.8), (1.8, 0), (-1.8, 0)):
            pt = pcbnew.VECTOR2I(c.x + pcbnew.FromMM(dx), c.y + pcbnew.FromMM(dy))
            if pad_free(pt, 0.55, pd):
                t = pcbnew.PCB_TRACK(board); t.SetStart(c); t.SetEnd(pt); t.SetWidth(pcbnew.FromMM(0.4))
                t.SetLayer(pd.GetLayerSet().Seq()[0] if pd.IsOnLayer(pcbnew.F_Cu) is False else pcbnew.F_Cu); t.SetNet(gnd); t.SetLocked(True); board.Add(t)
                v = pcbnew.PCB_VIA(board); v.SetPosition(pt); v.SetWidth(pcbnew.FromMM(0.6)); v.SetDrill(pcbnew.FromMM(0.3)); v.SetNet(gnd); v.SetLocked(True)
                board.Add(v); fan += 1
                break
        else:
            print("no room for a GND via at", f.GetReference(), pd.GetNumber())
print("GND fan-out vias", fan)

ds = board.GetDesignSettings()
ds.m_CopperEdgeClearance = pcbnew.FromMM(0.3)
board.Save(OUT)
import json
with open(os.path.join(os.path.dirname(OUT), "expected_nets.json"), "w") as f:
    json.dump({"%s.%s" % k: v for k, v in EXPECT.items()}, f, indent=0, sort_keys=True)
print("saved", OUT, "footprints", len(board.GetFootprints()), "nets", len(nets))
