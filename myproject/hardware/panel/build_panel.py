# Front panel (printed FR4, black solder mask, white legends) and hidden switch plate (FR4 1.5 mm), built from the
# virtual panel design (myproject/panel/panel_oled.html: 62 px = 19.05 mm). verify_panel.py checks them independently
# against the key/encoder/OLED/hole positions of the routed PCBs. Run with KiCad's Python.
import os, sys, math, pcbnew
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from boardlib import OX, OY, LIB
D = os.path.dirname(os.path.abspath(__file__))
MM = 62 / 19.05                                   # px per mm (panel_oled.html)
PW, PH = 1545 / MM, 470 / MM                      # #wrap 1545 x 470 px
CORNER = 12 / MM                                  # border-radius 12 px

# ---- design geometry (panel_oled.html), in panel mm, origin = top left of the front panel ----
GL, OY_ = 360, 50
OXg = [GL + g * 91.0 * MM for g in range(4)]
CX = [(OXg[d] + 31 + k * 62) / MM for d in range(4) for k in range(4)]        # GP column centres
ROWS = [(203 + 29) / MM, (281 + 29) / MM, (359 + 29) / MM]                    # GP, select, function row centres
ENC_Y = (160 + 17) / MM                                                       # encoder knob centre (34 px knob at y 160)
LXc = [(22 + k * 62 + 29) / MM for k in range(5)]                             # left block columns
SROW = [(174.5 + 29) / MM, (228.5 + 29) / MM, (282.5 + 29) / MM]              # small key rows
TR_Y = (359 + 29) / MM
DW = ((130 + 48) / MM, (28 + 48) / MM)
BEAT = ((266 + 9) / MM, (66 + 9) / MM)
OLED_C = [((OXg[d] + 124) / MM, OY_ / MM + 19.18 / 2) for d in range(4)]     # centre of each active area
SMALL = [["STEP", "TRACK", "PARAM.", "TRIGGER", "INSTR."], ["MUTE", "PHRASE", "BOOKM.", "SOLO", "METR."], ["LOOP", "REC.", "LIVE"]]
FNL = ["COPY", "PASTE", "CLEAR", "MOVE", "SCROLL", "FAST", "ALL", "SHIFT", "<", ">", "EXIT", "MENU", "EDIT", "PATTERN", "SONG", "UTILITY"]
# PCB placement on the panel (same numbers as the virtual panel): left PCB key column 0 at x = 13.0 mm of the board,
# column PCB k centred on its OLED group; all boards start 3.163 mm below the panel top (top edge of the Newhaven PCB)
BOARD_Y = OY_ / MM + 19.18 + 12.62 - 44.0
LEFT_X = LXc[0] - 13.0
COL_X = [OLED_C[d][0] - 45.3 for d in range(4)]
# stack through-bolts: positions of the PCB holes (build_left.py / build_column.py), + extra standoffs in the free margins
LEFT_KX = [13.0 + 19.05 * k for k in range(5)]
LEFT_HOLES = [(24.0, 3.5), (86.0, 3.5), ((LEFT_KX[0] + LEFT_KX[1]) / 2, 84.25), ((LEFT_KX[3] + LEFT_KX[4]) / 2, 84.25), (4.5, 101.5), (100.3 - 4.5, 101.5)]
COL_HOLES = [(3.5, 80.0), (90.6 - 3.5, 80.0), (3.5, 104.0), (90.6 - 3.5, 104.0)]
STACK = [(LEFT_X + x, BOARD_Y + y) for x, y in LEFT_HOLES] + [(COL_X[d] + x, BOARD_Y + y) for d in range(4) for x, y in COL_HOLES]
MARGIN = [(x, (BOARD_Y + 126.0 + PH) / 2) for x in (12.0, 100.0, 194.2, 285.2, 376.2, 462.0)] + [(PW - 3.8, 60.0), (PW - 3.8, 100.0)]

def new_board():
    b = pcbnew.BOARD()
    return b
def seg(b, x1, y1, x2, y2, layer=pcbnew.Edge_Cuts, w=0.1):
    s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(pcbnew.VECTOR2I_MM(OX + x1, OY + y1)); s.SetEnd(pcbnew.VECTOR2I_MM(OX + x2, OY + y2))
    s.SetLayer(layer); s.SetWidth(pcbnew.FromMM(w)); b.Add(s)
