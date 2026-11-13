"""Рисует иконку Rework Time и сохраняет её в .ico для exe и ярлыков."""
import math
import os

from PIL import Image, ImageDraw

OUT = r'C:\KNOT\REWORK-TIME\assets'
os.makedirs(OUT, exist_ok=True)

BG = (16, 21, 32, 255)
RING = (77, 107, 254, 255)
GOLD = (229, 192, 123, 255)
GREEN = (152, 195, 121, 255)

base = 256
img = Image.new('RGBA', (base, base), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

# скруглённый квадрат
d.rounded_rectangle((8, 8, base - 8, base - 8), radius=56, fill=BG, outline=RING, width=6)

cx = cy = base / 2
r = 74
d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=RING, width=12)

# стрелки: часовая вверх, минутная вправо-вниз
d.line((cx, cy, cx, cy - 44), fill=GOLD, width=12)
d.line((cx, cy, cx + 38, cy + 22), fill=GREEN, width=12)
d.ellipse((cx - 9, cy - 9, cx + 9, cy + 9), fill=RING)

sizes = [256, 128, 64, 48, 32, 16]
with_downsized = [img.resize((s, s), Image.LANCZOS) for s in sizes]
png_path = os.path.join(OUT, 'reworktime-256.png')
img.save(png_path)

ico_path = os.path.join(OUT, 'reworktime.ico')
with_downsized[0].save(ico_path, format='ICO', sizes=[(s, s) for s in sizes],
                       append_images=with_downsized[1:])
print('saved', ico_path, os.path.getsize(ico_path), 'bytes')
print('saved', png_path)
