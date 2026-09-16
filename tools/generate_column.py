"""Encode the original left-column artwork; requires Pillow."""
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
files=[]
for size in ['xs','s']:
 for n in range(10): files.append((size+str(n),f'num-{size}-{n}-light'+('-v2' if n==7 else '')+'.png'))
 files.append((size+'dash',f'num-{size}-dash-light.png'))
files.append(('degree','num-s-degree-light.png'))
for name,f in [('clear','clear-sun-light-v3'),('moon','clear-moon-light'),('partly','partly-cloudy-sun-light-v4'),('partlymoon','partly-cloudy-moon-light-v3'),('cloud','clouds-light-v3'),('drizzle','drizzle-light-v3'),('rain','rain-light-v3'),('snow','snow-light-v3'),('thunder','thunderstorm-light-v3'),('fog','atmosphere-light')]:
 files.append((name,'icon-condition-'+f+'.png'))
files.append(('background','background-static-v10-sunrise_set.png'))
lines=['// Generated from original Anticipate PNG artwork; see tools/generate_column.py.', '#pragma once', '#include <stdint.h>', 'typedef struct { uint8_t w,h,stride; const uint8_t *bits; } ColumnSprite;']
for name,f in files:
 im=Image.open(ROOT/'resources/images'/f).convert('RGBA')
 if name=='background': im=im.crop((4,4,40,164))
 stride=(im.width+7)//8;bits=[0]*(stride*im.height)
 for y in range(im.height):
  for x in range(im.width):
   r,g,b,a=im.getpixel((x,y))
   if a>=128 and r>=128:bits[y*stride+x//8]|=1<<(x%8)
 lines.append('static const uint8_t bits_'+name+'[]={'+','.join(map(str,bits))+'};')
 lines.append(f'static const ColumnSprite sprite_{name}={{{im.width},{im.height},{stride},bits_{name}}};')
for size in ['xs','s']:
 names=[size+str(i) for i in range(10)]+[size+'dash']
 if size=='s':names+=['degree']
 lines.append('static const ColumnSprite *const '+size+'_sprites[]={'+','.join('&sprite_'+n for n in names)+'};')
lines.append('static const ColumnSprite *const condition_sprites[]={'+','.join('&sprite_'+n for n in ['clear','moon','partly','partlymoon','cloud','drizzle','rain','snow','thunder','fog'])+'};')
(ROOT/'src/c/column_sprites.h').write_text('\n'.join(lines)+'\n')
