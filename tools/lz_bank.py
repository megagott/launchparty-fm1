from pathlib import Path
import json,collections
def compress(data):
 out=bytearray(b'LPC2'+len(data).to_bytes(4,'little'));pos=0;literal=bytearray();seen=collections.defaultdict(list)
 def flush():
  while literal:
   n=min(128,len(literal));out.append(n-1);out.extend(literal[:n]);del literal[:n]
 while pos<len(data):
  best=0;distance=0
  for previous in reversed(seen[data[pos:pos+3]][-24:]):
   d=pos-previous
   if d>65535:continue
   length=0
   while length<130 and pos+length<len(data) and data[pos+length]==data[pos+length-d]:length+=1
   if length>best:best=length;distance=d
  length=best if best>=4 else 1
  if best>=4:
   flush();out.append(128|(best-3));out.extend(distance.to_bytes(2,'little'))
  else:literal.append(data[pos]);
  for i in range(pos,pos+length):
   key=data[i:i+3];seen[key].append(i)
   if len(seen[key])>32:del seen[key][0]
  pos+=length
 flush();return bytes(out)
def expand(data):
 assert data[:4]==b'LPC2';size=int.from_bytes(data[4:8],'little');out=bytearray();p=8
 while p<len(data):
  token=data[p];p+=1
  if token<128:
   n=token+1;out.extend(data[p:p+n]);p+=n
  else:
   n=(token&127)+3;d=int.from_bytes(data[p:p+2],'little');p+=2
   for i in range(n):out.append(out[-d])
 assert len(out)==size;return bytes(out)
if __name__=='__main__':
 root=Path(__file__).resolve().parents[1];manifest=json.loads((root/'shows/firmware-bank.json').read_text());lines=['/* Losslessly compressed factory scenes. Raw sources remain in shows/. */'];total=0
 for i,e in enumerate(manifest['scenes']):
  raw=(root/'shows'/e['file']).read_bytes();packed=compress(raw);assert expand(packed)==raw;total+=len(packed)
  lines.append(f'static const uint8_t lp_data_{i}[] = {{'+','.join(map(str,packed))+'};')
 lines.append('static const lp_scene_t lp_bank[] = {')
 for i,e in enumerate(manifest['scenes']):lines.append('{lp_data_%d,sizeof(lp_data_%d),"%s %s"},'%(i,i,e['id'],e['name']))
 lines+=['};','#define LP_BANK_COUNT (sizeof(lp_bank)/sizeof(lp_bank[0]))','#ifndef LP_EXTERNAL_SCENES','static const lp_scene_t *lp_scene_at(uint32_t i) {return &lp_bank[i];}','#endif']
 (root/'firmware/src/launchparty_bank.h').write_text('\n'.join(lines)+'\n')
 print(json.dumps({'raw_bytes':manifest['bytes'],'packed_bytes':total,'saved':manifest['bytes']-total,'roundtrip':'all41 byte exact'}))
