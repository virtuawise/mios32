# Step after build_column.py + Freerouting: import the routed tracks (column.ses), add GND planes on both layers and a grid
# of stitching vias, fill the zones and save. Run with KiCad's Python.
#   java -jar freerouting-2.5.0.jar -de column.dsn -do column.ses -mp 40 --gui.enabled=false
import os, pcbnew
D = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(D, "column.kicad_pcb")
OX, OY, W, H = 100.0, 100.0, 90.6, 126.0

b = pcbnew.LoadBoard(PCB)
# always run on a freshly built board (build_column.py): it only contains the locked GND fan-out and the LED keep-outs
assert pcbnew.ImportSpecctraSES(b, os.path.join(D, "column.ses")), "SES import failed"
gnd = b.FindNet("GND")

def zone(layer):
    z = pcbnew.ZONE(b)
    z.SetLayer(layer)
    z.SetNet(gnd)
    m = 0.3                                # keep the copper 0.3 mm from the board edge
    pts = [(m, m), (W - m, m), (W - m, H - m), (m, H - m)]
    ol = z.Outline(); ol.NewOutline()
    for x, y in pts:
        ol.Append(pcbnew.FromMM(OX + x), pcbnew.FromMM(OY + y))
    z.SetLocalClearance(pcbnew.FromMM(0.3))
    z.SetMinThickness(pcbnew.FromMM(0.25))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THT_THERMAL)   # SMD pads solid, THT pads with thermal relief (solderable)
    z.SetThermalReliefGap(pcbnew.FromMM(0.3))
    z.SetThermalReliefSpokeWidth(pcbnew.FromMM(0.4))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    b.Add(z)
zone(pcbnew.F_Cu)
zone(pcbnew.B_Cu)

# stitching vias on a 7.5 mm grid, only where they keep >= 0.5 mm from every pad, track and hole
def free(x, y, r):
    p = pcbnew.VECTOR2I_MM(OX + x, OY + y)
    for f in b.GetFootprints():
        for pad in f.Pads():
            if pad.HitTest(p, pcbnew.FromMM(r)) or pad.GetBoundingBox().Inflate(pcbnew.FromMM(r)).Contains(p) and pad.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH:
                return False
        for g in f.GraphicalItems():                     # LED cut-outs etc.
            if g.GetLayer() == pcbnew.Edge_Cuts and g.GetBoundingBox().Inflate(pcbnew.FromMM(r)).Contains(p):
                return False
    for t in b.GetTracks():
        if t.HitTest(p, pcbnew.FromMM(r + t.GetWidth() / 2e6)):
            return False
    return True
n = 0
y = 4.0
while y < H - 3:
    x = 4.0
    while x < W - 3:
        if free(x, y, 1.0) and 1.5 < x < W - 1.5 and 1.5 < y < H - 1.5:
            v = pcbnew.PCB_VIA(b)
            v.SetPosition(pcbnew.VECTOR2I_MM(OX + x, OY + y))
            v.SetWidth(pcbnew.FromMM(0.6)); v.SetDrill(pcbnew.FromMM(0.3))
            v.SetNet(gnd)
            b.Add(v); n += 1
        x += 7.5
    y += 7.5
# GND pins of connectors and encoders: full connection to the planes (return current between boards must see a low impedance)
for f in b.GetFootprints():
    if f.GetReference()[:1] in ("J",) or f.GetReference().startswith("EN"):
        for pd in f.Pads():
            if pd.GetNetname() == "GND":
                pd.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
filler = pcbnew.ZONE_FILLER(b)
filler.Fill(b.Zones())

# Every piece of copper plane must hang on the GND net through at least 2 vias/pins (no stubs that hang on one point):
# add extra stitching vias inside pieces with fewer links, where the other layer's plane is also present, then refill.
def links_in(o):
    n = []
    for t in b.GetTracks():
        if t.Type() == pcbnew.PCB_VIA_T and t.GetNetCode() == gnd.GetNetCode() and o.PointInside(t.GetPosition()):
            n.append(t.GetPosition())
    for f in b.GetFootprints():
        for pd in f.Pads():
            if pd.GetNetCode() == gnd.GetNetCode() and pd.GetAttribute() == pcbnew.PAD_ATTRIB_PTH and o.PointInside(pd.GetPosition()):
                n.append(pd.GetPosition())
    return n
extra = 0
for _ in range(3):
    planes = {z.GetLayer(): z.GetFilledPolysList(z.GetLayer()) for z in b.Zones() if not z.GetIsRuleArea()}
    added = 0
    for l, poly in planes.items():
        other = planes[pcbnew.B_Cu if l == pcbnew.F_Cu else pcbnew.F_Cu]
        for k in range(poly.OutlineCount()):
            o = poly.Outline(k)
            have = links_in(o)
            if len(have) >= 2:
                continue
            bb = o.BBox()
            x0, y0, x1, y1 = bb.GetLeft() / 1e6 - OX, bb.GetTop() / 1e6 - OY, bb.GetRight() / 1e6 - OX, bb.GetBottom() / 1e6 - OY
            cand = []
            y = y0
            while y <= y1:
                x = x0
                while x <= x1:
                    c = pcbnew.VECTOR2I_MM(OX + x, OY + y)
                    ring = [pcbnew.VECTOR2I_MM(OX + x + dx, OY + y + dy) for dx, dy in ((0.45, 0), (-0.45, 0), (0, 0.45), (0, -0.45))]
                    if all(o.PointInside(q) for q in ring) and other.Contains(c) and free(x, y, 0.65):
                        d = min([((c.x - h.x) ** 2 + (c.y - h.y) ** 2) ** 0.5 / 1e6 for h in have] or [99])
                        if d > 2.5:
                            cand.append((-d, x, y))
                    x += 0.5
                y += 0.5
            if cand:
                _, x, y = sorted(cand)[0]
                v = pcbnew.PCB_VIA(b)
                v.SetPosition(pcbnew.VECTOR2I_MM(OX + x, OY + y))
                v.SetWidth(pcbnew.FromMM(0.6)); v.SetDrill(pcbnew.FromMM(0.3))
                v.SetNet(gnd)
                b.Add(v); added += 1
    extra += added
    if not added:
        break
    filler.Fill(b.Zones())
n += extra
b.Save(PCB)
print("routed board saved, tracks", len([t for t in b.GetTracks() if t.Type() == pcbnew.PCB_TRACE_T]),
      "vias", len([t for t in b.GetTracks() if t.Type() == pcbnew.PCB_VIA_T]), "stitching vias", n)
