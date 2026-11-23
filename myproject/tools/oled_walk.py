# usage: walk.py out.png "label|keys;keys" ...   keys: b:BUTTON  e:ENC:d  w:secs ; each step captures after navigation
import sys, time, json, urllib.request, zlib, struct, subprocess
A="http://127.0.0.1:8765/api"
def get(p): return urllib.request.urlopen(A+p).read()
def act(tok):
    k,_,v=tok.partition(':')
    if k=='b': get('/press?b='+v)
    elif k=='e': n,_,d=v.partition(':'); get(f'/enc?e={n}&d={d}')
    elif k=='w': time.sleep(float(v))
    time.sleep(0.6)
def shot():
    scr=[urllib.request.urlopen(f"http://127.0.0.1:8765/oled_raw?d={d}").read() for d in range(4)]
    rows=[]
    for y in range(64):
        r=bytearray()
        for x in range(1024):
            b=scr[x//256][y*128+(x%256)//2]; r.append((((b>>4) if x%2==0 else (b&15)))*17)
        rows.append(bytes(r))
    return rows
out=sys.argv[1]; allrows=[]; log=[]
for spec in sys.argv[2:]:
    label,_,keys=spec.partition('|')
    for tok in keys.split(';'):
        if tok: act(tok)
    time.sleep(2.2)
    lcd=json.loads(get('/state'))['lcd']
    row0=''.join(chr(c) if 32<=c<127 else '#' for c in lcd[0])
    log.append(f"{label}: {row0}")
    allrows+=shot()+[bytes([60])*1024]*2
def chunk(t,c): return struct.pack('>I',len(c))+t+c+struct.pack('>I',zlib.crc32(t+c)&0xffffffff)
open(out,'wb').write(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',1024,len(allrows),8,0,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(b'\x00'+r for r in allrows)))+chunk(b'IEND',b''))
print('\n'.join(log))
