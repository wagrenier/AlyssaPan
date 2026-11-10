"""Draws AlyssaPan's icon: a clock face in the overlay's colours (ui/theme.cpp), no other source art.

Writes resources/alyssapan.png (256 px, the window icon SDL loads at run time) and extern/alyssapan.ico
(16, 24, 32, 48, 64 and 256 px as PNG entries, the exe's icon through extern/icon.rc). Pure Python: no
Pillow on the build machine. Rerun after changing the design; both outputs are committed.

    py tools/make_icon.py
"""

import math
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

NIGHT = (0x14, 0x12, 0x18)
BONE = (0xE6, 0xDD, 0xC8)
ASH = (0x8A, 0x82, 0x78)
BLOOD = (0x7A, 0x14, 0x22)
REDWATER = (0xA3, 0x21, 0x3A)
BRASS = (0xD8, 0xB4, 0x6A)


def seg_dist(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def sample(x, y):
    """Colour and alpha at (x, y) in a unit square, centre (0.5, 0.5)."""
    cx, cy = x - 0.5, y - 0.5
    r = math.hypot(cx, cy)
    # Rounded square of night behind the dial.
    q = max(abs(cx), abs(cy))
    corner = 0.09
    ex, ey = max(abs(cx) - (0.5 - corner), 0.0), max(abs(cy) - (0.5 - corner), 0.0)
    if q > 0.5 or math.hypot(ex, ey) > corner:
        return None
    colour = NIGHT
    if r < 0.43:
        colour = BRASS  # rim
    if r < 0.395:
        colour = BONE  # dial
        # Twelve ticks, the quarters longer.
        for i in range(12):
            a = i * math.pi / 6
            inner = 0.29 if i % 3 == 0 else 0.33
            if seg_dist(cx, cy, inner * math.cos(a), inner * math.sin(a), 0.37 * math.cos(a),
                        0.37 * math.sin(a)) < (0.022 if i % 3 == 0 else 0.012):
                colour = NIGHT
        # The hands at five to twelve: hour near XII, minute at XI.
        hour = -math.pi / 2 - 0.04
        minute = -math.pi / 2 - math.pi / 6
        if seg_dist(cx, cy, 0, 0, 0.2 * math.cos(hour), 0.2 * math.sin(hour)) < 0.03:
            colour = NIGHT
        if seg_dist(cx, cy, 0, 0, 0.31 * math.cos(minute), 0.31 * math.sin(minute)) < 0.02:
            colour = REDWATER
        if r < 0.045:
            colour = BLOOD
    return colour


def render(size, ss=4):
    rows = []
    for py in range(size):
        row = bytearray()
        for px in range(size):
            acc = [0, 0, 0]
            hits = 0
            for sy in range(ss):
                for sx in range(ss):
                    c = sample((px + (sx + 0.5) / ss) / size, (py + (sy + 0.5) / ss) / size)
                    if c is not None:
                        hits += 1
                        for k in range(3):
                            acc[k] += c[k]
            if hits:
                row += bytes((acc[0] // hits, acc[1] // hits, acc[2] // hits, 255 * hits // (ss * ss)))
            else:
                row += bytes(4)
        rows.append(bytes(row))
    return rows


def png(size, rows):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    raw = b"".join(b"\x00" + r for r in rows)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def ico(images):
    header = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    entries = b""
    for size, data in images:
        dim = 0 if size >= 256 else size
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    return header + entries + b"".join(d for _, d in images)


def main():
    images = []
    for size in (16, 24, 32, 48, 64, 256):
        images.append((size, png(size, render(size, 8 if size <= 64 else 3))))
    (ROOT / "resources" / "alyssapan.png").write_bytes(images[-1][1])
    (ROOT / "extern").mkdir(exist_ok=True)
    (ROOT / "extern" / "alyssapan.ico").write_bytes(ico(images))
    print("resources/alyssapan.png, extern/alyssapan.ico")


if __name__ == "__main__":
    main()
