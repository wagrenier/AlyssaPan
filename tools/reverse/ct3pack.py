"""ct3pack.py -- read DATA\\CT3PACK.DAT, Clock Tower 3's one archive (architecture.md "Data").

    py ct3pack.py list [PATTERN]          the directory: name, size, sector (fnmatch PATTERN, upper case)
    py ct3pack.py extract NAME [...]      unpack each file ('CT3X' files through the game's Extract) to
                                          work/pack/<NAME> (the image the bank loader keeps, unrelocated)
    py ct3pack.py info NAME               the unpacked image's header and ALL_TABLE words (+0x80)
    py ct3pack.py tex NAME [...]          every texture the bank's draw lists name (DRAW_OBJ_ENTRY +8 ->
                                          TEX_INFO -> SUN_TEX_ATTR), level 0 with the entry's CLUT, to
                                          work/ref/<bank>/<list>_<i>_<tex>_c<clut>.png: the references

The directory (FlInit): sector 0's words [0] count, [1] header bytes, [2] header sectors; 0x20-byte rows
from sector 1: name[0x14], size, first sector (relative to the pack), sector count. A packed file
('CT3X' at +4): sector 0 is the decoder header (escape byte +0xc, words[246] +0x18 for codes 10..255,
byteWords[256] +0x3f0 for code 9), the packed bytes start at sector 1 (game/fileio.c Extract).
Offline; the data root is the game folder (ALYSSAPAN_DATA or alyssapan.py's default).
"""
import fnmatch
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_ROOT = r"C:\Users\willi\Documents\Decomp\Clocl Tower 3"
SECTOR = 0x800
CT3X = 0x43543358


def pack_path():
    root = os.environ.get("ALYSSAPAN_DATA", DEFAULT_ROOT)
    return os.path.join(root, "DATA", "CT3PACK.DAT")


def directory(f):
    f.seek(0)
    count, hbytes, hsects = struct.unpack("<3I", f.read(12))
    f.seek(SECTOR)
    raw = f.read(count * 0x20)
    rows = []
    for i in range(count):
        r = raw[i * 0x20:(i + 1) * 0x20]
        name = r[:0x14].split(b"\0", 1)[0].decode("ascii", "replace")
        size, sect, nsect = struct.unpack("<3I", r[0x14:0x20])
        rows.append((name, size, sect, nsect))
    return rows


def find(rows, name):
    name = name.upper()
    for r in rows:
        if r[0] == name:
            return r
    raise SystemExit("not in the pack: " + name)


def extract(hdr, src):
    """game/fileio.c Extract over the whole packed body at once."""
    esc = hdr[0xc]
    words = struct.unpack_from("<246I", hdr, 0x18)
    bwords = struct.unpack_from("<256I", hdr, 0x3f0)
    out = bytearray()
    i, n = 0, len(src)
    while i < n:
        b = src[i]
        if b != esc:
            out.append(b)
            i += 1
            continue
        if i + 1 >= n:
            break
        c = src[i + 1]
        if c == 0:
            out.append(esc); i += 2
        elif c == 1:
            out += bytes([src[i + 3]]) * src[i + 2]; i += 4
        elif c == 2:
            out += bytes(src[i + 3:i + 5]) * src[i + 2]; i += 5
        elif c == 3:
            out += bytes(src[i + 3:i + 7]) * src[i + 2]; i += 7
        elif c == 4:
            prev = struct.unpack_from("<i", out, len(out) - 4)[0]
            d = src[i + 2] - 256 if src[i + 2] >= 128 else src[i + 2]
            out += struct.pack("<I", (prev + d) & 0xffffffff); i += 3
        elif 5 <= c <= 8:
            i += 2
        elif c == 9:
            out += struct.pack("<I", bwords[src[i + 2]]); i += 3
        else:
            out += struct.pack("<I", words[c - 10]); i += 2
    return bytes(out)


def read_file(f, row):
    name, size, sect, nsect = row
    f.seek(sect * SECTOR)
    data = f.read(size)
    if len(data) >= 8 and struct.unpack_from("<I", data, 4)[0] == CT3X:
        unpacked = struct.unpack_from("<I", data, 8)[0]
        img = extract(data[:SECTOR], data[SECTOR:])
        if len(img) < unpacked:
            print("  warning: %s unpacked to %d of %d bytes" % (name, len(img), unpacked))
        return img[:unpacked], True
    return data, False


PSM_CT32, PSM_CT24, PSM_CT16, PSM_T8, PSM_T4 = 0x00, 0x01, 0x02, 0x13, 0x14
# ALL_TABLE draw lists of DRAW_OBJ_ENTRY words (0-ended): objOpaque, objTrans, objAfter.
DRAW_LISTS = ((0x18, "opaque"), (0x1c, "trans"), (0x60, "after"))


def u32(img, off):
    return struct.unpack_from("<I", img, off)[0]


def ct32(v):
    """A GS PSMCT32 colour (alpha 0..0x80) as RGBA bytes."""
    return bytes((v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, min(255, (v >> 24) * 2)))


