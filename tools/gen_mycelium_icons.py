#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Original Mycelium pixel icon drawings from geometric primitives.
No original atlas, external pictures or font glyphs are read. 12px / 2-bit format.
"""
from pathlib import Path
import math,json,hashlib
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
A=ROOT/'assets';meta=json.loads((A/'icons.json').read_text());names=meta['names']
def glyph(name):
 im=Image.new('L',(12,12));d=ImageDraw.Draw(im)
 def line(p,ink=255):d.line(p,fill=ink,width=1)
 def dot(x,y):d.rectangle((x,y,x+1,y+1),fill=255)
 def box(p):d.rectangle(p,outline=255)
 def root():line([(6,6),(6,9),(3,11)],170);line([(6,9),(9,11)],170)
 wave=name.removeprefix('w_')
 if name.startswith('w_') or name in ['wave','lfo_wave','vibrato']:
  points=[]
  for x in range(1,11):
   t=(x-1)/9
   if wave in ['tri','rtri']:y=8-6*(1-abs(2*t-1))
   elif wave in ['saw','rsaw']:y=8-6*t
   elif wave in ['sqr','pls','pwm','spls']:y=2 if t<(.3 if wave in ['pls','spls'] else .5) else 8
   elif wave=='sh':y=[7,7,3,3,6,6,2,2,5,5][x-1]
   elif wave=='rtrp':y=8-6*min(1,t*3,(1-t)*3)
   else:y=5-round(3*math.sin(t*math.pi*2))
   points.append((x,round(y)))
  line(points);dot(5,10)
 elif name in ['attack','decay','sustain','release','env','fade']:
  paths={'attack':[(1,9),(7,2),(10,2)],'decay':[(1,2),(8,8),(10,8)],'sustain':[(1,3),(4,7),(10,7)],'release':[(1,3),(4,3),(10,9)],'env':[(1,9),(3,2),(5,6),(8,6),(10,9)],'fade':[(1,2),(10,9)]}
  line(paths[name]);root()
 elif name in ['generic','algorithm','mod','order','mix','chorus','voice','trio','ratio']:
  nodes=[(2,2),(8,2),(5,8)]
  if name in ['mix','chorus']:nodes=[(1,3),(8,3),(5,8)]
  if name=='order':nodes=[(1,1),(5,5),(9,9)]
  for x,y in nodes:dot(x,y)
  line([(3,4),(6,6),(9,4)],170);line([(6,6),(6,8)],170)
  if name in ['voice','trio']:line([(1,10),(5,10),(10,10)],85)
  if name=='ratio':box((5,0,6,1))
 elif name in ['cutoff','reso','tone','damp','sweep']:
  line([(1,3),(5,3),(7,5),(10,9)])
  if name=='reso':line([(4,3),(6,1),(8,5)])
  if name=='damp':line([(2,7),(4,7),(6,9),(9,9)],170)
  if name=='sweep':line([(2,10),(9,10),(7,8)],170)
  if name=='tone':dot(9,1)
 elif name in ['dist','drive','fold','shape','pulse','gate']:
  line([(1,8),(3,8),(3,2),(6,2),(6,8),(8,8),(8,4),(10,4)])
  if name=='dist':line([(1,3),(3,5),(5,3),(7,5),(9,3)],170)
  if name=='drive':line([(5,10),(9,10),(7,8)],170)
  if name=='fold':line([(1,2),(5,7),(9,2)],170)
  if name=='shape':dot(5,10)
  if name=='gate':line([(1,0),(1,10)],85)
 elif name in ['pitch','tune','transpose','octave','glide','slide','keytrack','scale','quantize']:
  line([(2,9),(5,9),(5,5),(8,5),(8,1),(10,1)])
  if name in ['glide','slide']:line([(1,9),(10,1)],170)
  if name=='tune':line([(1,1),(3,3),(1,5)],170)
  if name=='octave':dot(1,0);dot(9,9)
  if name=='transpose':line([(1,6),(1,1),(3,3)],170)
  if name=='keytrack':box((0,0,3,2))
  if name=='scale':line([(1,11),(10,11)],85)
  if name=='quantize':dot(0,5)
 elif name in ['level','pan','mute','sub','accent']:
  for x,h in [(2,3),(5,6),(8,9)]:line([(x,10),(x,10-h)])
  if name=='pan':line([(1,1),(10,1),(8,3)],170)
  if name=='mute':line([(0,1),(11,10)],255)
  if name=='sub':line([(1,11),(10,11)],170)
  if name=='accent':dot(1,0)
 elif name in ['delay','reverb','feedback','loop','hold','size']:
  if name=='reverb':
   for x in [2,5,8]:line([(x,2),(x+1,6),(x,10)],255-(x//3)*85)
  else:
   line([(2,3),(5,1),(9,3),(10,7),(7,10),(3,9),(1,6),(2,3)])
   if name=='delay':dot(5,4);dot(8,7)
   if name=='feedback':line([(1,6),(4,6),(2,8)])
   if name=='loop':line([(8,1),(8,4),(10,2)])
   if name=='hold':line([(4,4),(4,8)]);line([(7,4),(7,8)])
   if name=='size':line([(3,7),(8,3)],170)
 elif name in ['bits','noise','grain','chip']:
  coords={'bits':[(1,1),(5,1),(9,1),(1,5),(5,5),(9,9)],'noise':[(1,2),(4,0),(8,3),(2,6),(6,7),(10,10),(1,10)],'grain':[(3,2),(7,2),(1,6),(5,6),(9,6),(3,10),(7,10)],'chip':[(3,3),(7,3),(3,7),(7,7)]}
  for x,y in coords[name]:dot(x,y)
  if name=='chip':box((2,2,9,9));line([(0,4),(2,4)]);line([(9,7),(11,7)])
 elif name in ['tempo','rate','time','phase','swing','division','length','steps','arp']:
  if name in ['steps','arp','division','length']:
   for x,y in [(1,8),(4,5),(7,2),(9,5)]:dot(x,y)
   if name=='arp':line([(1,7),(8,0)],170)
   if name=='length':line([(1,11),(10,11)],170)
   if name=='division':line([(5,0),(5,11)],85)
  else:
   d.ellipse((1,1,10,10),outline=170);line([(6,6),(6,2)]);line([(6,6),(9,7)])
   if name=='swing':line([(1,11),(4,9),(8,11)],255)
   if name=='rate':dot(9,0)
   if name=='tempo':dot(0,0)
   if name=='phase':line([(1,8),(10,3)],85)
 elif name in ['prob','detune']:
  line([(6,10),(6,5),(2,1)],170);line([(6,5),(10,1)],170)
  dot(1,0);dot(9,0)
  if name=='prob':dot(5,4);dot(8,8)
  else:line([(4,9),(2,6)],255)
 elif name in ['save','load','clear','sample','tape','slice','drum','mouth','drawbar','midi']:
  if name in ['save','load']: 
   box((1,7,10,10));line([(6,1),(6,7)]);line([(3,4),(6,7),(9,4)] if name=='save' else [(3,4),(6,1),(9,4)])
  elif name=='clear':line([(2,2),(9,9)]);line([(9,2),(2,9)]);dot(0,9)
  elif name=='sample':box((1,1,10,10));line([(2,6),(4,3),(6,8),(8,4),(9,6)])
  elif name=='tape':box((0,2,11,9));d.ellipse((2,4,4,6),outline=255);d.ellipse((7,4,9,6),outline=255)
  elif name=='slice':box((1,2,10,9));line([(5,0),(5,11)])
  elif name=='drum':d.ellipse((1,1,10,5),outline=255);line([(1,3),(1,9),(10,9),(10,3)]);line([(3,5),(5,9),(7,5)])
  elif name=='mouth':line([(1,5),(4,3),(7,3),(10,5),(8,8),(3,8),(1,5)])
  elif name=='drawbar':
   for x,y in [(2,3),(5,7),(8,5)]:line([(x,1),(x,10)],170);box((x-1,y,x+1,y+1))
  else:d.ellipse((1,1,10,10),outline=170);dot(3,3);dot(7,3);dot(5,7)
 else:raise ValueError(name)
 return im
def main():
 cols=meta['cols'];atlas=Image.new('L',(cols*12,math.ceil(len(names)/cols)*12))
 all_icons=[]
 for i,name in enumerate(names):
  im=glyph(name);assert im.getbbox(),name;all_icons.append(im);atlas.paste(im,(i%cols*12,i//cols*12))
 atlas.save(A/'icons.png')
 proof={'design':'Original Mycelium geometric pixel drawings','generator':'tools/gen_mycelium_icons.py','external_image_inputs':[],'cell':12,'levels':[0,85,170,255],'names':names,'sha256':hashlib.sha256((A/'icons.png').read_bytes()).hexdigest(),'license':'GPL-3.0-only'}
 (A/'MYCELIUM-ICONS.json').write_text(json.dumps(proof,indent=2),encoding='utf-8')
 (A/'MYCELIUM-ICONS-LICENSE.txt').write_text('Mycelium icon atlas\nOriginal geometric pixel drawings generated by tools/gen_mycelium_icons.py.\nNo external pictures, fonts or original atlas pixels used as inputs.\nAtlas and generator distributed under GPL-3.0-only; full text in ../LICENSE.\nUpstream firmware code and its notices retain their respective attribution.\n',encoding='utf-8')
 # A readable review sheet uses the actual new 12px glyphs, enlarged without smoothing.
 out=ROOT.parent/'Mycelium-1.0-Eigene-Icons.png';scale=5;cw=160;ch=103;sheet=Image.new('RGB',(cw*8,110+ch*math.ceil(len(names)/8)),'#080f0c');dr=ImageDraw.Draw(sheet)
 font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',14);title=ImageFont.truetype('C:/Windows/Fonts/segoeuib.ttf',32)
 dr.text((25,18),'MYCELIUM / ORIGINAL PIXEL ICONS',font=title,fill='#a0ff20');dr.text((25,62),f'{len(names)} eigene Symbole / 12 x 12 Pixel / Myzel, Sporen und Signal',font=font,fill='#a5b8ac')
 colors=['#a0ff20','#00ffc0','#f0ff00','#ff20c4']
 for i,(name,im) in enumerate(zip(names,all_icons)):
  x=i%8*cw+25;y=i//8*ch+105
  fg=Image.new('RGB',(60,60),colors[i%4]);mask=im.resize((60,60),Image.Resampling.NEAREST)
  sheet.paste(fg,(x,y),mask);dr.text((x,y+65),name,font=font,fill='#e5ede7')
 sheet.save(out);print(json.dumps({'count':len(names),'sha256':proof['sha256'],'preview':str(out)}))
if __name__=='__main__':main()