def arc(b, cx, cy, r, a1, a2, layer=pcbnew.Edge_Cuts, w=0.1):
    p = lambda a: pcbnew.VECTOR2I_MM(OX + cx + r * math.cos(math.radians(a)), OY + cy + r * math.sin(math.radians(a)))
    s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_ARC); s.SetArcGeometry(p(a1), p((a1 + a2) / 2), p(a2))
    s.SetLayer(layer); s.SetWidth(pcbnew.FromMM(w)); b.Add(s)
def rrect(b, cx, cy, w, h, r, layer=pcbnew.Edge_Cuts, lw=0.1):
    x0, y0, x1, y1 = cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2
    seg(b, x0 + r, y0, x1 - r, y0, layer, lw); seg(b, x1, y0 + r, x1, y1 - r, layer, lw)
    seg(b, x1 - r, y1, x0 + r, y1, layer, lw); seg(b, x0, y1 - r, x0, y0 + r, layer, lw)
    arc(b, x1 - r, y0 + r, r, 270, 360, layer, lw); arc(b, x1 - r, y1 - r, r, 0, 90, layer, lw)
    arc(b, x0 + r, y1 - r, r, 90, 180, layer, lw); arc(b, x0 + r, y0 + r, r, 180, 270, layer, lw)
def circle(b, cx, cy, d, layer=pcbnew.Edge_Cuts, w=0.1, fill=False):
    s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_CIRCLE)
    s.SetCenter(pcbnew.VECTOR2I_MM(OX + cx, OY + cy)); s.SetEnd(pcbnew.VECTOR2I_MM(OX + cx + d / 2, OY + cy))
    s.SetLayer(layer); s.SetWidth(pcbnew.FromMM(w)); s.SetFilled(fill); b.Add(s)
def poly(b, pts, layer=pcbnew.F_SilkS):
    s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_POLY)
    s.SetPolyPoints([pcbnew.VECTOR2I_MM(OX + x, OY + y) for x, y in pts]); s.SetFilled(True)
    s.SetLayer(layer); s.SetWidth(0); b.Add(s)
def text(b, s, x, y, h=1.6, bold=True):
    t = pcbnew.PCB_TEXT(b); t.SetText(s); t.SetPosition(pcbnew.VECTOR2I_MM(OX + x, OY + y)); t.SetLayer(pcbnew.F_SilkS)
    t.SetTextSize(pcbnew.VECTOR2I_MM(h, h)); t.SetTextThickness(pcbnew.FromMM(h * (0.18 if bold else 0.13))); b.Add(t)

CAP, SCAP, CUT_M = 18.0, 10.6, 1.0                 # 18 mm caps, 1 mm free around the caps of a row; small keys: 11 x 11 hole
def row_cut(b, xs, y):
    x0, x1 = min(xs) - CAP / 2 - CUT_M, max(xs) + CAP / 2 + CUT_M
    rrect(b, (x0 + x1) / 2, y, x1 - x0, CAP + 2 * CUT_M, 1.0)

# ================================ front panel ================================
F = new_board()
rrect(F, PW / 2, PH / 2, PW, PH, CORNER)
for d in range(4):
    for y in ROWS:
        row_cut(F, CX[4 * d:4 * d + 4], y)
    rrect(F, OLED_C[d][0], OLED_C[d][1], 80.0, 21.0, 1.0)                      # OLED window (bezel opening 82 x 22)
row_cut(F, LXc, TR_Y)
for r, row in enumerate(SMALL):
    for c, lbl in enumerate(row):
        rrect(F, LXc[c], SROW[r], 11.0, 11.0, 0.5)                             # APEM: 1T cap 10.6 mm -> cut-out 11.0 x 11.0
        text(F, lbl, LXc[c], SROW[r] + 7.6, 1.5)
for x in CX:
    circle(F, x, ENC_Y, 7.5)                                                   # EC11E bushing / shaft (VERIFY with the part)
