/* Physical-position translation, Novation Pro MK3 official Legacy diagram
 * and Programmer Reference p19. Shows retain their original Legacy addresses.
 * 255 denotes an address absent from the Legacy surface. */
static uint8_t lp_source_note(uint32_t x,uint32_t y) {
    return (uint8_t)(36+4*y+x%4+(x>=4?32:0));
}
static uint8_t lp_programmer_note(uint32_t note) {
    if(note>=36 && note<=99) {
        uint32_t n=note-36;
        return (uint8_t)(11+n%4+(n>=32?4:0)+10*((n%32)/4));
    }
    if(note>=28 && note<=35)return (uint8_t)(91+note-28);
    if(note==26)return 90; /* Shift */
    if(note==27)return 99; /* logo */
    if(note>=100 && note<=107)return (uint8_t)(89-10*(note-100)); /* right */
    if(note>=108 && note<=115)return (uint8_t)(80-10*(note-108)); /* left */
    if(note>=116 && note<=123)return (uint8_t)(101+note-116); /* Track Select */
    if(note>=12 && note<=19)return (uint8_t)(1+note-12); /* Track Control */
    return 255;
}
