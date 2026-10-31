"""Read Ableton lightshow clips without modifying the originals.

LPS2: duration uint32 LE, count uint32 LE, initial palette[128],
then repeated (delta_tick ULEB128, pad uint8, palette uint8).
960 ticks/quarter note. Original MIDI note numbers are retained, including
peripheral buttons. A separate optional map adapts grid notes 36..99 to
Programmer Mode. No guessed peripheral mapping is baked into this format.
"""
from pathlib import Path
import gzip, xml.etree.ElementTree as ET, json, struct, hashlib, argparse, math

PPQ = 960

def val(node, path, default='0'):
    n=node.find(path)
    return n.get('Value',default) if n is not None else default

def note_events(clip):
    events=[]; ignored=0; disabled=0
    for k in clip.findall('Notes/KeyTracks/KeyTrack'):
        note=int(val(k,'MidiKey'))
        for n in k.findall('Notes/MidiNoteEvent'):
            if n.get('IsEnabled','true')=='false':
                disabled+=1; continue
            if not 0<=note<=127:
                ignored+=1; continue
            if n.get('Probability','1')!='1':
                raise ValueError('Probabilistic light notes require rendered capture')
            t=round(float(n.get('Time'))*PPQ)
            end=round((float(n.get('Time'))+float(n.get('Duration')))*PPQ)
            color=max(0,min(127,round(float(n.get('Velocity')))))
            pad=note
            events += [(t,1,pad,color), (max(t+1,end),0,pad,0)]
    # At an exact retrigger boundary, releases happen before attacks.
    return sorted(events),ignored,disabled

def slice_scene(events,start,end):
    if end<=start: raise ValueError('Empty scene')
    state=[0]*128; initial=None; out=[]
    for tick,kind,pad,color in events:
        if tick>=end: break
        if tick>=start and initial is None: initial=state.copy()
        if tick>=start and state[pad]!=color:
            out.append([tick-start,pad,color])
        state[pad]=color
    if initial is None: initial=state.copy()
    return {'duration':end-start,'initial':initial,'events':out}

def varint(v):
    out=bytearray()
    while v>=128: out.append((v&127)|128); v>>=7
    out.append(v); return out

def encode(scene):
    b=bytearray(b'LPS2')+struct.pack('<II',scene['duration'],len(scene['events']))+bytes(scene['initial'])
    last=0
    for tick,pad,color in scene['events']:
        b+=varint(tick-last)+bytes([pad,color]); last=tick
    return bytes(b)

def decode(data):
    if data[:4]!=b'LPS2' or len(data)<140: raise ValueError('Bad LPS2 header')
    duration,count=struct.unpack_from('<II',data,4); initial=list(data[12:140]); p=140; tick=0; events=[]
    for _ in range(count):
        delta=shift=0
        while True:
            if p>=len(data) or shift>28: raise ValueError('Bad delta')
            b=data[p];p+=1;delta|=(b&127)<<shift;shift+=7
            if b<128: break
        tick+=delta
        if p+2>len(data): raise ValueError('Truncated event')
        pad,color=data[p:p+2];p+=2
        if pad>=128 or color>=128 or tick>=duration: raise ValueError('Bad event')
        events.append([tick,pad,color])
    if p!=len(data): raise ValueError('Trailing bytes')
    return {'duration':duration,'initial':initial,'events':events}

