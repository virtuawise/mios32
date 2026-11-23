# Footprint for the APEM / MEC Multimec 3FTH9xx (through-hole, with factory-fitted LED; xx = LED colour, e.g. 65 white,
# 40 yellow, 80 red). Source: APEM datasheet MEC-MULTIMEC3F-2203, p. 3 "PCB layout - TH illuminated" (top view):
#   switch pins on a 10.16 x 7.62 mm grid, 2 top-left, 1 top-right, 4 bottom-left, 3 bottom-right, hole 0.9 mm;
#   switch contact between 1/2 (internally connected) and 3/4 (internally connected);
#   LED cathode (-) in the left column 2.54 mm below pin 2, LED anode (+) in the right column 2.54 mm above pin 3.
#   Body 10 x 10 mm, PCB thickness 1.4 +- 0.2 mm (datasheet) -> order the PCB in 1.6 mm (within tolerance).
# Origin = centre of the switch (= centre of the cap). Pads: 1-4 switch, 5 = LED K, 6 = LED A.
import os
DST = os.path.join(os.path.dirname(os.path.abspath(__file__)), "seq4oled.pretty", "SW_MEC_3FTH9_LED.kicad_mod")
X0, Y0 = -5.08, -3.81                      # pin 2 relative to the centre
pads = [("1", 10.16, 0), ("2", 0, 0), ("3", 10.16, 7.62), ("4", 0, 7.62), ("5", 0, 2.54), ("6", 10.16, 5.08)]
out = ['(footprint "SW_MEC_3FTH9_LED"', '\t(version 20260206)', '\t(generator "pcbnew")', '\t(layer "F.Cu")',
       '\t(descr "APEM Multimec 3FTH9 with LED, datasheet MEC-MULTIMEC3F-2203 p. 3")',
       '\t(property "Reference" "REF**" (at 0 -6.4 0) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))',
       '\t(property "Value" "3FTH9" (at 0 6.6 0) (layer "F.Fab") (effects (font (size 1 1) (thickness 0.15))))',
       '\t(attr through_hole)']
def rect(layer, hw, hh, w):
    pts = [(-hw, -hh), (hw, -hh), (hw, hh), (-hw, hh), (-hw, -hh)]
    for (a, b), (c, d) in zip(pts, pts[1:]):
        out.append('\t(fp_line (start %.3f %.3f) (end %.3f %.3f) (stroke (width %.2f) (type solid)) (layer "%s"))' % (a, b, c, d, w, layer))
rect("F.Fab", 5.0, 5.0, 0.1)               # body 10 x 10
rect("F.SilkS", 6.1, 5.15, 0.12)            # sides outside the pins (x +-5.08, pad 1.7 mm)
rect("F.CrtYd", 6.3, 5.3, 0.05)
out.append('\t(fp_circle (center 0 0) (end 3.25 0) (stroke (width 0.1) (type solid)) (fill no) (layer "F.Fab"))')  # 6.5 mm plunger
for n, x, y in pads:
    shape = "rect" if n in ("1", "5") else "circle"
    out.append('\t(pad "%s" thru_hole %s (at %.3f %.3f) (size 1.7 1.7) (drill 1.0) (layers "*.Cu" "*.Mask"))' % (n, shape, X0 + x, Y0 + y))
out.append('\t(fp_text user "K" (at %.3f %.3f 0) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.1))))' % (X0 + 1.3, Y0 + 2.54))
out.append('\t(fp_text user "A" (at %.3f %.3f 0) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.1))))' % (X0 + 8.86, Y0 + 5.08))
out.append(")")
open(DST, "w").write("\n".join(out) + "\n")
print("saved", DST)