circle(F, DW[0], DW[1], 7.5)
circle(F, BEAT[0], BEAT[1], 3.2)
text(F, "DATA", DW[0], DW[1] + 17.0, 1.6); text(F, "BEAT", BEAT[0], BEAT[1] + 4.2, 1.4)
text(F, "SELECT MODE", (LXc[0] + LXc[4]) / 2, SROW[0] - 7.6, 1.5)
for i, x in enumerate(CX):                                                     # function row legends (no labels on GP/select)
    text(F, FNL[i], x, ROWS[2] + CAP / 2 + CUT_M + 2.0, 1.5)
# transport symbols (filled silkscreen)
sy = TR_Y + CAP / 2 + CUT_M + 2.1
def tri(x, y, s, left=False):
    poly(F, [(x + s / 2, y - s / 2), (x + s / 2, y + s / 2), (x - s / 2, y)] if left else [(x - s / 2, y - s / 2), (x - s / 2, y + s / 2), (x + s / 2, y)])
tri(LXc[0] - 0.9, sy, 1.8, True); tri(LXc[0] + 0.9, sy, 1.8, True)
tri(LXc[1] - 0.9, sy, 1.8); tri(LXc[1] + 0.9, sy, 1.8)
poly(F, [(LXc[2] - 0.8, sy - 0.8), (LXc[2] + 0.8, sy - 0.8), (LXc[2] + 0.8, sy + 0.8), (LXc[2] - 0.8, sy + 0.8)])
tri(LXc[3], sy, 2.0)
for dx in (-0.6, 0.6):
    poly(F, [(LXc[4] + dx - 0.35, sy - 0.9), (LXc[4] + dx + 0.35, sy - 0.9), (LXc[4] + dx + 0.35, sy + 0.9), (LXc[4] + dx - 0.35, sy + 0.9)])
text(F, "MIDIBOX SEQ V4+", PW - 40.0, PH - 6.0, 2.2)
# blind M1.6 standoffs on the back (reflowed), no hole through the front
lib = os.path.join(LIB, "seq4oled.pretty")
for i, (x, y) in enumerate(STACK + MARGIN):
    f = pcbnew.FootprintLoad(lib, "Wuerth_WA-SMSI_M1.6_BottomClosed")
    f.SetReference("ST%d" % (i + 1)); f.SetValue("97730506330R"); F.Add(f)
    f.Flip(f.GetPosition(), pcbnew.FLIP_DIRECTION_TOP_BOTTOM)
    f.SetPosition(pcbnew.VECTOR2I_MM(OX + x, OY + y))
F.GetDesignSettings().m_CopperEdgeClearance = pcbnew.FromMM(0.5)
F.Save(os.path.join(D, "front.kicad_pcb"))
from boardlib import set_paper
set_paper(os.path.join(D, "front.kicad_pcb"), "A2")

# ================================ switch plate ================================
P = new_board()
rrect(P, PW / 2, PH / 2, PW, PH, CORNER)
for d in range(4):
    for y in ROWS:
        for x in CX[4 * d:4 * d + 4]:
            rrect(P, x, y, 14.0, 14.0, 0.3)                                    # MX plate cut-out 14.0 x 14.0
    # one opening per column for the Newhaven module (89.2 x 44.0 mm, sits 2.5 mm above the PCB and passes the plate)
    # and the 4 EC11E bodies below it (a 0.2 mm web between them would be useless); 1.8 mm webs between the columns
    y0, y1 = BOARD_Y - 0.5, ENC_Y + 7.0
    rrect(P, OLED_C[d][0], (y0 + y1) / 2, 89.6, y1 - y0, 1.0)
for x in LXc:
    rrect(P, x, TR_Y, 14.0, 14.0, 0.3)
for r, row in enumerate(SMALL):
    for c in range(len(row)):
        rrect(P, LXc[c], SROW[r], 10.8, 10.8, 0.3)                             # 3F body 10 x 10 passes the plate
rrect(P, DW[0], DW[1], 13.0, 14.0, 0.5)
circle(P, BEAT[0], BEAT[1], 3.6)
for x, y in STACK + MARGIN:
    circle(P, x, y, 2.2)                                                       # M1.6 clearance
P.Save(os.path.join(D, "plate.kicad_pcb"))
set_paper(os.path.join(D, "plate.kicad_pcb"), "A2")
print("front panel %.1f x %.1f mm, standoffs %d (stack %d, margin %d); plate saved" % (PW, PH, len(STACK + MARGIN), len(STACK), len(MARGIN)))
