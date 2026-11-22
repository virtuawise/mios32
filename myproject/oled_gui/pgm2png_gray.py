#!/usr/bin/env python3
"""pgm2png_gray.py in.pgm out.png [schaal=2] : PGM met 4-bit grijs (maxval 15) -> amber PNG (zonder bibliotheken)."""
import sys, zlib, struct
src, dst = sys.argv[1], sys.argv[2]; sc = int(sys.argv[3]) if len(sys.argv) > 3 else 2
xa = int(sys.argv[4]) if len(sys.argv) > 4 else 0; xb = int(sys.argv[5]) if len(sys.argv) > 5 else None
data = open(src, 'rb').read()
hdr = data.split(b'\n', 3); w, h = map(int, hdr[1].split()); mx = int(hdr[2]); px = hdr[3]
rows = []
for y in range(h):
    r = bytearray()
    for x in range(xa, xb or w):
        v = px[y*w+x] / mx
        c = (int(255*v), int(181*v), int(71*v))
        if x % 256 == 255 or x % 256 == 0: c = tuple(max(a, 40) for a in c)
        r += bytes(c) * sc
    rows += [b'\x00' + bytes(r)] * sc
def chunk(t, d): c = struct.pack('>I', len(d)) + t + d; return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
open(dst, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', ((xb or w)-xa)*sc, h*sc, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(b''.join(rows))) + chunk(b'IEND', b''))
