#!/usr/bin/env python3
"""MIOS32-firmware uploaden via USB-MIDI (zelfde protocol als MIOS Studio, tools/mios_studio/src/UploadHandler.cpp).

  python3 mios32_flash.py query                 # core identificeren (schrijft niets)
  python3 mios32_flash.py flash <bestand.hex>   # uploaden (core herstart in bootloader, schrijft, herstart)
Poort: standaard 'MIDIbox SEQ V4+ Port 1' (--port om te wijzigen). Zet eerst mbseq_panel.py UIT.
Blokken van 256 bytes; bootloader-bereik 0x08000000-0x08003FFF wordt overgeslagen; ontbrekende bytes = 0x00 (zoals MIOS Studio).
"""
import sys, time, os, mido

HDR = [0x00, 0x00, 0x7E, 0x32]


def port_names(kind):
    return mido.get_input_names() if kind == "in" else mido.get_output_names()


def send(out, data):
    out.send(mido.Message("sysex", data=data))


def wait_msg(inp, pred, timeout):
    t = time.time()
    while time.time() - t < timeout:
        for m in inp.iter_pending():
            if m.type == "sysex" and list(m.data)[:4] == HDR and pred(list(m.data)):
                return list(m.data)
        time.sleep(0.01)
    return None


def query(out, inp, what):
    send(out, HDR + [0x00, 0x00, what])
    r = wait_msg(inp, lambda d: len(d) > 5 and d[5] == 0x0F, 1.0)
    return "".join(chr(b) for b in r[6:] if 32 <= b < 127).strip() if r else None


def identify(out, inp):
    names = {1: "OS", 2: "Board", 3: "Familie", 4: "Chip-ID", 5: "Serienr", 6: "Flash", 7: "RAM", 8: "App-regel 1", 9: "App-regel 2"}
    info = {}
    for k, n in names.items():
        # antwoord: F0 00 00 7E 32 <dev> 0F <ascii> F7 ; query: ... 00 <k>
        send(out, HDR + [0x00, 0x00, k])
        r = wait_msg(inp, lambda d: len(d) > 5 and d[5] == 0x0F, 1.0)
        info[n] = "".join(chr(b) for b in r[6:] if 32 <= b < 127).strip() if r else None
    return info


def parse_hex(path):
    mem, ext = {}, 0
    for line in open(path):
        line = line.strip()
        if not line.startswith(":"):
            continue
        rec = bytes.fromhex(line[1:])
        n, addr, typ = rec[0], (rec[1] << 8) | rec[2], rec[3]
        if sum(rec) & 0xFF:
            raise SystemExit("Hex: foute checksum")
        if typ == 0:
            for i in range(n):
                mem[ext + addr + i] = rec[4 + i]
        elif typ == 2:
            ext = ((rec[4] << 8) | rec[5]) << 4
        elif typ == 4:
            ext = ((rec[4] << 8) | rec[5]) << 16
    blocks = sorted({a & 0xFFFFFF00 for a in mem})
    return mem, blocks


def block_msg(dev, addr, data):
    body = [0x02]
    chk = 0
    for sh in (25, 18, 11, 4):
        b = (addr >> sh) & 0x7F; body.append(b); chk += b
    for sh in (25, 18, 11, 4):
        b = (256 >> sh) & 0x7F; body.append(b); chk += b
    m, cnt, enc = 0, 0, []
    for byte in data:
        for _ in range(8):
            m = ((m << 1) | (1 if byte & 0x80 else 0)) & 0xFF
            byte = (byte << 1) & 0xFF
            cnt += 1
            if cnt == 7:
                enc.append(m); chk += m; m = 0; cnt = 0
    if cnt:
        m <<= (7 - cnt); enc.append(m & 0x7F); chk += m & 0x7F
    body += enc
    body.append((-chk) & 0x7F)
    return HDR[:] + [dev] + body


def open_ports(name):
    return mido.open_output(name), mido.open_input(name)


