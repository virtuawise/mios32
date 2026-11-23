# Shared helpers for the board build scripts (KiCad 10 Python API): nets, placement with per-pad nets (recorded for the
# verify step), outline, LED cut-out keep-outs, GND fan-out vias, save + expected_nets.json.
import os, json, pcbnew
FP = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints"
LIB = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lib")
OX, OY = 100.0, 100.0

def edge_keepouts(board, W, H, OX, OY, m=0.8):
    """no tracks/vias within 0.8 mm of the board edge (Freerouting does not apply the edge clearance)"""
    for x0, y0, x1, y1 in ((0, 0, W, m), (0, H - m, W, H), (0, 0, m, H), (W - m, 0, W, H)):
        z = pcbnew.ZONE(board)
        z.SetIsRuleArea(True); z.SetDoNotAllowTracks(True); z.SetDoNotAllowVias(True)
        z.SetDoNotAllowZoneFills(False); z.SetDoNotAllowPads(False); z.SetDoNotAllowFootprints(False)
        z.SetLayerSet(pcbnew.LSET.AllCuMask())
        ol = z.Outline(); ol.NewOutline()
        for x, y in ((x0, y0), (x1, y0), (x1, y1), (x0, y1)):
            ol.Append(pcbnew.FromMM(OX + x), pcbnew.FromMM(OY + y))
        board.Add(z)

def set_paper(path, size):
    """KiCad 10's Python API has no PAGE_INFO: set the sheet in the saved file (boards sit at x/y = 100 mm, so A4 is too
    small - the title block would end up in the middle of the board)"""
    import re
    s = open(path).read()
    s = re.sub(r'\(paper "[^"]*"( portrait)?\)', '(paper "%s")' % size, s, count=1)
    open(path, "w").write(s)

