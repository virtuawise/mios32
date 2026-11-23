# Footprint for the Würth WA-SMSI M1.6 "bottom closed" SMT spacer (e.g. 97730506330R, L = 5 mm, OD 3.3 mm):
# one round SMD pad 4.0 mm (datasheet 97730506330R, recommended land pattern), NO hole - the front side of the
# front panel stays closed. Reflow-soldered on the back of the front panel PCB.
import os
DST = os.path.join(os.path.dirname(os.path.abspath(__file__)), "seq4oled.pretty", "Wuerth_WA-SMSI_M1.6_BottomClosed.kicad_mod")
s = '''(footprint "Wuerth_WA-SMSI_M1.6_BottomClosed"
\t(version 20260206)
\t(generator "pcbnew")
\t(layer "F.Cu")
\t(descr "Wuerth WA-SMSI M1.6 bottom closed SMT spacer, 3.3 mm OD, pad 4.0 mm, no hole")
\t(property "Reference" "REF**" (at 0 -3 0) (layer "F.SilkS") (hide yes) (effects (font (size 0.8 0.8) (thickness 0.12))))
\t(property "Value" "97730506330R" (at 0 3 0) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
\t(attr smd)
\t(fp_circle (center 0 0) (end 1.65 0) (stroke (width 0.1) (type solid)) (fill no) (layer "F.Fab"))
\t(fp_circle (center 0 0) (end 2.3 0) (stroke (width 0.05) (type solid)) (fill no) (layer "F.CrtYd"))
\t(pad "1" smd circle (at 0 0) (size 4.0 4.0) (layers "F.Cu" "F.Paste" "F.Mask"))
)
'''
open(DST, "w").write(s)
print("saved", DST)