def decode_tex(img, base, attr, clut_index):
    """(w, h, rgba) of level 0 of a SUN_TEX_ATTR (game/QuatSlerp.h), or None for a format not read."""
    w, h, tw, th, psm, mip_max = struct.unpack_from("<5Ii", img, attr)
    data = u32(img, attr + 0x1c) - base
    clut_count, clut_list = u32(img, attr + 0x2c), u32(img, attr + 0x30)
    if not (0 < w <= 1024 and 0 < h <= 1024) or data < 0 or data >= len(img):
        return None
    pal = None
    if psm in (PSM_T8, PSM_T4):
        if clut_count == 0:
            return None
        ci = clut_index if clut_index < clut_count else 0
        clut = u32(img, clut_list - base + 4 * ci) - base
        n = 256 if psm == PSM_T8 else 16
        pal = [ct32(u32(img, clut + 4 * i)) for i in range(n)]
        if n == 256:                    # CSM1: entries 8..15 and 16..23 of every 32 swapped
            pal = [pal[(i & ~0x18) | ((i & 8) << 1) | ((i & 0x10) >> 1)] for i in range(256)]
    out = bytearray()
    if psm == PSM_T8:
        for i in range(w * h):
            out += pal[img[data + i]]
    elif psm == PSM_T4:
        for i in range(w * h):
            b = img[data + i // 2]
            out += pal[(b >> 4) if i & 1 else (b & 15)]
    elif psm == PSM_CT32:
        for i in range(w * h):
            out += ct32(u32(img, data + 4 * i))
    elif psm == PSM_CT24:
        for i in range(w * h):
            out += bytes(img[data + 3 * i:data + 3 * i + 3]) + b"\xff"
    elif psm == PSM_CT16:
        for i in range(w * h):
            v = struct.unpack_from("<H", img, data + 2 * i)[0]
            out += bytes(((v & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31,
                          ((v >> 10) & 31) * 255 // 31, 255 if v & 0x8000 else 0))
    else:
        return None
    return w, h, bytes(out)


def textures(name, img):
    """Every texture the bank's draw lists name, to work/ref/<bank>/: the reference pictures."""
    sys.path.insert(0, HERE)
    import tm2png
    base = u32(img, 8)
    table = 0x80
    outdir = os.path.join(HERE, "work", "ref", os.path.splitext(name)[0])
    seen = set()
    for off, kind in DRAW_LISTS:
        lst = u32(img, table + off)
        if lst == 0:
            continue
        p = lst - base
        i = 0
        while 0 <= p < len(img) and u32(img, p) != 0:
            ent = u32(img, p) - base
            tex = u32(img, ent + 8)
            if tex:
                t = tex - base
                attr = u32(img, t)
                clut = u32(img, t + 0xc)
                key = (attr, clut)
                if attr and key not in seen:
                    seen.add(key)
                    a = attr - base
                    w, h, tw, th, psm = struct.unpack_from("<5I", img, a)
                    got = decode_tex(img, base, a, clut)
                    tag = "%s_%02d_%08x_c%d" % (kind, i, attr, clut)
                    if got:
                        path = os.path.join(outdir, tag + ".png")
                        tm2png.png(path, got[0], got[1], got[2])
                        print("  %-5s %3d  tex 0x%08x  %4dx%-4d psm 0x%02x clut %d -> %s"
                              % (kind, i, attr, w, h, psm, clut, os.path.relpath(path, HERE)))
                    else:
                        print("  %-5s %3d  tex 0x%08x  %4dx%-4d psm 0x%02x: not decoded" % (kind, i, attr, w, h, psm))
            p += 4
            i += 1
        print("%s list (ALL_TABLE +0x%02x): %d entries" % (kind, off, i))


def main(argv):
    if not argv:
        print(__doc__)
        return 2
    cmd = argv[0]
    with open(pack_path(), "rb") as f:
        rows = directory(f)
        if cmd == "list":
            pat = argv[1].upper() if len(argv) > 1 else "*"
            for name, size, sect, nsect in rows:
                if fnmatch.fnmatch(name, pat):
                    print("%-20s %10d  sector %7d  (%d)" % (name, size, sect, nsect))
            return 0
        if cmd == "tex":
            for name in argv[1:]:
                img, _ = read_file(f, find(rows, name))
                print("%s (%s):" % (name.upper(), img[4:8].decode("ascii", "replace")))
                textures(name.upper(), img)
            return 0
        if cmd in ("extract", "info"):
            outdir = os.path.join(HERE, "work", "pack")
            os.makedirs(outdir, exist_ok=True)
            for name in argv[1:]:
                img, packed = read_file(f, find(rows, name))
                path = os.path.join(outdir, name.upper())
                with open(path, "wb") as o:
                    o.write(img)
                kind = img[4:8].decode("ascii", "replace")
                print("%s: %d bytes%s, kind %r -> %s" % (name.upper(), len(img),
                      " (CT3X)" if packed else "", kind, path))
                if cmd == "info" and len(img) >= 0x80 + 0xf0:
                    base = struct.unpack_from("<I", img, 8)[0]
                    print("  linked base 0x%08x" % base)
                    for off in range(0, 0xf0, 0x10):
                        w = struct.unpack_from("<4I", img, 0x80 + off)
                        print("  ALL_TABLE +0x%02x: %s" % (off, " ".join("%08x" % x for x in w)))
            return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
