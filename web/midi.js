// SPDX-License-Identifier: GPL-3.0-only
import {target,decode} from './core.js';
// Standard MIDI files 0/1. Times remain musical beats; tempo is controlled live.
export function importMidi(bytes,mapping='programmer'){
 let b=new Uint8Array(bytes),d=new DataView(b.buffer,b.byteOffset,b.byteLength),p=0;
 const need=n=>{if(p+n>b.length)throw Error('Unvollständige MIDI-Datei.');};
 const u32=()=>{need(4);let v=d.getUint32(p);p+=4;return v},u16=()=>{need(2);let v=d.getUint16(p);p+=2;return v};
 const text=n=>{need(n);let s=String.fromCharCode(...b.slice(p,p+n));p+=n;return s};
 const variable=end=>{let n=0,v;for(let i=0;i<4;i++){if(p>=end)throw Error('Ungültige MIDI-Zeit.');v=b[p++];n=n*128+(v&127);if(!(v&128))return n}throw Error('MIDI-Zeit zu lang.')};
 if(b.length>2000000||text(4)!=='MThd')throw Error('Keine Standard-MIDI-Datei.');let header=u32();if(header<6||header>1000)throw Error('Ungültiger MIDI-Kopf.');let format=u16(),tracks=u16(),division=u16();if(format>1||!tracks||tracks>128||!division||(division&32768))throw Error('Nur MIDI Format 0/1 mit musikalischem Zeitraster wird unterstützt.');p=8+header;let events=[],endTick=0,order=0;
 const inverse=new Map();for(let i=0;i<128;i++){let t=target(i);if(t>=0)inverse.set(t,i)}
 for(let tr=0;tr<tracks;tr++){
  if(text(4)!=='MTrk')throw Error('MIDI-Spur fehlt.');let end=p+4+u32();if(end>b.length)throw Error('MIDI-Spur abgeschnitten.');let tick=0,running=0;
  while(p<end){tick+=variable(end);if(tick>division*4096)throw Error('Clip zu lang. Bitte vorher kürzen.');let status=b[p];if(status&128){p++;if(status<240)running=status;else if(status!==255)running=0}else{status=running;if(!status)throw Error('Ungültiger MIDI-Status.')}
   if(status===255){if(p>=end)throw Error('Meta-Daten fehlen.');p++;let n=variable(end);if(p+n>end)throw Error('Meta-Daten abgeschnitten.');p+=n;continue}
   if(status===240||status===247){let n=variable(end);if(p+n>end)throw Error('SysEx abgeschnitten.');p+=n;continue}
   if(status>=240)throw Error('Nicht unterstützter MIDI-Status.');let kind=status&240,n=kind===192||kind===208?1:2;if(p+n>end||b[p]>127||(n===2&&b[p+1]>127))throw Error('Ungültige MIDI-Nachricht.');let note=b[p++],v=n===2?b[p++]:0;
   if(kind===144||kind===128){let src=mapping==='programmer'?inverse.get(note):target(note)>=0?note:undefined;if(src!==undefined)events.push([Math.round(tick*960/division),src,kind===128?0:v,order++])}
  }endTick=Math.max(endTick,tick);
 }
 if(!events.length)throw Error('Keine passenden Launchpad-LED-Noten gefunden.');events.sort((a,b)=>a[0]-b[0]||a[3]-b[3]);let duration=Math.max(Math.ceil(endTick*960/division/960)*960,960),body=[],last=0,state=new Uint8Array(128);
 for(let[t,n,v]of events){if(t>=duration)continue;if(state[n]===v)continue;state[n]=v;let delta=t-last;do{let c=delta&127;delta=Math.floor(delta/128);body.push(c|(delta?128:0))}while(delta);body.push(n,v);last=t}
 let count=0;for(let i=0;i<body.length;){while(body[i++]&128){}i+=2;count++}let out=new Uint8Array(140+body.length),view=new DataView(out.buffer);out.set([76,80,83,50]);view.setUint32(4,duration,true);view.setUint32(8,count,true);out.set(body,140);decode(out);return out;
}
