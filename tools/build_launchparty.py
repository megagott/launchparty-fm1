#!/usr/bin/env python3
"""Build the public snapshot using retained generated assets. Never flash hardware."""
from pathlib import Path
import argparse,os,sys,shutil,hashlib,json
import build
ap=argparse.ArgumentParser();ap.add_argument('--toolchain',required=True);ap.add_argument('--sdk',required=True);args=ap.parse_args()
root=Path(__file__).resolve().parents[1];os.chdir(root)
os.environ['JIELI_TOOLCHAIN']=args.toolchain;os.environ['AC79_SDK']=args.sdk
required=['felucca_drumkits.h','felucca_font.h','felucca_icons.h','felucca_samples.h','felucca_tables.h','mycelium_logo.h','sloop_logo.h']
for name in required:
    if not (root/'build/gen'/name).is_file():raise SystemExit('Missing corresponding generated asset: '+name)
build.generate=lambda:None
sys.argv=['build.py'];result=build.main()
if result:raise SystemExit(result)
p=root/'build/launchparty-0.3.2-public.fwsc';shutil.copy2(root/'build/felucca.fwsc',p)
print(json.dumps({'file':p.name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size,'installed':False}))
