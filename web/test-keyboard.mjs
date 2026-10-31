import assert from 'node:assert/strict';import {KEYS,HeldClips,colorStep,typingTarget} from './keyboard.js';
assert.equal(KEYS.length,27);assert.equal(new Set(KEYS.map(k=>k[0])).size,27);
let changes=[],updates=0,h=new HeldClips((i,down)=>changes.push([i,down]),()=>updates++);
h.press('KeyQ',0);h.press('KeyQ',0);assert.deepEqual(changes,[[0,true]]);
h.press('KeyW',1);assert.equal(h.active,1);h.release('KeyW');assert.equal(h.active,0);
h.press('pointer1',0);h.release('KeyQ');assert.equal(h.active,0);assert.deepEqual(changes,[[0,true],[1,true],[1,false]]);h.release('pointer1');assert.equal(h.active,-1);assert.deepEqual(changes.at(-1),[0,false]);
h.press('KeyA',9);h.press('KeyS',10);h.clear();assert.equal(h.active,-1);assert.equal(h.sources.size,0);assert.deepEqual(changes.slice(-2),[[9,false],[10,false]]);
assert.equal(colorStep(0,-1),5);assert.equal(colorStep(5,1),0);assert.equal(colorStep(4,1),5);assert.equal(typingTarget(null),false);assert.equal(typingTarget({closest:()=>({})}),true);
console.log('PASS: 27 keys, repeat suppression, last-held priority/fallback, pointer+keyboard overlap, all-notes-off, six-color wrap, typing exclusion');
