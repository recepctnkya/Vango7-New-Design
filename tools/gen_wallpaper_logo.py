"""Hexnet_logo_0.png → crisp screensaver logo (TRUE_COLOR_ALPHA, no LVGL zoom).

Ink is cyan; dark rings are transparent gaps (black wallpaper shows through).
"""
from __future__ import annotations

import shutil
from pathlib import Path

from PIL import Image

PROJ = Path(__file__).resolve().parents[1]
SRC_CANDIDATES = [
    Path(
        r"C:\Users\Smartup\.cursor\projects\c-Users-Smartup-Desktop-ARGE-Hexnet-HexnetWorkspace"
        r"\assets\Hexnet_logo_0-ac3ef3ba-229e-44ab-bdc6-a8c2c3f9c182.png"
    ),
    PROJ / "main" / "assets" / "Hexnet_logo_0.png",
]
ASSET = PROJ / "main" / "assets" / "Hexnet_logo_0.png"
OUT_C = PROJ / "main" / "icons" / "vango_wallpaper_logo.c"
OUT_PNG = PROJ / "main" / "assets" / "Hexnet_wallpaper_logo.png"
PREVIEW = PROJ / "tools" / "hexnet_wallpaper_logo.png"

SIZE = 320
CYAN = (0, 168, 243)
PAD = 14


def pick_src() -> Path:
    for p in SRC_CANDIDATES:
        if p.is_file():
            return p
    raise FileNotFoundError("Hexnet logo source not found")


def content_bbox(im: Image.Image) -> tuple[int, int, int, int]:
    pix = im.load()
    w, h = im.size
    x0, y0, x1, y1 = w, h, 0, 0
    found = False
    for y in range(h):
        for x in range(w):
            if pix[x, y][3] < 12:
                continue
            found = True
            x0 = min(x0, x)
            y0 = min(y0, y)
            x1 = max(x1, x)
            y1 = max(y1, y)
    if not found:
        return 0, 0, w - 1, h - 1
    return x0, y0, x1, y1


def prepare_square(im: Image.Image) -> Image.Image:
    x0, y0, x1, y1 = content_bbox(im)
    crop = im.crop((x0, y0, x1 + 1, y1 + 1))
    side = max(crop.size) + PAD * 2
    square = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    ox = (side - crop.size[0]) // 2
    oy = (side - crop.size[1]) // 2
    square.paste(crop, (ox, oy), crop)
    return square


def snap_cyan_keep_alpha(im: Image.Image) -> Image.Image:
    w, h = im.size
    src = im.load()
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    op = out.load()
    cr, cg, cb = CYAN
    for y in range(h):
        for x in range(w):
            a = src[x, y][3]
            if a < 8:
                continue
            op[x, y] = (cr, cg, cb, a)
    return out


def to_lvgl_rgb565a(im: Image.Image) -> bytes:
    w, h = im.size
    data = bytearray(w * h * 3)
    i = 0
    for y in range(h):
        for x in range(w):
            r, g, b, a = im.getpixel((x, y))
            c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            data[i] = c & 0xFF
            data[i + 1] = (c >> 8) & 0xFF
            data[i + 2] = a
            i += 3
    return bytes(data)


def main() -> None:
    src = pick_src()
    ASSET.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, ASSET)
    print("src", src)

    im = Image.open(ASSET).convert("RGBA")
    square = prepare_square(im)
    hi = square.resize((SIZE * 2, SIZE * 2), Image.Resampling.LANCZOS)
    scaled = hi.resize((SIZE, SIZE), Image.Resampling.LANCZOS)
    out = snap_cyan_keep_alpha(scaled)

    out.save(OUT_PNG)
    prev = Image.new("RGB", (SIZE, SIZE), (0, 0, 0))
    prev.paste(out, mask=out.split()[3])
    prev.save(PREVIEW)
    print("square", square.size, "->", SIZE)

    data = to_lvgl_rgb565a(out)
    var = "vango_wallpaper_logo"
    with OUT_C.open("w", encoding="utf-8") as f:
        f.write('#include "lvgl.h"\n\n')
        f.write("#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n\n")
        f.write(f"const LV_ATTRIBUTE_MEM_ALIGN uint8_t {var}_map[] = {{\n")
        for i in range(0, len(data), 24):
            chunk = ", ".join(f"0x{b:02X}" for b in data[i : i + 24])
            f.write(f"  {chunk},\n")
        f.write("};\n\n")
        f.write(f"const lv_img_dsc_t {var} = {{\n")
        f.write("  .header.always_zero = 0,\n")
        f.write(f"  .header.w = {SIZE},\n")
        f.write(f"  .header.h = {SIZE},\n")
        f.write(f"  .data_size = sizeof({var}_map),\n")
        f.write("  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n")
        f.write(f"  .data = {var}_map\n")
        f.write("};\n")
    print("wrote", OUT_C, "bytes", len(data))


if __name__ == "__main__":
    main()
