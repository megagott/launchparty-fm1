"""Convert the approved transparent brand asset into bounded RGB565 ROM strips.
Only transparent padding is trimmed; the approved artwork is not redesigned.
"""
from pathlib import Path
from PIL import Image
import hashlib,json
root=Path(__file__).resolve().parents[1]
source=root/'branding/launchparty-logo-v1.png'
original=Image.open(source).convert('RGBA')
bounds=original.getchannel('A').point(lambda x:255 if x>=32 else 0).getbbox()
assert bounds
x0,y0,x1,y1=bounds;bounds=(max(0,x0-6),max(0,y0-6),min(original.width,x1+6),min(original.height,y1+6))
art=original.crop(bounds)
lines=['/* Generated from the logo selected by Lars. Do not redraw the brand. */','#ifndef LAUNCHPARTY_LOGO_H','#define LAUNCHPARTY_LOGO_H']
report={'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'crop':bounds,'variants':[]}
for index,width in enumerate([224,114]):
 height=round(art.height*width/art.width);assert height<48
 resized=art.resize((width,height),Image.Resampling.LANCZOS)
 opaque=Image.new('RGBA',resized.size,(0,0,0,255));opaque.alpha_composite(resized)
 pixels=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b,a in opaque.getdata()]
 runs=[];p=0
 while p<len(pixels):
  end=p+1
  while end<len(pixels) and end-p<255 and pixels[end]==pixels[p]:end+=1
  runs.extend((end-p,pixels[p]>>8,pixels[p]&255));p=end
 check=[]
 for n in range(0,len(runs),3):check.extend([(runs[n+1]<<8)|runs[n+2]]*runs[n])
 assert check==pixels
 lines+=['#define LP_LOGO_%d_W %du'%(index,width),'#define LP_LOGO_%d_H %du'%(index,height),'static const uint8_t lp_logo_%d[]={ '%index]
 lines += [','.join(str(v) for v in runs[n:n+48])+',' for n in range(0,len(runs),48)]
 lines+=['};'];report['variants'].append({'width':width,'height':height,'rom_bytes':len(runs)})
lines+=['static void lp_logo_draw(uint32_t small,uint32_t x,uint32_t y) {',
 ' const uint8_t *data=small?lp_logo_1:lp_logo_0;uint32_t w=small?LP_LOGO_1_W:LP_LOGO_0_W,h=small?LP_LOGO_1_H:LP_LOGO_0_H;',
 ' uint32_t size=small?sizeof(lp_logo_1):sizeof(lp_logo_0),i,p=0,j,n;uint16_t c;',
 ' cv_begin(w,h,C_BLACK);',
 ' for(i=0;i+2<size;i+=3){n=data[i];c=(uint16_t)((uint32_t)data[i+1]<<8|data[i+2]);for(j=0;j<n && p<w*h;j++)cv_px[p++]=swap16(c);}',
 ' cv_blit(x,y);lcd_sync();','}','#endif']
(root/'firmware/src/launchparty_logo.h').write_text('\n'.join(lines)+'\n')
(root/'branding/rom-conversion.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
