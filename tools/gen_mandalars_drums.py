#!/usr/bin/env python3
"""Original procedural Mandalars drum assets. No external recordings or sample inputs.
Reproducible oscillator/noise synthesis; fixed seeds; mono PCM16 at 44.1 kHz.
"""
import math, random, wave, struct, json, hashlib
from pathlib import Path
BASE=Path(__file__).resolve().parents[1]/'assets/mycelium/samples'
SR=44100
def sound(role,kit):
    i=['PSY','FOREST','HITECH'].index(kit); rng=random.Random(8317+i*191+sum(map(ord,role)))
    dur={'KICK':.28,'HAT':.105,'CLAP':.15,'PERC':.13,'SNARE':.14}[role]
    phase=0.; lp=0.; a=[]
    for n in range(int(SR*dur)):
        t=n/SR; noise=rng.uniform(-1,1); lp+=.16*(noise-lp); hp=noise-lp
        if role=='KICK':
            f=[64,58,70][i]+[150,195,260][i]*math.exp(-t/[.012,.018,.007][i])
            phase+=2*math.pi*f/SR
            body=math.sin(phase)*math.exp(-t/[.068,.085,.045][i])
            click=hp*math.exp(-t/.0017)*[.18,.10,.25][i]
            v=math.tanh([1.55,1.8,1.4][i]*(body+click))
        elif role=='HAT':
            metal=sum(math.sin(2*math.pi*f*t) for f in [3211+137*i,4871+211*i,7333-97*i])/3
            v=(hp*.7+metal*.3)*math.exp(-t/[.019,.032,.012][i])
        elif role=='CLAP':
            burst=sum(math.exp(-(t-b)/.004) for b in [0,.009+i*.001,.019+i*.002] if t>=b)
            v=(noise-lp*.8)*(burst*.45+math.exp(-t/[.027,.038,.019][i])*.35)
        elif role=='SNARE':
            v=(math.sin(2*math.pi*(175+i*31)*t)*.35*math.exp(-t/.03)+hp*.7*math.exp(-t/[.035,.044,.024][i]))
        else:
            phase+=2*math.pi*(240+i*110+400*math.exp(-t/.01))/SR
            v=math.sin(phase+1.4*math.sin(phase*1.71))*math.exp(-t/.025)+hp*.08*math.exp(-t/.006)
        v*=min(1,t/.0002)*min(1,(dur-t)/.008)
        a.append(v)
    pk=max(map(abs,a)); return [round(v/pk*.78*32767) for v in a]
def write(file,role,kit):
    samples=sound(role,kit)
    with wave.open(str(BASE/file),'wb') as w:
        w.setparams((1,2,SR,len(samples),'NONE','not compressed')); w.writeframes(struct.pack('<'+'h'*len(samples),*samples))
    return {'file':file,'role':role,'kit_name':kit,'sr':SR,'duration':len(samples)/SR,'sha256':hashlib.sha256((BASE/file).read_bytes()).hexdigest(),'source':'Original procedural synthesis: tools/gen_mandalars_drums.py; no external audio input'}
def main():
    BASE.mkdir(parents=True,exist_ok=True)
    old=json.loads((BASE/'DRUM-SOURCES.json').read_text())['entries']
    entries=[]; assets=[]
    for kit in ['PSY','FOREST','HITECH']:
        for role in ['KICK','HAT','CLAP','PERC','SNARE']:
            file=f'{kit}-{role}.wav'; e=write(file,role,kit); assets.append(e)
            match=next((x for x in old if x.get('file')==file),None)
            if match: entries.append(dict(e,kit=match['kit'],note=match['note']))
    (BASE/'DRUM-SOURCES.json').write_text(json.dumps({'entries':entries},indent=2),encoding='utf-8')
    (BASE/'SOURCE.json').write_text(json.dumps(next(x for x in assets if x['file']=='FOREST-KICK.wav'),indent=2),encoding='utf-8')
    (BASE/'ORIGINAL-SOUNDS.json').write_text(json.dumps({'design':'Mandalars original procedural drum collection','audio_inputs':[],'assets':assets},indent=2),encoding='utf-8')
    legacy=BASE/'LTWB-KICK.wav'
    if legacy.exists(): legacy.unlink()
    print('15 original drum assets generated; manifest hashes recorded.')
if __name__=='__main__':main()
