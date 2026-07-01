#!/usr/bin/env python3
"""Convert .raw frames written by cui::Canvas::SaveRaw() into real PNGs.

Uses only the Python standard library (zlib + struct) so no third-party
imaging package is required to inspect the demo's rendered output.

Usage: python3 raw_to_png.py input.raw output.png
"""
import struct
import sys
import zlib


def raw_to_png(in_path: str, out_path: str) -> None:
    with open(in_path, "rb") as f:
        data = f.read()

    if data[:7] != b"CUIRAW1":
        raise ValueError(f"{in_path} is not a CUIRAW1 file")

    width, height = struct.unpack_from("<ii", data, 7)
    pixel_offset = 7 + 8
    pixels = data[pixel_offset:]
    expected = width * height * 3
    if len(pixels) != expected:
        raise ValueError(f"unexpected pixel data size: got {len(pixels)}, expected {expected}")

    # Build raw scanlines with PNG filter-type byte 0 (None) prefixed to each row.
    stride = width * 3
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw.extend(pixels[y * stride:(y + 1) * stride])

    compressed = zlib.compress(bytes(raw), level=9)

    def chunk(tag: bytes, payload: bytes) -> bytes:
        return (struct.pack(">I", len(payload)) + tag + payload +
                struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF))

    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)  # 8-bit, RGB truecolor
    png = sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", compressed) + chunk(b"IEND", b"")

    with open(out_path, "wb") as f:
        f.write(png)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} input.raw output.png", file=sys.stderr)
        sys.exit(1)
    raw_to_png(sys.argv[1], sys.argv[2])
    print(f"wrote {sys.argv[2]}")
