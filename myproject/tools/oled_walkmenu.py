# Walks through all pages of the Page Menu (via the panel API), captures every page; writes m0.png, m1.png, ... (8 pages each) and prints the LCD row 1 per page.
import time, json, urllib.request, zlib, struct
A="http://127.0.0.1:8765/api"
def get(p): return urllib.request.urlopen(A+p).read()
def lcd():
    l=json.loads(get('/state'))['lcd']; return [''.join(chr(c) if 32<=c<127 else '#' for c in r) for r in l]
def shot():
    scr=[urllib.request.urlopen(f"http://127.0.0.1:8765/oled_raw?d={d}").read() for d in range(4)]
    return [bytes((((scr[x//256][y*128+(x%256)//2]>>4) if x%2==0 else (scr[x//256][y*128+(x%256)//2]&15))*17) for x in range(1024)) for y in range(64)]
def save(name, rows):
    def chunk(t,c): return struct.pack('>I',len(c))+t+c+struct.pack('>I',zlib.crc32(t+c)&0xffffffff)
    open(name,'wb').write(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',1024,len(rows),8,0,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(b'\x00'+r for r in rows)))+chunk(b'IEND',b''))
def to_menu():
    for t in range(4):
        if lcd()[0].startswith('MStp'): return True
        get('/press?b=EXIT'); time.sleep(1.0)
    return lcd()[0].startswith('MStp')
SKIP=('Edit','Mute Tracks','Utility')
to_menu()
for i in range(60): get('/enc?e=DATAWHEEL&d=-1'); time.sleep(0.05)
time.sleep(1)
seen=[]; batch=[]; bi=0; stale=0
while stale < 3:
    if not to_menu(): print('menu?'); break
    row=lcd()[1]
    names=[row[0:19].strip(), row[19:39].strip()]
    new=False
    for side,name in enumerate(names):
        if not name or name in seen: continue
        new=True; seen.append(name)
        if name in SKIP: continue
        to_menu()
        get('/press?b=' + ('GP1' if side==0 else 'GP5')); time.sleep(2.4)
        print(f"{len(seen):2d} {name:20s} | {lcd()[0]}", flush=True)
        batch+=shot()+[bytes([60])*1024]*2
        if len(batch) >= 8*66: save(f'm{bi}.png', batch); batch=[]; bi+=1
        to_menu()
    stale = 0 if new else stale+1
    get('/enc?e=DATAWHEEL&d=2'); time.sleep(1.3)
if batch: save(f'm{bi}.png', batch)
print("done", len(seen), seen)
