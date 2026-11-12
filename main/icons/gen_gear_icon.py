import math
import os
from PIL import Image, ImageChops, ImageDraw

out_dir = os.path.dirname(os.path.abspath(__file__))


def draw_gear(size, teeth=8):
    im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    cx = cy = size / 2
    outer = size * 0.46
    inner = size * 0.34
    hole = size * 0.14
    white = (255, 255, 255, 255)
    for i in range(teeth):
        a0 = (i / teeth) * math.tau - 0.18
        a1 = a0 + 0.36
        pts = []
        for a in (a0, a1):
            pts.append((cx + math.cos(a) * outer, cy + math.sin(a) * outer))
        a_back0 = a1
        a_back1 = ((i + 1) / teeth) * math.tau - 0.18
        pts.append((cx + math.cos(a_back0) * inner, cy + math.sin(a_back0) * inner))
        pts.append((cx + math.cos(a_back1) * inner, cy + math.sin(a_back1) * inner))
        d.polygon(pts, fill=white)
    d.ellipse((cx - inner, cy - inner, cx + inner, cy + inner), fill=white)
    d.ellipse((cx - hole, cy - hole, cx + hole, cy + hole), fill=(0, 0, 0, 0))
    return im


def write_c(im, var):
    iw, ih = im.size
    data = bytearray()
    for y in range(ih):
        for x in range(iw):
            r, g, b, a = im.getpixel((x, y))
            c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            data.append(c & 0xFF)
            data.append((c >> 8) & 0xFF)
            data.append(a)
    path = os.path.join(out_dir, var + ".c")
    with open(path, "w", encoding="utf-8") as f:
        f.write('#include "lvgl.h"\n\n')
        f.write("#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n\n")
        f.write(f"const LV_ATTRIBUTE_MEM_ALIGN uint8_t {var}_map[] = {{\n")
        for i in range(0, len(data), 24):
            chunk = ", ".join(f"0x{b:02X}" for b in data[i : i + 24])
            f.write(f"  {chunk},\n")
        f.write("};\n\n")
        f.write(f"const lv_img_dsc_t {var} = {{\n")
        f.write("  .header.always_zero = 0,\n")
        f.write(f"  .header.w = {iw},\n")
        f.write(f"  .header.h = {ih},\n")
        f.write(f"  .data_size = sizeof({var}_map),\n")
        f.write("  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n")
        f.write(f"  .data = {var}_map\n")
        f.write("};\n")
    print(var, iw, ih)


def draw_metal_gear(size=96, teeth=12):
    im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    cx = cy = size / 2.0
    outer = size * 0.48
    root = size * 0.34
    face = size * 0.30
    hub = size * 0.15
    hole = size * 0.055
    tooth_w = 0.18
    for i in range(teeth):
        a = (i / teeth) * math.tau
        shade = 128 + int(48 * math.cos(a - 0.8))
        col = (shade, min(255, shade + 6), min(255, shade + 14), 255)
        pts = []
        for ang, rad in (
            (a - tooth_w, root),
            (a - tooth_w * 0.45, outer),
            (a + tooth_w * 0.45, outer),
            (a + tooth_w, root),
        ):
            pts.append((cx + math.cos(ang) * rad, cy + math.sin(ang) * rad))
        d.polygon(pts, fill=col)
    d.ellipse((cx - root, cy - root, cx + root, cy + root), fill=(176, 184, 194, 255))
    d.ellipse((cx - root + 3, cy - root + 3, cx + root - 4, cy + root - 4), fill=(132, 140, 150, 255))
    d.ellipse((cx - face, cy - face, cx + face, cy + face), fill=(198, 206, 214, 255))
    d.ellipse((cx - face + 2, cy - face + 3, cx + face - 3, cy + face - 2), fill=(150, 158, 168, 255))
    d.ellipse((cx - hub, cy - hub, cx + hub, cy + hub), fill=(78, 86, 96, 255))
    for i in range(6):
        a = (i / 6.0) * math.tau + 0.35
        hx = cx + math.cos(a) * (face * 0.62)
        hy = cy + math.sin(a) * (face * 0.62)
        hr = size * 0.038
        d.ellipse((hx - hr, hy - hr, hx + hr, hy + hr), fill=(46, 52, 60, 255))
        d.ellipse((hx - hr + 1, hy - hr + 1, hx + hr - 2, hy + hr - 2), fill=(24, 28, 34, 255))
    alpha = im.getchannel("A")
    cut = Image.new("L", im.size, 0)
    ImageDraw.Draw(cut).ellipse((cx - hole, cy - hole, cx + hole, cy + hole), fill=255)
    im.putalpha(ImageChops.subtract(alpha, cut))
    return im


nav = draw_gear(40)
nav.save(os.path.join(out_dir, "nav_motor.png"))
write_c(nav, "vango_nav_motor")
gear = draw_gear(64, 8)
gear.save(os.path.join(out_dir, "gear.png"))
write_c(gear, "vango_icon_gear")
metal = draw_metal_gear(96, 12)
metal.save(os.path.join(out_dir, "gear_metal.png"))
write_c(metal, "vango_icon_gear_metal")
