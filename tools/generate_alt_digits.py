"""Rasterize GPL-2.0 Naive/Brutal PDC numeral paths by ir33k. Requires Pillow."""
from pathlib import Path
import struct
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
lines=['// Derived from ir33k/naive and ir33k/brutal PDC artwork. GPL-2.0.', '#pragma once']
for name,file in [('naive_hours','naive/font1.pdc'),('naive_minutes','naive/font2.pdc'),('brutal','brutal/digits.pdc')]:
 b=(ROOT/'resources'/file).read_bytes();assert b[:4]==b'PDCI'
 w,h,n=struct.unpack_from('<HHH',b,10);offset=16;glyphs=[]
 for _ in range(n):
  typ,hidden,stroke,sw,fill,param,count=struct.unpack_from('<BBBBBHH',b,offset);offset+=9
  pts=[struct.unpack_from('<hh',b,offset+4*i) for i in range(count)];offset+=count*4
  if typ==1:
   im=Image.new('1',(w,h));glyphs.append(im);draw=ImageDraw.Draw(im)
   draw.polygon(pts,fill=1)
  elif typ==2:
   x,y=pts[0];draw.ellipse((x-param,y-param,x+param,y+param),fill=1)
  else:raise ValueError('Unexpected command')
 assert len(glyphs)==10 and offset==len(b)
 for digit,im in enumerate(glyphs):
  width=[60,30,60,60,45,60,60,45,60,60][digit] if name=='brutal' else 60
  stride=(width+7)//8;bits=[0]*(stride*h)
  for y in range(h):
   for x in range(width):
    if im.getpixel((x,y)):bits[y*stride+x//8]|=1<<(x%8)
  key=f'{name}_{digit}'
  lines.append('static const uint8_t alt_bits_'+key+'[]={'+','.join(map(str,bits))+'};')
  lines.append(f'static const ColumnSprite alt_{key}={{{width},{h},{stride},alt_bits_{key}}};')
 lines.append('static const ColumnSprite *const '+name+'_digits[]={'+','.join(f'&alt_{name}_{d}' for d in range(10))+'};')
(ROOT/'src/c/alt_digits.h').write_text('\n'.join(lines)+'\n')
