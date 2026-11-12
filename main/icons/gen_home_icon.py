from PIL import Image, ImageDraw
import os

out_dir = os.path.dirname(os.path.abspath(__file__))
w, h = 40, 40
im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
d = ImageDraw.Draw(im)
yellow = (255, 214, 10, 255)
d.polygon([(20, 2), (2, 18), (38, 18)], fill=yellow)
d.rectangle([7, 16, 32, 38], fill=yellow)
d.rectangle([16, 25, 23, 38], fill=(0, 0, 0, 0))
d.rectangle([10, 21, 14, 26], fill=(0, 0, 0, 0))
im = im.crop(im.getbbox())
im.save(os.path.join(out_dir, "nav_home.png"))
iw, ih = im.size
data = bytearray()
for y in range(ih):
    for x in range(iw):
        r, g, b, a = im.getpixel((x, y))
        c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        data.append(c & 0xFF)
        data.append((c >> 8) & 0xFF)
        data.append(a)
var = "vango_nav_home"
with open(os.path.join(out_dir, var + ".c"), "w", encoding="utf-8") as f:
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
