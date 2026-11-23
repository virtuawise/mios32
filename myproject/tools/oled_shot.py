# Captures the 4 OLEDs from the virtual panel as one PNG. usage: oled_shot.py out.png [scale] [x0] [x1]
# usage: shot.py out.png [scale] [x0] [x1]  (x range over the 1024-px strip)
import sys, urllib.request, zlib, struct
sc=int(sys.argv[2]) if len(sys.argv)>2 else 1
x0=int(sys.argv[3]) if len(sys.argv)>3 else 0
x1=int(sys.argv[4]) if len(sys.argv)>4 else 1024
scr=[urllib.request.urlopen(f"http://127.0.0.1:8765/oled_raw?d={d}").read() for d in range(4)]
def g(x,y):
    b=scr[x//256]; x%=256; v=b[y*128+x//2]; return ((v>>4) if x%2==0 else (v&15))*17
W=(x1-x0)*sc; H=64*sc; img=bytearray()
for y in range(64):
    row=bytearray([0])
    for x in range(x0,x1): row+=bytes([g(x,y)])*sc
    for _ in range(sc): img+=row
def chunk(t,d): return struct.pack(">I",len(d))+t+d+struct.pack(">I",zlib.crc32(t+d)&0xffffffff)
open(sys.argv[1],"wb").write(b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",W,H,8,0,0,0,0))+chunk(b"IDAT",zlib.compress(bytes(img)))+chunk(b"IEND",b""))
