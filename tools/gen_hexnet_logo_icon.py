"""Scale Hexnet_logo_0.png → 34×34 LVGL TRUE_COLOR_ALPHA asset (aspect preserved)."""
from PIL import Image
import os
import shutil

SRC = r"C:\Users\Smartup\.cursor\projects\c-Users-Smartup-Desktop-ARGE-Hexnet-HexnetWorkspace\assets\Hexnet_logo_0-ac3ef3ba-229e-44ab-bdc6-a8c2c3f9c182.png"
PROJ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSET_DIR = os.path.join(PROJ, "main", "assets")
ICONS_DIR = os.path.join(PROJ, "main", "icons")
SIZE = 34


def main():
    os.makedirs(ASSET_DIR, exist_ok=True)
    dst_orig = os.path.join(ASSET_DIR, "Hexnet_logo_0.png")
    shutil.copy2(SRC, dst_orig)

    im = Image.open(dst_orig).convert("RGBA")
    pix = im.load()
    w, h = im.size
    black = 0
    trans = 0
    for y in range(h):
        for x in range(w):
            r, g, b, a = pix[x, y]
            if a == 0:
                trans += 1
            elif r < 12 and g < 12 and b < 12:
                black += 1
    print("size", w, h, "transparent", trans, "near_black_opaque", black)

    # Viewer may bake black behind transparent PNG — clear only if needed
    if trans < (w * h) // 10 and black > (w * h) // 5:
        for y in range(h):
            for x in range(w):
                r, g, b, a = pix[x, y]
                if r < 12 and g < 12 and b < 12:
                    pix[x, y] = (0, 0, 0, 0)
        print("cleared black background")

    im34 = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    scaled = im.copy()
    scaled.thumbnail((SIZE, SIZE), Image.Resampling.LANCZOS)
    ox = (SIZE - scaled.size[0]) // 2
    oy = (SIZE - scaled.size[1]) // 2
    im34.paste(scaled, (ox, oy), scaled)
    out_png = os.path.join(ASSET_DIR, "Hexnet_logo_0_34.png")
    im34.save(out_png)
    print("scaled", scaled.size, "padded", im34.size, "->", out_png)

    data = bytearray()
    for y in range(SIZE):
        for x in range(SIZE):
            r, g, b, a = im34.getpixel((x, y))
            c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            data.append(c & 0xFF)
            data.append((c >> 8) & 0xFF)
            data.append(a)

    var = "vango_icon_logo"
    out_c = os.path.join(ICONS_DIR, "vango_icon_logo.c")
    with open(out_c, "w", encoding="utf-8") as f:
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
    print("wrote", out_c, "bytes", len(data))


if __name__ == "__main__":
    main()
