# Copy of KiCad's LED_SK6812MINI-E reverse-mount footprint with a tighter cut-out:
# rounded rectangle 3.50 x 3.10 mm, corner radius 0.5 mm (LED body 3.2 x 2.8 mm fits; pad-to-edge 0.30 mm instead of 0.17 mm).
# Pads unchanged. VERIFY the manufacturer's minimum inner router radius (must be <= 0.5 mm).
import os, re, math
SRC = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints/LED_SMD.pretty/LED_SK6812MINI-E_3.2x2.8mm_P1.5mm_ReverseMount.kicad_mod"
DST = os.path.join(os.path.dirname(os.path.abspath(__file__)), "seq4oled.pretty", "LED_SK6812MINI-E_ReverseMount_Cut3.5x3.1.kicad_mod")
s = open(SRC).read()
def blocks(text):                       # top-level (fp_...) blocks inside the footprint
    out, i = [], 0
    while True:
        j = text.find("\n\t(fp_", i)
        if j < 0: return out
        depth, k = 0, j + 1
        while True:
            if text[k] == "(": depth += 1
            elif text[k] == ")":
                depth -= 1
                if depth == 0: break
            k += 1
        out.append((j, k + 1)); i = k + 1
for a, b in reversed(blocks(s)):
    if '"Edge.Cuts"' in s[a:b]:
        s = s[:a] + s[b:]
hw, hh, r = 1.75, 1.55, 0.5
def line(x1, y1, x2, y2):
    return '\n\t(fp_line (start %.4f %.4f) (end %.4f %.4f) (stroke (width 0.05) (type solid)) (layer "Edge.Cuts"))' % (x1, y1, x2, y2)
def arc(cx, cy, a1, a2):                # angles in degrees, y down
    p = lambda a: (cx + r * math.cos(math.radians(a)), cy + r * math.sin(math.radians(a)))
    (sx, sy), (mx, my), (ex, ey) = p(a1), p((a1 + a2) / 2), p(a2)
    return '\n\t(fp_arc (start %.4f %.4f) (mid %.4f %.4f) (end %.4f %.4f) (stroke (width 0.05) (type solid)) (layer "Edge.Cuts"))' % (sx, sy, mx, my, ex, ey)
cut = (line(-hw + r, -hh, hw - r, -hh) + line(hw, -hh + r, hw, hh - r) + line(hw - r, hh, -hw + r, hh) + line(-hw, hh - r, -hw, -hh + r)
       + arc(hw - r, -hh + r, 270, 360) + arc(hw - r, hh - r, 0, 90) + arc(-hw + r, hh - r, 90, 180) + arc(-hw + r, -hh + r, 180, 270))
k = s.rfind(")")
s = s[:k] + cut + "\n" + s[k:]
s = s.replace('(footprint "LED_SK6812MINI-E_3.2x2.8mm_P1.5mm_ReverseMount"', '(footprint "LED_SK6812MINI-E_ReverseMount_Cut3.5x3.1"', 1)
os.makedirs(os.path.dirname(DST), exist_ok=True)
open(DST, "w").write(s)
print("saved", DST, "edge items", s.count('"Edge.Cuts"'))
