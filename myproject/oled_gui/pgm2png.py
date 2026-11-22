#!/usr/bin/env python3
"""pgm2png.py in.pgm out.png [schaal=2] : zet frame.pgm om naar een amber-op-zwart PNG (geen extra bibliotheken). Elk scherm krijgt een grijze rand."""
import sys, zlib, struct
src, dst = sys.argv[1], sys.argv[2]; sc = int(sys.argv[3]) if len(sys.argv) > 3 else 2
data = open(src, 'rb').read()
parts = data.split(b'\n', 3); w, h = map(int, parts[1].split()); px = parts[3]
rows = []
for y in range(h):
    r = bytearray()
    for x in range(w):
        on = px[y*w+x] > 127
        c = (255, 181, 71) if on else (8, 6, 3)
        if x % 256 == 255 or x % 256 == 0: c = (60, 60, 60) if not on else c
        r += bytes(c) * sc
    rows += [b'\x00' + bytes(r)] * sc
def chunk(t, d): c = struct.pack('>I', len(d)) + t + d; return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
open(dst, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w*sc, h*sc, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(b''.join(rows))) + chunk(b'IEND', b''))
