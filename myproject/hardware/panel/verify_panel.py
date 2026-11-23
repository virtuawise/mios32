# Checks the front panel and the switch plate against the routed PCBs (independent of build_panel.py's numbers):
# every key / encoder / LED / OLED / stack hole of left.kicad_pcb and column.kicad_pcb (4x) is transformed to panel
# coordinates and must sit in the right cut-out of both plates; standoffs must keep clear of cut-outs and edges;
# tall parts on the top side of the PCBs must be inside a plate cut-out (the plate is 3.5 mm above the PCB).
import os, sys, math, pcbnew
D = os.path.dirname(os.path.abspath(__file__)); H = os.path.join(D, "..")
MM = 62 / 19.05
BOARD_Y = 50 / MM + 19.18 + 12.62 - 44.0
OFF = {"left": [((22 + 29) / MM - 13.0, BOARD_Y)],
       "column": [((360 + g * 91.0 * MM + 124) / MM - 45.3, BOARD_Y) for g in range(4)]}
mm = lambda v: v / 1e6 - 100.0

def cutouts(path):
    """closed Edge.Cuts shapes as bounding boxes (x0, y0, x1, y1) + circles (cx, cy, d); the largest box is the outline"""
    b = pcbnew.LoadBoard(path)
    segs, boxes, circles = [], [], []
    for s in b.GetDrawings():
        if s.GetLayer() != pcbnew.Edge_Cuts: continue
        if s.GetShape() == pcbnew.SHAPE_T_CIRCLE:
            circles.append((mm(s.GetCenter().x), mm(s.GetCenter().y), 2 * s.GetRadius() / 1e6))
        else:                                         # bounding box without the line width
            bb = s.GetBoundingBox(); w = s.GetWidth() / 2e6
            segs.append((mm(bb.GetLeft()) + w, mm(bb.GetTop()) + w, mm(bb.GetRight()) - w, mm(bb.GetBottom()) - w))
    # group touching segments/arcs into closed shapes
    groups = []
    for s in segs:
        hit = [g for g in groups if any(not (s[2] < o[0] - 0.01 or s[0] > o[2] + 0.01 or s[3] < o[1] - 0.01 or s[1] > o[3] + 0.01) for o in g)]
        merged = [s]
        for g in hit:
            merged += g; groups.remove(g)
        groups.append(merged)
    for g in groups:
        boxes.append((min(o[0] for o in g), min(o[1] for o in g), max(o[2] for o in g), max(o[3] for o in g)))
    boxes.sort(key=lambda r: (r[2] - r[0]) * (r[3] - r[1]))
    outline = boxes.pop()
    return b, outline, boxes, circles

front, f_out, f_box, f_circ = cutouts(os.path.join(D, "front.kicad_pcb"))
plate, p_out, p_box, p_circ = cutouts(os.path.join(D, "plate.kicad_pcb"))
err = 0
def box_at(boxes, x, y):
    return [r for r in boxes if r[0] <= x <= r[2] and r[1] <= y <= r[3]]
def circ_at(circs, x, y, tol=0.05):
    return [c for c in circs if math.hypot(c[0] - x, c[1] - y) <= tol]
def fail(*a):
    global err; err += 1; print("FAIL", *a)

