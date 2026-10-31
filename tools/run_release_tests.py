"""Run host-side release checks without connecting to hardware."""
from pathlib import Path
import subprocess,sys,os,json
root=Path(__file__).resolve().parents[1];os.chdir(root)
out=root/'build/host-tests';out.mkdir(parents=True,exist_ok=True)
names=['player_test','layout_test','fx_test','mix_test','party_test','overlay_color_test','solo_frame_test','store_test','touch_floor_test','boot_test','bank_test']
results=[]
for name in names:
    exe=out/name
    subprocess.run(['cc','-O2','-Ibuild/gen','-Ifirmware/src','tests/'+name+'.c','-o',str(exe)],check=True)
    result=subprocess.run([str(exe)],check=True,capture_output=True,text=True)
    if name=='bank_test':
        actual=out/'bank-actual.txt';actual.write_text(result.stdout)
        subprocess.run([sys.executable,'tests/check_bank.py',str(actual)],check=True)
    else: print(result.stdout.strip())
    results.append({'test':name,'result':'PASS'})
(out/'results.json').write_text(json.dumps(results,indent=2))
print('PASS:',len(results),'host checks; no hardware accessed')
