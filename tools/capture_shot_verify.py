#!/usr/bin/env python3
"""Listen COM16 for SHOTB (RLE+base64) framebuffer, write _shot_see.png."""
import base64
import re
import struct
import time
from pathlib import Path

import serial

OUTDIR = Path(__file__).resolve().parent
NEED = 800 * 480 * 2
LOG = OUTDIR / "_shot_capture.log"
PORT = "COM16"


def rgb565_to_png(payload: bytes, pngp: Path) -> None:
    from PIL import Image

    im = Image.new("RGB", (800, 480))
    px = im.load()
    off = 0
    for y in range(480):
        for x in range(800):
            v = struct.unpack_from("<H", payload, off)[0]
            off += 2
            r = (v >> 11) & 0x1F
            g = (v >> 5) & 0x3F
            b = v & 0x1F
            px[x, y] = (r * 255 // 31, g * 255 // 63, b * 255 // 31)
    im.save(pngp)


def collapse_crlf(blob: bytes) -> bytes:
    fixed = bytearray()
    i = 0
    while i < len(blob):
        if blob[i] == 0x0D and i + 1 < len(blob) and blob[i + 1] == 0x0A:
            fixed.append(0x0A)
            i += 2
        else:
            fixed.append(blob[i])
            i += 1
    return bytes(fixed)


def rle_decode(blob: bytes, need: int) -> bytes:
    out = bytearray()
    i = 0
    while i + 4 <= len(blob) and len(out) < need:
        run, pix = struct.unpack_from("<HH", blob, i)
        i += 4
        if run == 0:
            break
        out.extend(struct.pack("<H", pix) * run)
    return bytes(out[:need])


def parse_shot(raw: bytes):
    for m in re.finditer(rb"SHOTB (\d+) (\d+) (\d+) (\d+)\r?\n", raw):
        blen = int(m.group(3))
        rawlen = int(m.group(4))
        start = m.end()
        end = raw.find(b"SHOT_END", start)
        blob = raw[start:end] if end > start else raw[start:]
        text = re.sub(rb"\s+", b"", blob)[:blen]
        try:
            rle = base64.b64decode(text, validate=False)
        except Exception as e:
            return "shotb_fail", None, {"err": str(e), "blen": blen, "got": len(text)}
        payload = rle_decode(rle, rawlen)
        return "shotb", payload, {"blen": blen, "b64": len(text), "rle": len(rle), "out": len(payload)}
    for key in (b"\nSHOT 800 ", b"SHOT 800 "):
        h = raw.find(key)
        if h < 0:
            continue
        if key.startswith(b"\n"):
            h += 1
        hl = raw.find(b"\n", h)
        if hl < 0:
            continue
        end = raw.find(b"SHOT_END", hl)
        blob = raw[hl + 1 : end] if end > hl else raw[hl + 1 :]
        return "shot", collapse_crlf(blob), {"got": len(blob)}
    return "none", None, {}


def main() -> None:
    print("open %s (JTAG secondary — baud ignored; wait one ready-dump)" % PORT, flush=True)
    s = serial.Serial()
    s.port = PORT
    s.baudrate = 115200
    s.timeout = 0.1
    s.dtr = False
    s.rts = False
    s.open()
    s.dtr = False
    s.rts = False
    time.sleep(0.05)
    s.reset_input_buffer()

    raw = b""
    t0 = time.time()
    t_native = None
    while time.time() - t0 < 45:
        d = s.read(262144)
        if d:
            raw += d
            if any(k in d for k in (b"shot", b"SHOT", b"frame sent", b"Rebooting", b"Guru", b"assert")):
                try:
                    for line in d.decode("utf-8", "ignore").splitlines():
                        if any(
                            k in line
                            for k in (
                                "shot task",
                                "shot ready",
                                "SHOT",
                                "frame sent",
                                "send_shot",
                                "Rebooting",
                                "Guru",
                                "assert",
                            )
                        ):
                            print(">>", line[:200], flush=True)
                except Exception:
                    pass
            if t_native is None and b"SHOT_NATIVE" in raw:
                t_native = time.time()
                print("native @ %.1fs" % (t_native - t0), flush=True)
            if b"SHOT_END" in raw and (b"SHOTB " in raw or b"SHOT 800" in raw):
                time.sleep(0.1)
                raw += s.read(65536)
                break

    LOG.write_bytes(raw)
    print("saved", LOG, "bytes", len(raw), "wall=%.1fs" % (time.time() - t0), flush=True)
    if t_native:
        print("dump_after_native=%.1fs" % (time.time() - t_native), flush=True)
    print(
        "frame_sent",
        b"frame sent bytes=" in raw,
        "shot_end",
        b"SHOT_END" in raw,
        flush=True,
    )
    kind, payload, meta = parse_shot(raw)
    print("parse", kind, meta, flush=True)
    if payload is None or len(payload) < NEED:
        print("no/short payload", flush=True)
        s.close()
        return
    (OUTDIR / "_shot_see.bin").write_bytes(payload[:NEED])
    pngp = OUTDIR / "_shot_see.png"
    rgb565_to_png(payload[:NEED], pngp)
    print("PNG", pngp, flush=True)
    s.close()


if __name__ == "__main__":
    main()
