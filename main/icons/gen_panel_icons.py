#!/usr/bin/env python3
"""Tablet panel_icon_*.png → LVGL ALPHA_8BIT 96x96 (home) + 40x40 (settings).

ALPHA_8BIT zoom must stay 256 on device — smaller UI uses the 40px set.
"""
from __future__ import annotations

import re
from collections import deque
from pathlib import Path

from PIL import Image, ImageFilter

SIZE_LG = 96
SIZE_SM = 40
HI = 256
INK_THR = 85
MIN_BLOB = 18

HERE = Path(__file__).resolve().parent
OUT = HERE / "panel"
SRC_ROOTS = [
    Path(
        r"C:\Users\Smartup\Desktop\ARGE\Hexnet\HexnetWorkspace"
        r"\VanGO\VanGO-v0.91-001\composeApp\src\commonMain\composeResources"
    ),
    Path(
        r"C:\Users\Smartup\Desktop\ARGE\Hexnet\HexnetWorkspace"
        r"\VanGO\VanGO-v0.90-001\composeApp\src\commonMain\composeResources"
    ),
]


def find_sources() -> list[Path]:
    found: dict[str, Path] = {}
    for root in SRC_ROOTS:
        if not root.is_dir():
            continue
        for p in root.rglob("panel_icon_*.png"):
            key = p.stem.replace("panel_icon_", "", 1)
            prev = found.get(key)
            if prev is None or p.stat().st_size > prev.stat().st_size:
                found[key] = p
    return [found[k] for k in sorted(found)]


def to_binary(im: Image.Image) -> Image.Image:
    im = im.convert("RGBA")
    w, h = im.size
    px = im.load()
    mask = Image.new("L", (w, h), 0)
    mp = mask.load()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 12:
                continue
            lum = (77 * r + 150 * g + 29 * b) >> 8
            if lum < 40:
                continue
            if (a * lum) // 255 >= INK_THR:
                mp[x, y] = 255
    return mask


def remove_small_blobs(mask: Image.Image, min_size: int) -> Image.Image:
    w, h = mask.size
    src = mask.load()
    seen = [[False] * w for _ in range(h)]
    out = Image.new("L", (w, h), 0)
    op = out.load()
    for y in range(h):
        for x in range(w):
            if src[x, y] == 0 or seen[y][x]:
                continue
            q = deque([(x, y)])
            seen[y][x] = True
            comp = [(x, y)]
            while q:
                cx, cy = q.popleft()
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = cx + dx, cy + dy
                    if nx < 0 or ny < 0 or nx >= w or ny >= h:
                        continue
                    if seen[ny][nx] or src[nx, ny] == 0:
                        continue
                    seen[ny][nx] = True
                    q.append((nx, ny))
                    comp.append((nx, ny))
            if len(comp) >= min_size:
                for cx, cy in comp:
                    op[cx, cy] = 255
    return out


def finalize_size(merged: Image.Image, size: int) -> bytes:
    final = merged.resize((size, size), Image.Resampling.BOX)
    final = final.point(
        lambda v: 0 if v < 28 else (255 if v >= 200 else int(90 + (v - 28) * (110 / 172)))
    )
    fp = final.load()
    cleaned = Image.new("L", (size, size), 0)
    cp = cleaned.load()
    for y in range(size):
        for x in range(size):
            v = fp[x, y]
            if v == 0:
                continue
            n = 0
            for dy in (-1, 0, 1):
                yy = y + dy
                if yy < 0 or yy >= size:
                    continue
                for dx in (-1, 0, 1):
                    if dx == 0 and dy == 0:
                        continue
                    xx = x + dx
                    if 0 <= xx < size and fp[xx, yy] >= 28:
                        n += 1
            if n >= 1:
                cp[x, y] = v
    return bytes(cleaned.tobytes())


