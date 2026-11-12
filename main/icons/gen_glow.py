from PIL import Image, ImageFilter
import math
import os

out = os.path.dirname(os.path.abspath(__file__))
size = 96
cx = cy = size // 2
img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
px = img.load()
for y in range(size):
    for x in range(size):
        d = math.sqrt((x - cx) ** 2 + (y - cy) ** 2) / (size * 0.48)
        if d >= 1:
            a = 0
        else:
            a = int(180 * (1 - d) ** 2.2)
        px[x, y] = (255, 255, 255, a)

img = img.filter(ImageFilter.GaussianBlur(radius=6))
px = img.load()
for y in range(size):
    for x in range(size):
        r, g, b, a = px[x, y]
        px[x, y] = (255, 255, 255, min(255, int(a * 1.15)))

img.save(os.path.join(out, "glow_soft.png"))

data = bytearray()
for y in range(size):
    for x in range(size):
        r, g, b, a = img.getpixel((x, y))
        c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        data.append(c & 0xFF)
        data.append((c >> 8) & 0xFF)
        data.append(a)

var = "vango_glow_soft"
with open(os.path.join(out, f"{var}.c"), "w", encoding="utf-8") as f:
    f.write('#include "lvgl.h"\n\n')
    f.write("#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n\n")
    f.write(f"const LV_ATTRIBUTE_MEM_ALIGN uint8_t {var}_map[] = {{\n")
    for i in range(0, len(data), 24):
        chunk = ", ".join(f"0x{b:02X}" for b in data[i : i + 24])
        f.write(f"  {chunk},\n")
    f.write("};\n\n")
    f.write(f"const lv_img_dsc_t {var} = {{\n")
    f.write("  .header.always_zero = 0,\n")
    f.write(f"  .header.w = {size},\n")
    f.write(f"  .header.h = {size},\n")
    f.write(f"  .data_size = sizeof({var}_map),\n")
    f.write("  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n")
    f.write(f"  .data = {var}_map\n")
    f.write("};\n")

print(var, size, "bytes", len(data))
