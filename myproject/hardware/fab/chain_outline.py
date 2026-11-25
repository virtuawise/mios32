#!/usr/bin/env python3
"""Rewrite a KiCad Edge.Cuts gerber so every contour is one closed, chained path.

KiCad writes each outline segment separately and arcs end a few nm away from the next segment. JLCPCB's quote page then
does not see the outer outline as a closed shape (it took a 10.8 mm cut-out as the board). This snaps endpoints within
2 um together and emits one D02 move per loop.  usage: chain_outline.py in.gm1 out.gm1"""
import re, sys, collections
TOL = 2000  # 2 um in 1e-6 mm units (format 4.6)
src = open(sys.argv[1]).read().split('\n')
head, body, tail = [], [], []
state = 0
for ln in src:
    if state == 0:
        head.append(ln)
        if ln == 'D10*': state = 1
    elif ln.startswith('M02'):
        state = 2; tail.append(ln)
    elif state == 1: body.append(ln)
    else: tail.append(ln)
cur = None; mode = 'G01'; segs = []
for ln in body:
    ln = ln.strip()
    if ln in ('G01*', 'G02*', 'G03*'): mode = ln[:3]; continue
    if ln == 'G75*': continue
    m = re.match(r'X(-?\d+)Y(-?\d+)(?:I(-?\d+)J(-?\d+))?D0([12])\*$', ln)
    if not m: raise SystemExit('unparsed: ' + ln)
    x, y = int(m[1]), int(m[2])
    if m[5] == '2': cur = (x, y); continue
    if m[3] is None: segs.append(dict(s=cur, e=(x, y), m='L'))
    else:
        c = (cur[0] + int(m[3]), cur[1] + int(m[4]))
        segs.append(dict(s=cur, e=(x, y), m='CW' if mode == 'G02' else 'CCW', c=c))
    cur = (x, y)
# snap endpoints
reps = []
def snap(p):
    for r in reps:
        if abs(r[0]-p[0]) <= TOL and abs(r[1]-p[1]) <= TOL: return r
    reps.append(p); return p
for sg in segs:
    sg['s'] = snap(sg['s']); sg['e'] = snap(sg['e'])
ends = collections.defaultdict(list)
for i, sg in enumerate(segs):
    ends[sg['s']].append(i); ends[sg['e']].append(i)
used = [False]*len(segs); loops = []
for i in range(len(segs)):
    if used[i]: continue
    loop = []; used[i] = True
    first = segs[i]; loop.append(dict(first)); p = first['e']; start = first['s']
    while p != start:
        nxt = None
        for j in ends[p]:
            if not used[j]: nxt = j; break
        if nxt is None: raise SystemExit('open contour near %s' % (p,))
        used[nxt] = True; sg = segs[nxt]
        if sg['s'] == p: loop.append(dict(sg)); p = sg['e']
        else:
            r = dict(s=sg['e'], e=sg['s'], m=sg['m'], c=sg.get('c'))
            if sg['m'] != 'L': r['m'] = 'CCW' if sg['m'] == 'CW' else 'CW'
            loop.append(r); p = r['e']
    loops.append(loop)
def bbox(l):
    xs = [s['s'][0] for s in l]; ys = [s['s'][1] for s in l]
    return (max(xs)-min(xs))*(max(ys)-min(ys))
loops.sort(key=bbox, reverse=True)
out = list(head)
for l in loops:
    out.append('X%dY%dD02*' % l[0]['s'])
    for sg in l:
        if sg['m'] == 'L':
            out.append('G01*'); out.append('X%dY%dD01*' % sg['e'])
        else:
            out.append('G75*'); out.append('G02*' if sg['m'] == 'CW' else 'G03*')
            if sg['s'] == sg['e']: out.append('X%dY%dI%dJ%dD01*' % (sg['e'][0], sg['e'][1], sg['c'][0]-sg['s'][0], sg['c'][1]-sg['s'][1]))
            else: out.append('X%dY%dI%dJ%dD01*' % (sg['e'][0], sg['e'][1], sg['c'][0]-sg['s'][0], sg['c'][1]-sg['s'][1]))
    out.append('G01*')
out += tail
open(sys.argv[2], 'w').write('\n'.join(out))
print(len(segs), 'segments ->', len(loops), 'loops; largest bbox first')
