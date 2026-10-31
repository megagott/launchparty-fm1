"""Compare every millisecond of compiled C playback to decoded source data."""
from pathlib import Path
import json,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from import_shows import decode
root=Path(__file__).resolve().parents[1]
bank=json.loads((root/'shows/firmware-bank.json').read_text(encoding='utf8'))['scenes']
actual={int(i):(int(m),int(h)) for i,m,h in (line.split() for line in Path(sys.argv[1]).read_text().splitlines())}
assert len(actual)==len(bank)
for i,entry in enumerate(bank):
    scene=decode((root/'shows'/entry['file']).read_bytes());state=scene['initial'].copy();e=0;h=2166136261;m=0
    while True:
        tick=m*120*960//60000
        if tick>=scene['duration']:state=[0]*128
        else:
            while e<len(scene['events']) and scene['events'][e][0]<=tick:
                _,pad,c=scene['events'][e];state[pad]=c;e+=1
        for c in state:h=((h^c)*16777619)&0xffffffff
        m+=1
        if tick>=scene['duration']:break
    assert actual[i]==(m,h),(entry['name'],actual[i],(m,h))
print(f'PASS: all {len(bank)} compiled firmware scenes match imported state at every millisecond, including end blackout')