n = {"mx": 0, "mec": 0, "enc": 0, "oled": 0, "hole": 0, "tall": 0, "led": 0}
for name, offs in OFF.items():
    pcb = pcbnew.LoadBoard(os.path.join(H, name, name + ".kicad_pcb"))
    for ox, oy in offs:
        for f in pcb.GetFootprints():
            ref, fp = f.GetReference(), f.GetFPID().GetLibItemName().wx_str()
            pads = {p.GetNumber(): p.GetPosition() for p in f.Pads()}
            if "Cherry_MX" in fp:                     # switch centre = pad 1 + (-2.54, +5.08) for this footprint
                cx, cy = mm(pads["1"].x) - 2.54 + ox, mm(pads["1"].y) + 5.08 + oy
                pc = box_at(p_box, cx, cy); fc = box_at(f_box, cx, cy)
                if len(pc) != 1 or abs((pc[0][0] + pc[0][2]) / 2 - cx) > 0.05 or abs((pc[0][1] + pc[0][3]) / 2 - cy) > 0.05 or abs(pc[0][2] - pc[0][0] - 14.0) > 0.05:
                    fail(name, ref, "MX not centred in a 14 mm plate cut-out", cx, cy, pc)
                if len(fc) != 1 or not (fc[0][0] <= cx - 9.9 and cx + 9.9 <= fc[0][2] and fc[0][1] <= cy - 9.9 and cy + 9.9 <= fc[0][3]):
                    fail(name, ref, "18 mm cap does not have 0.9 mm free in the front cut-out", fc)
                n["mx"] += 1
            elif "MEC_3FTH9" in fp:
                cx, cy = mm(f.GetPosition().x) + ox, mm(f.GetPosition().y) + oy
                fc = box_at(f_box, cx, cy); pc = box_at(p_box, cx, cy)
                if len(fc) != 1 or abs(fc[0][2] - fc[0][0] - 11.0) > 0.05 or abs((fc[0][0] + fc[0][2]) / 2 - cx) > 0.05 or abs((fc[0][1] + fc[0][3]) / 2 - cy) > 0.05:
                    fail(name, ref, "1T cap not centred in an 11 mm front hole", fc)
                if len(pc) != 1 or pc[0][2] - pc[0][0] < 10.4:
                    fail(name, ref, "3F body (10 mm) does not pass the plate", pc)
                n["mec"] += 1
            elif "EC11E" in fp:                       # shaft = pad A + (7.5, 2.5)
                cx, cy = mm(pads["A"].x) + 7.5 + ox, mm(pads["A"].y) + 2.5 + oy
                if not circ_at(f_circ, cx, cy): fail(name, ref, "encoder shaft not centred in a front hole", cx, cy)
                if len(box_at(p_box, cx, cy)) != 1: fail(name, ref, "encoder body not in a plate cut-out")
                n["enc"] += 1
            elif fp == "LED_D3.0mm":
                cx, cy = mm((pads["1"].x + pads["2"].x) / 2) + ox, mm(pads["1"].y) + oy
                if not circ_at(f_circ, cx, cy) or not circ_at(p_circ, cx, cy): fail(name, ref, "beat LED not under its holes")
                n["led"] += 1
            elif fp == "MountingHole_2.2mm_M2":
                cx, cy = mm(f.GetPosition().x) + ox, mm(f.GetPosition().y) + oy
                if not circ_at(p_circ, cx, cy): fail(name, ref, "stack hole without plate hole", cx, cy)
                if not [s for s in front.GetFootprints() if math.hypot(mm(s.GetPosition().x) - cx, mm(s.GetPosition().y) - cy) < 0.05]:
                    fail(name, ref, "stack hole without front panel standoff", cx, cy)
                n["hole"] += 1
            elif fp == "PinHeader_1x20_P2.54mm_Vertical":      # Newhaven: active area centre 21.79 mm below the board top, x = 45.3
                ax, ay = 45.3 + ox, 44.0 - 12.62 - 19.18 / 2 + oy
                w = box_at(f_box, ax, ay)
                if len(w) != 1 or not (w[0][0] <= ax - 76.78 / 2 and ax + 76.78 / 2 <= w[0][2] and w[0][1] <= ay - 19.18 / 2 and ay + 19.18 / 2 <= w[0][3]):
                    fail(name, ref, "OLED active area not inside its window", w)
                pc = box_at(p_box, ax, oy + 22.0)
                if len(pc) != 1 or pc[0][2] - pc[0][0] < 89.4 or pc[0][1] > oy - 0.2 or pc[0][3] < oy + 44.2: fail(name, "Newhaven module (89.2 x 44) not inside a plate cut-out", pc)
                n["oled"] += 1
            if not f.IsFlipped() and ref[0] in "JSE" or fp in ("LED_D3.0mm",) or "CP_Elec" in fp and not f.IsFlipped():
                # tall top-side parts (> 3 mm): must be under a plate cut-out
                if f.IsFlipped(): continue
                cx, cy = mm(f.GetPosition().x) + ox, mm(f.GetPosition().y) + oy
                if "Cherry_MX" in fp: cx, cy = cx - 2.54, cy + 5.08
                if "EC11E" in fp: cx, cy = cx + 7.5, cy + 2.5
                if fp.startswith("PinHeader_1x20"): cx, cy = 45.3 + ox, oy + 22.0
                if fp == "LED_D3.0mm": cx = mm((pads["1"].x + pads["2"].x) / 2) + ox
                if not box_at(p_box, cx, cy) and not circ_at(p_circ, cx, cy, 0.5):
                    fail(name, ref, fp, "tall part under the plate (no cut-out)")
                n["tall"] += 1
# standoffs: pad (4.0 mm) at least 1.0 mm from every front cut-out and from the outline
for s in front.GetFootprints():
    x, y = mm(s.GetPosition().x), mm(s.GetPosition().y)
    for r in f_box:
        dx = max(r[0] - x, 0, x - r[2]); dy = max(r[1] - y, 0, y - r[3])
        if math.hypot(dx, dy) < 2.0 + 1.0: fail("standoff", s.GetReference(), "too close to a cut-out", round(math.hypot(dx, dy), 2))
    for c in f_circ:
        if math.hypot(c[0] - x, c[1] - y) < c[2] / 2 + 3.0: fail("standoff", s.GetReference(), "too close to a hole")
    if min(x - f_out[0], f_out[2] - x, y - f_out[1], f_out[3] - y) < 3.0: fail("standoff", s.GetReference(), "too close to the edge")
    if not circ_at(p_circ, x, y): fail("standoff", s.GetReference(), "no plate hole below")
print("checked", n, "front cut-outs", len(f_box), "holes", len(f_circ), "standoffs", len(front.GetFootprints()),
      "plate cut-outs", len(p_box), "holes", len(p_circ), "->", "OK" if not err else "%d ERRORS" % err)
sys.exit(1 if err else 0)
