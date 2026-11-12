from PIL import Image
import os

src = r"C:\Users\Smartup\.cursor\projects\c-Users-Smartup-Desktop-ARGE-Hexnet-HexnetWorkspace\assets\c__Users_Smartup_AppData_Roaming_Cursor_User_workspaceStorage_empty-window_images_ChatGPT_Image_26_Tem_2026_16_29_51-e5e6b72f-ff77-4688-9050-884325364297.png"
out_dir = os.path.dirname(os.path.abspath(__file__))
im = Image.open(src).convert("RGBA")
w, h = im.size
pix = im.load()
for y in range(h):
    for x in range(w):
        r, g, b, a = pix[x, y]
        if r < 22 and g < 22 and b < 22:
            pix[x, y] = (0, 0, 0, 0)

# Precise boxes from content scan (1024x682)
cells = {
    "light": (200, 55, 320, 195),
    "power": (450, 55, 570, 195),
    "water": (700, 55, 830, 195),
    "thermo": (200, 250, 320, 380),
    "sensor": (450, 250, 580, 380),
    "settings": (700, 250, 830, 380),
    "battery": (110, 470, 270, 545),  # icon only-ish
    "drop_clean": (470, 470, 560, 575),
    "drop_waste": (620, 470, 710, 575),
    "drop_grey": (820, 470, 910, 575),
}

for name, box in cells.items():
    crop = im.crop(box)
    bbox = crop.getbbox()
    if bbox:
        crop = crop.crop(bbox)
    if name.startswith("drop"):
        # Prefer icon without % text: keep top portion if tall
        if crop.height > crop.width * 1.4:
            crop = crop.crop((0, 0, crop.width, int(crop.width * 1.35)))
        crop.thumbnail((48, 48), Image.Resampling.LANCZOS)
    elif name == "battery":
        crop.thumbnail((64, 40), Image.Resampling.LANCZOS)
    else:
        crop.thumbnail((80, 80), Image.Resampling.LANCZOS)
    crop.save(os.path.join(out_dir, f"{name}.png"))
    print(name, crop.size)


def rgba_to_lvgl_c(png_path, var_name, out_c):
    img = Image.open(png_path).convert("RGBA")
    iw, ih = img.size
    data = bytearray()
    for y in range(ih):
        for x in range(iw):
            r, g, b, a = img.getpixel((x, y))
            c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            data.append(c & 0xFF)
            data.append((c >> 8) & 0xFF)
            data.append(a)

    with open(out_c, "w", encoding="utf-8") as f:
        f.write('#include "lvgl.h"\n\n')
        f.write("#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n\n")
        f.write(f"const LV_ATTRIBUTE_MEM_ALIGN uint8_t {var_name}_map[] = {{\n")
        for i in range(0, len(data), 24):
            chunk = ", ".join(f"0x{b:02X}" for b in data[i : i + 24])
            f.write(f"  {chunk},\n")
        f.write("};\n\n")
        f.write(f"const lv_img_dsc_t {var_name} = {{\n")
        f.write("  .header.always_zero = 0,\n")
        f.write(f"  .header.w = {iw},\n")
        f.write(f"  .header.h = {ih},\n")
        f.write(f"  .data_size = sizeof({var_name}_map),\n")
        f.write("  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n")
        f.write(f"  .data = {var_name}_map\n")
        f.write("};\n")
    print("wrote", var_name, iw, ih)


for n in cells:
    rgba_to_lvgl_c(
        os.path.join(out_dir, f"{n}.png"),
        f"vango_icon_{n}",
        os.path.join(out_dir, f"vango_icon_{n}.c"),
    )
