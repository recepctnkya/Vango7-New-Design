"""Tavan: leke temiz + HI binary → BOX 96 (doğal soft AA, hale yok)."""
from __future__ import annotations

from collections import deque
from pathlib import Path

from PIL import Image, ImageFilter

SIZE = 96
HI = 256
SRC = Path(
    r"C:\Users\Smartup\Desktop\ARGE\Hexnet\HexnetWorkspace"
    r"\VanGO\VanGO-v0.91-001\composeApp\src\commonMain\composeResources"
    r"\drawable\panel_icon_tavan_aydinlatma.png"
)
OUT_C = Path(
    r"C:\Users\Smartup\Desktop\ARGE\Hexnet\HexnetWorkspace"
    r"\Vango4.3\VanGO-Display7-v0-50\main\icons\panel\vango_panel_tavan_aydinlatma.c"
)
TOOLS = Path(__file__).resolve().parent

INK_THR = 110
MIN_BLOB = 22


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
            if lum < 55:
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


def write_c(data: bytes) -> None:
    var = "vango_panel_tavan_aydinlatma"
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
        f.write("  .header.cf = LV_IMG_CF_ALPHA_8BIT,\n")
        f.write(f"  .data = {var}_map\n")
        f.write("};\n")


def preview(final: Image.Image, path: Path, bg, fg) -> None:
    out = Image.new("RGB", final.size, bg)
    fp, op = final.load(), out.load()
    w, h = final.size
    for y in range(h):
        for x in range(w):
            a = fp[x, y]
            if not a:
                continue
            t = a / 255.0
            op[x, y] = (
                int(bg[0] * (1 - t) + fg[0] * t),
                int(bg[1] * (1 - t) + fg[1] * t),
                int(bg[2] * (1 - t) + fg[2] * t),
            )
    out.save(path)
    out.resize((w * 3, h * 3), Image.Resampling.BILINEAR).save(
        path.with_name(path.stem + "_x3.png")
    )


def main() -> None:
    im = Image.open(SRC).convert("RGBA")
    bb = im.getbbox()
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
    # Kalın yuvarlak stroke = daha soft görünüm; leke yok
    mask = mask.filter(ImageFilter.MaxFilter(5))
    mask = remove_small_blobs(mask, 8)
    # Çok hafif blur sadece stroke üzerinde (dışarı taşmasın)
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
                # Sadece çekirdeğe bitişik soft; uzak hale yok
                s = sp[x, y]
                if s >= 40:
                    op[x, y] = min(160, s)

    final = merged.resize((SIZE, SIZE), Image.Resampling.BOX)
    # Orta tonları sıkı tut: sis/leke kes, soft AA ince kalsın
    final = final.point(
        lambda v: 0 if v < 28 else (255 if v >= 200 else int(90 + (v - 28) * (110 / 172)))
    )
    # 96px izole lekeleri sil
    fp = final.load()
    cleaned = Image.new("L", (SIZE, SIZE), 0)
    cp = cleaned.load()
    for y in range(SIZE):
        for x in range(SIZE):
            v = fp[x, y]
            if v == 0:
                continue
            n = 0
            for dy in (-1, 0, 1):
                yy = y + dy
                if yy < 0 or yy >= SIZE:
                    continue
                for dx in (-1, 0, 1):
                    if dx == 0 and dy == 0:
                        continue
                    xx = x + dx
                    if 0 <= xx < SIZE and fp[xx, yy] >= 28:
                        n += 1
            if n >= 1:
                cp[x, y] = v
    final = cleaned

    data = bytes(final.tobytes())
    preview(final, TOOLS / "_tavan_new_96.png", (13, 23, 41), (240, 240, 240))
    preview(final, TOOLS / "_tavan_recolor_sim.png", (20, 60, 40), (80, 220, 100))
    write_c(data)
    print("wrote", OUT_C, "nonzero", sum(1 for b in data if b))


if __name__ == "__main__":
    main()
