# 1:1 test prints (FDM) to try the real switches, caps, encoders and the OLED before ordering anything:
#   left section   = left PCB stand-in + switch plate + front panel above it   (panel x 0 .. 103.2 mm)
#   column section = column PCB stand-in + switch plate + front panel above it (panel x 103.2 .. 194.2 mm)
# The PCB stand-ins keep every hole (switch pins, encoder lugs, header, stack holes) so parts can be pushed in unsoldered.
# Each section is cut in the gap between two groups, so no cut-out is split. Run with KiCad's Python (one board per
# process: KiCad 10's Python breaks on a second LoadBoard in the same process).
import os, sys, subprocess
D = os.path.dirname(os.path.abspath(__file__)); H = os.path.join(D, "..")
PY = sys.executable
MM = 62 / 19.05
PW, PH = 1545 / MM, 470 / MM
SECTIONS = {"left": (0.0, 103.2), "column1": (103.2, 194.2)}

if len(sys.argv) == 4 and sys.argv[1] == "holes":         # worker: enlarge the holes of a PCB for printing
    import pcbnew
    b = pcbnew.LoadBoard(sys.argv[2])
    for f in b.GetFootprints():
        for pd in f.Pads():
            d = pd.GetDrillSize()
            if d.x <= 0:
                continue
            if pd.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH and min(d.x, d.y) >= pcbnew.FromMM(3.0):
                continue                                  # big unplated holes (MX centre peg 4 mm) stay as they are
            nx = max(d.x + pcbnew.FromMM(0.6), pcbnew.FromMM(1.8))
            ny = max(d.y + pcbnew.FromMM(0.6), pcbnew.FromMM(1.8))
            pd.SetDrillSize(pcbnew.VECTOR2I(nx, ny))
    for t in [t for t in b.GetTracks()]:
        if t.Type() == pcbnew.PCB_VIA_T:
            b.Remove(t)                                   # vias would print as tiny holes
    b.Save(sys.argv[3])
    sys.exit(0)

if len(sys.argv) == 5:                                   # worker: section <src> <dst> <x0> <x1>
    import pcbnew
    sys.path.insert(0, H)
    from boardlib import OX, OY
    src, dst, x0, x1 = sys.argv[1], sys.argv[2], float(sys.argv[3]), float(sys.argv[4])
    b = pcbnew.LoadBoard(src)
    for s in [d for d in b.Drawings()]:
        if s.GetLayer() != pcbnew.Edge_Cuts:
            continue
        bb = s.GetBoundingBox()
        l, r, t, btm = bb.GetLeft() / 1e6 - OX, bb.GetRight() / 1e6 - OX, bb.GetTop() / 1e6 - OY, bb.GetBottom() / 1e6 - OY
        outline = l < 0.3 or r > PW - 0.3 or t < 0.3 or btm > PH - 0.3
        crossing = l < x0 < r or l < x1 < r            # e.g. a margin standoff hole right on the cut line
        if outline or crossing or r < x0 or l > x1:
            b.Remove(s)
    for fp in [f for f in b.GetFootprints()]:
        b.Remove(fp)
    for (a, c) in (((x0, 0), (x1, 0)), ((x1, 0), (x1, PH)), ((x1, PH), (x0, PH)), ((x0, PH), (x0, 0))):
        s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(pcbnew.VECTOR2I_MM(OX + a[0], OY + a[1])); s.SetEnd(pcbnew.VECTOR2I_MM(OX + c[0], OY + c[1]))
        s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(pcbnew.FromMM(0.1)); b.Add(s)
    b.Save(dst)
    sys.exit(0)

def stl(src, out):
    r = subprocess.run(["kicad-cli", "pcb", "export", "stl", "--board-only", "--force", "-o", out, src], capture_output=True, text=True)
    ok = r.returncode == 0 and os.path.exists(out)
    print("%-22s %s" % (os.path.basename(out), "OK %.0f kB" % (os.path.getsize(out) / 1024) if ok else "FAILED " + r.stdout[-200:] + r.stderr[-200:]))

for name, (x0, x1) in SECTIONS.items():
    for part in ("front", "plate"):
        dst = os.path.join(D, "%s_%s.kicad_pcb" % (part, name))
        subprocess.run([PY, __file__, os.path.join(H, "panel", part + ".kicad_pcb"), dst, str(x0), str(x1)], capture_output=True)
        stl(dst, dst.replace(".kicad_pcb", ".stl"))
# PCB stand-ins: every plated hole enlarged for FDM (holes print ~0.2-0.4 mm small and 1 mm holes close up):
# + 0.6 mm, at least 1.8 mm, so the pins of switches, encoders, LEDs and headers can be pushed through by hand.
for name in ("left", "column"):
    dst = os.path.join(D, "pcb_%s_print.kicad_pcb" % name)
    subprocess.run([PY, __file__, "holes", os.path.join(H, name, name + ".kicad_pcb"), dst], capture_output=True)
    stl(dst, os.path.join(D, "pcb_%s.stl" % name))