class Board:
    def __init__(self, W, H):
        self.b = pcbnew.BOARD(); self.W, self.H = W, H
        self.nets, self.expect = {}, {}
        for x1, y1, x2, y2 in ((0, 0, W, 0), (W, 0, W, H), (W, H, 0, H), (0, H, 0, 0)):
            s = pcbnew.PCB_SHAPE(self.b); s.SetShape(pcbnew.SHAPE_T_SEGMENT)
            s.SetStart(pcbnew.VECTOR2I_MM(OX + x1, OY + y1)); s.SetEnd(pcbnew.VECTOR2I_MM(OX + x2, OY + y2))
            s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(pcbnew.FromMM(0.1)); self.b.Add(s)
        edge_keepouts(self.b, W, H, OX, OY)

    def net(self, name):
        if name not in self.nets:
            n = pcbnew.NETINFO_ITEM(self.b, name); self.b.Add(n); self.nets[name] = n
        return self.nets[name]

    def place(self, lib, name, ref, x, y, rot=0.0, padnets=None, back=False, value=""):
        libdir = os.path.join(LIB, lib + ".pretty") if lib == "seq4oled" else os.path.join(FP, lib + ".pretty")
        f = pcbnew.FootprintLoad(libdir, name)
        assert f is not None, (lib, name)
        f.SetReference(ref); f.SetValue(value or name)
        self.b.Add(f)
        if "0603" in name:                  # dense 0603 blocks: reference on the assembly (Fab) layer, not on the silkscreen
            f.Reference().SetLayer(pcbnew.B_Fab if back else pcbnew.F_Fab)
        if back:
            f.Flip(f.GetPosition(), pcbnew.FLIP_DIRECTION_TOP_BOTTOM)
        f.SetOrientationDegrees(rot)
        f.SetPosition(pcbnew.VECTOR2I_MM(OX + x, OY + y))
        padnets = padnets or {}
        for p in f.Pads():
            n = padnets.get(p.GetNumber())
            if n:
                p.SetNet(self.net(n)); self.expect[(ref, p.GetNumber())] = n
        missing = set(padnets) - {p.GetNumber() for p in f.Pads()}
        assert not missing, (ref, "has no pad", missing)
        return f

    def keepout(self, x, y, hw, hh, nofill=False, round_=False):
        """no tracks/vias (LED cut-outs); nofill=True: no copper at all (isolated mounting holes)"""
        import math
        z = pcbnew.ZONE(self.b)
        z.SetIsRuleArea(True); z.SetDoNotAllowTracks(True); z.SetDoNotAllowVias(True)
        z.SetDoNotAllowZoneFills(nofill); z.SetDoNotAllowPads(False); z.SetDoNotAllowFootprints(False)
        z.SetLayerSet(pcbnew.LSET.AllCuMask())
        ol = z.Outline(); ol.NewOutline()
        pts = [(hw * math.cos(a * math.pi / 8), hh * math.sin(a * math.pi / 8)) for a in range(16)] if round_ else \
              ((-hw, -hh), (hw, -hh), (hw, hh), (-hw, hh))
        for dx, dy in pts:
            ol.Append(pcbnew.FromMM(OX + x + dx), pcbnew.FromMM(OY + y + dy))
        self.b.Add(z)

    def stack_hole(self, ref, x, y):
        """M1.6 through-bolt of the panel stack (PCB -> spacer -> switch plate -> blind standoff on the front panel):
        unplated 2.2 mm hole, no copper within 2.5 mm, so the screw never touches GND (no ground tie between boards)"""
        self.place("MountingHole", "MountingHole_2.2mm_M2", ref, x, y, value="M1.6 stack")
        self.keepout(x, y, 2.5, 2.5, nofill=True, round_=True)

    def _free(self, pt, r, own):
        for f in self.b.GetFootprints():
            for pd in f.Pads():
                if pd is own: continue
                if pd.HitTest(pt, pcbnew.FromMM(r)) or (pd.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH and pd.GetBoundingBox().Inflate(pcbnew.FromMM(r)).Contains(pt)):
                    return False
            for g in f.GraphicalItems():
                if g.GetLayer() == pcbnew.Edge_Cuts and g.GetBoundingBox().Inflate(pcbnew.FromMM(r + 0.3)).Contains(pt):
                    return False
        for t in self.b.GetTracks():
            if t.HitTest(pt, pcbnew.FromMM(r + 0.25)):
                return False
        return True

    def gnd_fanout(self):
        """every SMD GND pad gets its own short track + via to the GND planes (decoupling best practice)"""
        gnd, fan = self.nets["GND"], 0
        for f in self.b.GetFootprints():
            for pd in f.Pads():
                if pd.GetNetname() != "GND" or pd.GetAttribute() != pcbnew.PAD_ATTRIB_SMD:
                    continue
                c = pd.GetPosition()
                layer = pcbnew.F_Cu if pd.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu
                for dx, dy in ((0, 1.3), (0, -1.3), (1.3, 0), (-1.3, 0), (1.0, 1.0), (-1.0, 1.0), (1.0, -1.0), (-1.0, -1.0), (0, 1.8), (0, -1.8), (1.8, 0), (-1.8, 0)):
                    pt = pcbnew.VECTOR2I(c.x + pcbnew.FromMM(dx), c.y + pcbnew.FromMM(dy))
                    if self._free(pt, 0.55, pd):
                        t = pcbnew.PCB_TRACK(self.b); t.SetStart(c); t.SetEnd(pt); t.SetWidth(pcbnew.FromMM(0.4))
                        t.SetLayer(layer); t.SetNet(gnd); t.SetLocked(True); self.b.Add(t)
                        v = pcbnew.PCB_VIA(self.b); v.SetPosition(pt); v.SetWidth(pcbnew.FromMM(0.6)); v.SetDrill(pcbnew.FromMM(0.3))
                        v.SetNet(gnd); v.SetLocked(True); self.b.Add(v); fan += 1
                        break
                else:
                    print("no room for a GND via at", f.GetReference(), pd.GetNumber())
        print("GND fan-out vias", fan)

    def save(self, path):
        ds = self.b.GetDesignSettings(); ds.m_CopperEdgeClearance = pcbnew.FromMM(0.3)
        self.b.Save(path)
        set_paper(path, "A3")
        with open(os.path.join(os.path.dirname(path), "expected_nets.json"), "w") as f:
            json.dump({"%s.%s" % k: v for k, v in self.expect.items()}, f, indent=0, sort_keys=True)
        print("saved", path, "footprints", len(self.b.GetFootprints()), "nets", len(self.nets))
