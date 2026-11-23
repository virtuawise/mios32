# Printable spacer tubes for the test stack (ASCII STL): OD 4.0 mm, bore 2.2 mm (M2 screw or a 2 mm pin/filament piece).
#   spacer_3.5mm.stl : PCB top -> switch plate bottom (MX plate mount)
#   spacer_5.0mm.stl : switch plate top -> front panel bottom (= the 5 mm Würth standoff of the real panel)
import math, os
D = os.path.dirname(os.path.abspath(__file__))
def tube(h, ro=2.0, ri=1.1, n=48):
    tris = []
    for i in range(n):
        a0, a1 = 2 * math.pi * i / n, 2 * math.pi * (i + 1) / n
        o0, o1 = (ro * math.cos(a0), ro * math.sin(a0)), (ro * math.cos(a1), ro * math.sin(a1))
        i0, i1 = (ri * math.cos(a0), ri * math.sin(a0)), (ri * math.cos(a1), ri * math.sin(a1))
        tris += [((*o0, 0), (*o1, 0), (*o1, h)), ((*o0, 0), (*o1, h), (*o0, h))]          # outer wall
        tris += [((*i1, 0), (*i0, 0), (*i0, h)), ((*i1, 0), (*i0, h), (*i1, h))]          # bore
        tris += [((*o0, h), (*o1, h), (*i1, h)), ((*o0, h), (*i1, h), (*i0, h))]          # top ring
        tris += [((*o1, 0), (*o0, 0), (*i0, 0)), ((*o1, 0), (*i0, 0), (*i1, 0))]          # bottom ring
    return tris
for h in (3.5, 5.0):
    with open(os.path.join(D, "spacer_%.1fmm.stl" % h), "w") as f:
        f.write("solid spacer\n")
        for t in tube(h):
            f.write(" facet normal 0 0 0\n  outer loop\n" + "".join("   vertex %.4f %.4f %.4f\n" % v for v in t) + "  endloop\n endfacet\n")
        f.write("endsolid spacer\n")
print("spacers written")
