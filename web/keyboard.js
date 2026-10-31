// Physical QWERTZ positions: three rows of nine overlay clips.
export const KEYS=[['KeyQ','Q'],['KeyW','W'],['KeyE','E'],['KeyR','R'],['KeyT','T'],['KeyY','Z'],['KeyU','U'],['KeyI','I'],['KeyO','O'],['KeyA','A'],['KeyS','S'],['KeyD','D'],['KeyF','F'],['KeyG','G'],['KeyH','H'],['KeyJ','J'],['KeyK','K'],['KeyL','L'],['KeyZ','Y'],['KeyX','X'],['KeyC','C'],['KeyV','V'],['KeyB','B'],['KeyN','N'],['KeyM','M'],['Comma',','],['Period','.']];
export const colorStep=(value,delta)=>(value+delta%6+6)%6;
export function typingTarget(target){return !!target?.closest?.('input,textarea,select,[contenteditable]:not([contenteditable="false"]),[role="textbox"]')}
export class HeldClips{
 constructor(change,update){this.sources=new Map();this.order=[];this.change=change;this.update=update}
 get active(){return this.order.at(-1)??-1}
 has(index){return this.order.includes(index)}
 press(source,index){if(this.sources.has(source))return;let already=this.has(index);this.sources.set(source,index);this.order=this.order.filter(n=>n!==index);this.order.push(index);if(!already)this.change(index,true);this.update()}
 release(source){if(!this.sources.has(source))return;let index=this.sources.get(source);this.sources.delete(source);if(![...this.sources.values()].includes(index)){this.order=this.order.filter(n=>n!==index);this.change(index,false)}this.update()}
 clear(){let previous=[...this.order];this.sources.clear();this.order=[];for(let index of previous)this.change(index,false);this.update()}
}