def fallback_alpha(im: Image.Image, size: int) -> bytes:
    im = im.convert("RGBA")
    bb = im.getbbox() or (0, 0, im.width, im.height)
    crop = im.crop(bb)
    crop.thumbnail((size, size), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    canvas.paste(
        crop,
        ((size - crop.size[0]) // 2, (size - crop.size[1]) // 2),
        crop,
    )
    out = bytearray()
    for y in range(size):
        for x in range(size):
            r, g, b, a = canvas.getpixel((x, y))
            if a < 8:
                out.append(0)
                continue
            lum = (77 * r + 150 * g + 29 * b) >> 8
            ink = (a * max(lum, 255 - lum)) // 255
            out.append(0 if ink < 24 else min(255, ink))
    return bytes(out)


def process_icon(src: Path) -> tuple[bytes, bytes]:
    im = Image.open(src).convert("RGBA")
    bb = im.getbbox()
    if not bb:
        return bytes(SIZE_LG * SIZE_LG), bytes(SIZE_SM * SIZE_SM)
    cw, ch = bb[2] - bb[0], bb[3] - bb[1]
    pad = max(12, int(max(cw, ch) * 0.08))
    bb = (
        max(0, bb[0] - pad),
        max(0, bb[1] - pad),
        min(im.width, bb[2] + pad),
        min(im.height, bb[3] + pad),
    )
    crop = im.crop(bb)
    scaled = crop.copy()
    scaled.thumbnail((HI, HI), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (HI, HI), (0, 0, 0, 0))
    canvas.paste(
        scaled,
        ((HI - scaled.size[0]) // 2, (HI - scaled.size[1]) // 2),
        scaled,
    )

    mask = to_binary(canvas)
    mask = remove_small_blobs(mask, MIN_BLOB)
    mask = mask.filter(ImageFilter.MaxFilter(5))
    mask = remove_small_blobs(mask, 8)

    soft = mask.filter(ImageFilter.GaussianBlur(radius=0.7))
    mp, sp = mask.load(), soft.load()
    w, h = mask.size
    merged = Image.new("L", (w, h), 0)
    op = merged.load()
    for y in range(h):
        for x in range(w):
            if mp[x, y]:
                op[x, y] = 255
            else:
                s = sp[x, y]
                if s >= 40:
                    op[x, y] = min(160, s)

    data_lg = finalize_size(merged, SIZE_LG)
    data_sm = finalize_size(merged, SIZE_SM)
    if sum(1 for b in data_lg if b) < 40:
        data_lg = fallback_alpha(im, SIZE_LG)
        data_sm = fallback_alpha(im, SIZE_SM)
    return data_lg, data_sm


def write_map(f, var: str, data: bytes) -> None:
    f.write(f"const LV_ATTRIBUTE_MEM_ALIGN uint8_t {var}_map[] = {{\n")
    for i in range(0, len(data), 24):
        chunk = ", ".join(f"0x{b:02X}" for b in data[i : i + 24])
        f.write(f"  {chunk},\n")
    f.write("};\n\n")


def write_dsc(f, var: str, size: int) -> None:
    f.write(f"const lv_img_dsc_t {var} = {{\n")
    f.write("  .header.always_zero = 0,\n")
    f.write(f"  .header.w = {size},\n")
    f.write(f"  .header.h = {size},\n")
    f.write(f"  .data_size = sizeof({var}_map),\n")
    f.write("  .header.cf = LV_IMG_CF_ALPHA_8BIT,\n")
    f.write(f"  .data = {var}_map\n")
    f.write("};\n")


def write_c(key: str, data_lg: bytes, data_sm: bytes) -> Path:
    var = re.sub(r"[^a-z0-9_]", "_", f"vango_panel_{key}")
    var_sm = f"{var}_sm"
    path = OUT / f"{var}.c"
    with path.open("w", encoding="utf-8") as f:
        f.write('#include "lvgl.h"\n\n')
        f.write("#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n\n")
        write_map(f, var, data_lg)
        write_dsc(f, var, SIZE_LG)
        f.write("\n")
        write_map(f, var_sm, data_sm)
        write_dsc(f, var_sm, SIZE_SM)
    return path


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for old in OUT.glob("vango_panel_*.c"):
        old.unlink()
    srcs = find_sources()
    if not srcs:
        raise SystemExit("no panel_icon_*.png found")

    keys: list[str] = []
    total = 0
    empty = 0
    for src in srcs:
        key = src.stem.replace("panel_icon_", "", 1)
        data_lg, data_sm = process_icon(src)
        nz = sum(1 for b in data_lg if b)
        if nz < 40:
            empty += 1
        path = write_c(key, data_lg, data_sm)
        keys.append(key)
        total += path.stat().st_size
        print(f"OK {key} nz={nz} <- {src.name}")

    hdr = HERE / "vango_panel_icons.h"
    with hdr.open("w", encoding="utf-8") as f:
        f.write("#pragma once\n#include \"lvgl.h\"\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
        for key in keys:
            var = re.sub(r"[^a-z0-9_]", "_", f"vango_panel_{key}")
            f.write(f"LV_IMG_DECLARE({var});\n")
            f.write(f"LV_IMG_DECLARE({var}_sm);\n")
        f.write("\nconst lv_img_dsc_t *vango_panel_icon_by_key(const char *key);\n")
        f.write("const lv_img_dsc_t *vango_panel_icon_sm_by_key(const char *key);\n\n")
        f.write("#ifdef __cplusplus\n}\n#endif\n")

    lookup = HERE / "vango_panel_icon_lookup.c"
    with lookup.open("w", encoding="utf-8") as f:
        f.write('#include "vango_panel_icons.h"\n#include <string.h>\n\n')
        f.write("const lv_img_dsc_t *vango_panel_icon_by_key(const char *key)\n{\n")
        f.write("    if (!key || !key[0]) {\n        return NULL;\n    }\n")
        for key in keys:
            var = re.sub(r"[^a-z0-9_]", "_", f"vango_panel_{key}")
            f.write(f'    if (strcmp(key, "{key}") == 0) {{\n        return &{var};\n    }}\n')
        f.write("    return NULL;\n}\n\n")
        f.write("const lv_img_dsc_t *vango_panel_icon_sm_by_key(const char *key)\n{\n")
        f.write("    if (!key || !key[0]) {\n        return NULL;\n    }\n")
        for key in keys:
            var = re.sub(r"[^a-z0-9_]", "_", f"vango_panel_{key}")
            f.write(f'    if (strcmp(key, "{key}") == 0) {{\n        return &{var}_sm;\n    }}\n')
        f.write("    return NULL;\n}\n")

    print(f"\n{len(keys)} icons (96+40), ~{total} B, empty/fallback={empty}")


if __name__ == "__main__":
    main()
