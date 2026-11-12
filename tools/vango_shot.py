"""Ask the display for a screen dump and write a PNG.

Usage:
  python tools/vango_shot.py COM16 tools/last_shot.png
"""

import struct
import sys
import time
import zlib

import serial


def rgb565_to_rgb(buf, w, h):
    rgb = bytearray(w * h * 3)
    o = 0
    for i in range(0, len(buf), 2):
        v = buf[i] | (buf[i + 1] << 8)
        rgb[o] = ((v >> 11) & 0x1F) * 255 // 31
        rgb[o + 1] = ((v >> 5) & 0x3F) * 255 // 63
        rgb[o + 2] = (v & 0x1F) * 255 // 31
        o += 3
    return rgb


def write_png(path, rgb, w, h):
    def chunk(tag, data):
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    raw = b"".join(b"\x00" + rgb[y * w * 3 : (y + 1) * w * 3] for y in range(h))
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 6))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM16"
    out = sys.argv[2] if len(sys.argv) > 2 else "last_shot.png"
    page = sys.argv[3] if len(sys.argv) > 3 else ""
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = 115200
    ser.timeout = 0.2
    ser.dtr = False
    ser.rts = False
    ser.open()
    ser.dtr = False
    ser.rts = False
    time.sleep(0.4)
    ser.reset_input_buffer()
    if page:
        ser.write(("page %s\n" % page).encode())
        ser.flush()
        time.sleep(0.8)
    ser.write(b"shot\n")
    ser.flush()
    t0 = time.time()

    line = b""
    deadline = time.time() + 25
    header = None
    seen = []
    got_n = 0
    while time.time() < deadline:
        b = ser.read(1)
        if not b:
            continue
        got_n += 1
        if b == b"\n":
            text = line.decode("ascii", "ignore").strip()
            line = b""
            if text.startswith("SHOT_FAST "):
                ser.baudrate = int(text.split()[1])
                ser.dtr = False
                ser.rts = False
                time.sleep(0.02)
                continue
            if text.startswith("SHOT ") and not text.startswith("SHOT_"):
                header = text
                break
            if text.startswith("SHOT_ERR"):
                raise SystemExit(text)
            if text and len(seen) < 12:
                seen.append(text[:160])
        else:
            line += b
            if len(line) > 200:
                line = line[-200:]
    if not header:
        raise SystemExit("no SHOT header bytes=%d lines=%s" % (got_n, seen))

    parts = header.split()
    w, h, nbytes = int(parts[1]), int(parts[2]), int(parts[3])
    buf = bytearray()
    deadline = time.time() + 120
    while len(buf) < nbytes and time.time() < deadline:
        chunk = ser.read(min(4096, nbytes - len(buf)))
        if chunk:
            buf += chunk
    if len(buf) != nbytes:
        raise SystemExit("short frame %d/%d" % (len(buf), nbytes))

    tail = b""
    deadline = time.time() + 5
    while b"SHOT_END" not in tail and time.time() < deadline:
        b = ser.read(1)
        if b:
            tail += b
    ser.close()
    write_png(out, rgb565_to_rgb(buf, w, h), w, h)
    print("wrote %s (%dx%d) in %.1fs" % (out, w, h, time.time() - t0))


if __name__ == "__main__":
    main()
