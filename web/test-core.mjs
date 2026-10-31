import assert from 'node:assert/strict';import fs from 'node:fs';
import {target,decode,frame,bank,readBank,crc32,pack,unpack} from './core.js';
const factory=JSON.parse(fs.readFileSync(new URL('factory.json',import.meta.url)));
assert.equal(target(36),11);assert.equal(target(99),88);assert.equal(target(116),101);assert.equal(target(123),108);
assert.equal(new Set(Array.from({length:64},(_,i)=>target(i+36))).size,64);
for(let s of factory.scenes){let b=Uint8Array.from(Buffer.from(s.data,'base64')),d=decode(b);assert.equal(frame(d,0).length,128);assert.throws(()=>decode(b.slice(0,-1)));}
let slots=Array(41).fill(null);slots[7]=factory.scenes[0];assert.deepEqual(readBank(bank(slots)),slots);assert.equal(crc32(new TextEncoder().encode('123456789')),0xcbf43926);
for(let n=0;n<300;n++){let b=Uint8Array.from({length:n},(_,i)=>(i*71+n)%256);assert.deepEqual(unpack(pack(b)),b)}
let malformed=bank(slots);malformed[16+7*32]=0;malformed[16+7*32+1]=0;assert.throws(()=>readBank(malformed));
console.log('PASS: 41 clips, mapping, truncated files, bank roundtrip/bounds, CRC, all 7-bit packing lengths');
const {importMidi}=await import('./midi.js');
const midi=new Uint8Array([77,84,104,100,0,0,0,6,0,0,0,1,3,192,77,84,114,107,0,0,0,14,0,144,11,21,0x83,0x60,128,11,0,0x83,0x60,255,47,0]);
let imported=decode(importMidi(midi));assert.equal(frame(imported,0)[36],21);assert.equal(frame(imported,480)[36],0);assert.throws(()=>importMidi(midi.slice(0,-1)));assert.throws(()=>importMidi(new Uint8Array(2000001)));
console.log('PASS: Programmer MIDI note conversion, note-off timing, truncated/oversized MIDI rejection');
