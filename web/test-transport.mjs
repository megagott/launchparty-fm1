import assert from 'node:assert/strict';
import {startShow} from './transport.js';
for(const initial of [false,true]){
 let playing=initial,scene=17,calls=[];
 const control=async a=>{calls.push(a);if(a[0]===2)playing=false;if(a[0]===3)scene=a[1];if(a[0]===1)playing=!playing};
 await startShow(control,4);assert.equal(playing,true);assert.equal(scene,4);
 await startShow(control,4);assert.equal(playing,true);
 assert.deepEqual(calls.slice(0,3),[[2],[3,4],[1]]);
}
let calls=0;await assert.rejects(startShow(async()=>{calls++;throw Error('offline')},2));assert.equal(calls,1);
await assert.rejects(startShow(async()=>{},41));
console.log('PASS: start from stopped/running; repeated start stays running; errors stop sequence; scene bounds');