def main():
    args = sys.argv[1:]
    port = "MIDIbox SEQ V4+ Port 1"
    if "--port" in args:
        i = args.index("--port"); port = args[i + 1]; del args[i:i + 2]
    if not args or args[0] not in ("query", "flash"):
        raise SystemExit(__doc__)
    out, inp = open_ports(port)
    time.sleep(0.3)
    for _ in inp.iter_pending():
        pass
    info = identify(out, inp)
    print("Core:", info)
    if args[0] == "query":
        os._exit(0)

    hexfile = args[1]
    mem, blocks = parse_hex(hexfile)
    todo = [b for b in blocks if not (0x08000000 <= b <= 0x08003FFF)]
    bad = [b for b in blocks if not (0x08000000 <= b <= 0x08FFFFFF)]
    print(f"{hexfile}: {len(mem)} bytes, {len(blocks)} blokken, {len(todo)} te schrijven (bootloader overgeslagen)")
    if bad:
        raise SystemExit("Hex heeft blokken buiten het STM32-flashbereik: " + ", ".join(hex(b) for b in bad[:5]))
    if not (info.get("Familie") or "").startswith("STM32F4") or "STM32F4" not in (info.get("Familie") or ""):
        raise SystemExit("Core is geen STM32F4xx: stop (niets geschreven)")

    # 1. naar bootloader (reboot). De bootloader wacht maar ~2 s op een commando en de USB-poort verschijnt opnieuw:
    #    daarom na de reset continu proberen het EERSTE blok te sturen tot er een ack (0x0F) komt.
    first = todo[0]
    first_msg = block_msg(0, first, [mem.get(first + i, 0) for i in range(256)])
    print("Naar bootloader-modus ...")
    send(out, HDR + [0x00, 0x00, 0x7F])
    time.sleep(0.2)
    try:
        inp.close(); out.close()
    except Exception:
        pass
    out = inp = None
    t0 = time.time(); seen = None; acked = False
    while time.time() - t0 < 25 and not acked:
        names = mido.get_output_names()
        if names != seen:
            seen = names; print(f"  [{time.time()-t0:4.1f}s] poorten: {names}")
        cand = [n for n in names if "SEQ" in n or "MIDIbox" in n or "MBHP" in n or "BSL" in n.upper() or "boot" in n.lower()]
        for name in cand[:1]:
            try:
                o, i_ = mido.open_output(name), mido.open_input(name)
            except Exception:
                continue
            send(o, first_msg)
            r = wait_msg(i_, lambda d: len(d) > 5 and d[5] in (0x0F, 0x0E), 0.25)
            if r and r[5] == 0x0F:
                out, inp, acked = o, i_, True
                print(f"  bootloader antwoordt op '{name}' na {time.time()-t0:.1f} s")
            else:
                o.close(); i_.close()
        time.sleep(0.05)
    if not acked:
        raise SystemExit("Bootloader niet bereikt binnen 25 s (niets geschreven). Origineel blijft werken.")
    todo = todo[1:]    # eerste blok is al geschreven

    # 2. blokken
    for n, b in enumerate(todo):
        data = [mem.get(b + i, 0) for i in range(256)]
        msg = block_msg(0, b, data)
        for attempt in range(16):
            send(out, msg)
            r = wait_msg(inp, lambda d: len(d) > 5 and d[5] in (0x0F, 0x0E), 2.0)
            if r and r[5] == 0x0F:
                break
            if r and r[5] == 0x0E:
                print(f"  fout {r[6:]} bij blok {hex(b)}, poging {attempt + 1}")
        else:
            raise SystemExit(f"Blok {hex(b)} mislukt na 16 pogingen. Core blijft in bootloader; opnieuw proberen of origineel hex flashen.")
        if n % 100 == 0:
            print(f"  {n}/{len(todo)} blokken")
    print(f"Alle {len(todo)} blokken geschreven. Herstart ...")
    send(out, HDR + [0x00, 0x00, 0x7F])
    time.sleep(3)
    print("Klaar. Poorten:", mido.get_output_names())
    os._exit(0)


if __name__ == "__main__":
    main()
