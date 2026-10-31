#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Rasterise Mandalars' original SVG logo with librsvg, preserving aspect ratio.
Requires Pillow and rsvg-convert (Ubuntu package librsvg2-bin).
Artwork from mandalars.org, supplied for this firmware by its owner.
"""
from pathlib import Path
from io import BytesIO
import subprocess,sys
from PIL import Image

assets=Path(__file__).resolve().parent.parent/'assets/mycelium'
def svg(name,width):
    return Image.open(BytesIO(subprocess.check_output(['rsvg-convert','-w',str(width),str(assets/name)]))).convert('RGBA')
im=Image.new('RGB',(240,188),'black')
symbol=svg('mycelium-symbol.svg',124)
word=svg('mycelium-logo.svg',232)
im.paste(symbol,(58,4),symbol)
im.paste(word,(4,132),word)
q=im.quantize(colors=16,method=Image.Quantize.MEDIANCUT)
pal=q.getpalette();cols=[tuple(pal[k*3:k*3+3]) for k in range(16)]
px=list(q.getdata());rle=[];i=0
while i<len(px):
    j=i+1
    while j<len(px) and px[j]==px[i] and j-i<16:j+=1
    rle.append(((j-i-1)<<4)|px[i]);i=j
rgb565=lambda c:((c[0]>>3)<<11)|((c[1]>>2)<<5)|(c[2]>>3)
lines=['/* generated from original Mandalars SVG artwork */','#pragma once','#include <stdint.h>',
       '#define MY_LOGO_W 240','#define MY_LOGO_H 188',
       'static const uint16_t MY_LOGO_PAL[16] = {'+', '.join(hex(rgb565(c)) for c in cols)+'};',
       'static const uint8_t MY_LOGO_RLE[] = {']
for k in range(0,len(rle),24):lines.append('  '+', '.join(map(str,rle[k:k+24]))+',')
lines.append('};');Path(sys.argv[1]).write_text('\n'.join(lines)+'\n')
print(f'Mycelium original logo: {len(rle)} bytes RLE, 240x188')
