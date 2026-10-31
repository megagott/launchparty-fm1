// Explicit actions for protocol 1 devices, whose PLAY command is a toggle.
// Reset first so an already-running FM1 also ends in the requested playing state.
export async function startShow(control,scene){
 if(!Number.isInteger(scene)||scene<0||scene>=41)throw Error('Ungültige Show.');
 await control([2]);
 await control([3,scene]);
 await control([1]);
}