def import_set(source,out):
    out.mkdir(parents=True,exist_ok=True)
    root=ET.fromstring(gzip.decompress(source.read_bytes()))
    tracks=root.findall('.//Tracks/MidiTrack')
    found=[t for t in tracks if 'LPProMK3 MIDI' in val(t,'DeviceChain/MidiOutputRouting/Target') and t.findall('DeviceChain/MainSequencer/ClipSlotList/ClipSlot/ClipSlot/Value/MidiClip') and not t.findall('DeviceChain/DeviceChain/Devices/*')]
    if len(found)!=1: raise ValueError('Expected one dedicated external LPProMK3 MIDI clip track')
    track=found[0]
    if len(track.findall('DeviceChain/DeviceChain/Devices/*')): raise ValueError('Light track has MIDI effects; capture required')
    if val(track,'DeviceChain/MidiOutputRouting/LowerDisplayString')!='Ch. 1': raise ValueError('Expected static palette channel 1')
    scenes=[]; clips=[]; hashes=set()
    # Session slots only: avoid double-counting arrangement copies.
    for slot in track.findall('DeviceChain/MainSequencer/ClipSlotList/ClipSlot'):
        clip=slot.find('.//MidiClip')
        if clip is None: continue
        name=val(clip,'Name','Unnamed'); sid=int(slot.get('Id'))
        events,ignored,disabled=note_events(clip)
        start=round(float(val(clip,'Loop/LoopStart'))*PPQ)
        end=round(float(val(clip,'Loop/LoopEnd'))*PPQ)
        if end<=start: continue
        clips.append({'slot':sid,'name':name,'beats':(end-start)/PPQ,'notes':len(events)//2,'ignored_notes':ignored,'disabled_notes':disabled,'start_relative':float(val(clip,'Loop/StartRelative'))})
        windows=[(start,end,'full')]
        if end-start>16*PPQ:
            windows += [(s,min(s+16*PPQ,end),f'part {(s-start)//(16*PPQ)+1}') for s in range(start,end,16*PPQ)]
        for a,b,part in windows:
            scene=slice_scene(events,a,b);data=encode(scene)
            assert decode(data)==scene
            digest=hashlib.sha256(data).hexdigest()
            index=len(scenes); file=f'{index:03d}.lps';(out/file).write_bytes(data)
            scenes.append({'id':index,'name':name if part=='full' else f'{name} / {part}','source_slot':sid,'source_start_tick':a,'part':part,'duration':scene['duration'],'events':len(scene['events']),'bytes':len(data),'file':file,'sha256':digest,'duplicate':digest in hashes})
            hashes.add(digest)
    catalog={'format':'LPS2','ppq':PPQ,'source':str(source.resolve()),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'source_layout':'original Custom Mode MIDI note numbers retained; Programmer Mode mapping pending hardware check','mode':'static palette','clips':clips,'scenes':scenes,'hardware_verified':False}
    (out/'catalog.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2),encoding='utf8')
    return catalog

def write_bank(catalog,out,limit=48*1024):
    # Prefer short existing clips and four-bar extracts; retain full long shows on disk.
    options=[s for s in catalog['scenes'] if s['duration']<=16*PPQ and not s['duplicate']]
    options.sort(key=lambda s:(s['part']!='full',0 if s['part']=='full' else int(s['part'].split()[-1]),s['source_slot']))
    chosen=[];size=0; lines=['/* Generated from personal Ableton light clips. Do not redistribute source shows without permission. */']
    for s in options:
        if len(chosen)==64: break
        data=(out/s['file']).read_bytes()
        if size+len(data)>limit: continue
        i=len(chosen);chosen.append(s);size+=len(data)
        lines += [f'static const uint8_t lp_data_{i}[] = {{',','.join(str(b) for b in data),'};']
    lines+=['static const lp_scene_t lp_bank[] = {']
    for i,s in enumerate(chosen):
        name=s['name'].encode('ascii','replace').decode().replace('"',"'")
        lines.append('{lp_data_%d,sizeof(lp_data_%d),"%s"},'%(i,i,name))
    lines+=['};','#define LP_BANK_COUNT (sizeof(lp_bank)/sizeof(lp_bank[0]))']
    (out.parent/'firmware/src/launchparty_bank.h').write_text('\n'.join(lines),encoding='utf8')
    (out/'firmware-bank.json').write_text(json.dumps({'bytes':size,'scenes':chosen},ensure_ascii=False,indent=2),encoding='utf8')
    return len(chosen),size

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('source',type=Path);ap.add_argument('out',type=Path);args=ap.parse_args()
    c=import_set(args.source,args.out);n,b=write_bank(c,args.out)
    print(json.dumps({'clips':len(c['clips']),'scenes':len(c['scenes']),'bank_scenes':n,'bank_bytes':b,'ignored_notes':sum(x['ignored_notes'] for x in c['clips'])}))
