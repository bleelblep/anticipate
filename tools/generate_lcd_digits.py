"""Encode the lit segments of Dalpek's Big-LCD digit tiles; requires Pillow."""
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
lines=['// Lit segments of the Big-LCD digit tiles by Dalpek; see tools/generate_lcd_digits.py.', '#pragma once']
for digit in range(10):
 im=Image.open(ROOT/'resources/biglcd'/f'NUM_{digit}.png').convert('RGBA')
 stride=(im.width+7)//8;bits=[0]*(stride*im.height)
 for y in range(im.height):
  for x in range(im.width):
   r,g,b,a=im.getpixel((x,y))
   # Tiles are black lit segments, dotted grey ghosts and white chamfers; keep only lit.
   if a>=128 and r<128:bits[y*stride+x//8]|=1<<(x%8)
 lines.append(f'static const uint8_t lcd_bits_{digit}[]={{'+','.join(map(str,bits))+'};')
 lines.append(f'static const ColumnSprite lcd_{digit}={{{im.width},{im.height},{stride},lcd_bits_{digit}}};')
lines.append('static const ColumnSprite *const lcd_digits[]={'+','.join(f'&lcd_{d}' for d in range(10))+'};')
(ROOT/'src/c/lcd_digits.h').write_text('\n'.join(lines)+'\n')
