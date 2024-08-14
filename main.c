#include "chip.h"
#include <stdint.h>

CHIP volatile unsigned short        chip_reg[0x1000 / 2];
CIAA volatile unsigned char         ciaa_reg[0xf01];
_Alignas(2) volatile unsigned char  buffer[2][4 * 12672]; // 352 x 288
unsigned short                      lineoff[256 + 16];
unsigned int                        silence;

volatile unsigned char  wbuf;
volatile unsigned char  missed;
unsigned char           tilemap[2][396];    // 22 x 18 = 396 tiles
unsigned short          etime;              // elapsed time
extern unsigned short   _binary_colors_pal_start;
extern unsigned char    _binary_tilemap_raw_start;

volatile unsigned char  *tile_p;
volatile unsigned int   *srcreg = (volatile unsigned int *)&chip_reg[BLTAPTH];
volatile unsigned int   *srcreg2 = (volatile unsigned int *)&chip_reg[BLTBPTH];
volatile unsigned int   *srcreg3 = (volatile unsigned int *)&chip_reg[BLTCPTH];
volatile unsigned int   *dstreg = (volatile unsigned int *)&chip_reg[BLTDPTH];
volatile unsigned int   *audsrc = (volatile unsigned int *)&chip_reg[AUD0LCH];
volatile unsigned short dmaconr;

// Register initializations
void hardwareInit(void) {
    ciaa_reg[ICR] = 0x7f;       // disable interrupt forwarding to chipset
    chip_reg[INTENA] = 0x7fff;  // disable interrupt forwarding to m68k 
    chip_reg[INTREQ] = 0x7fff;  // reply to all interrupt requests

    chip_reg[DMACON] = 0x7fff;  // turn off chipset DMA

    chip_reg[BPLCON0] = 0x0200; // show a gray background
    chip_reg[BPL1DAT] = 0x0;
    chip_reg[COLOR0] = 0x888;
}

__attribute__ ((interrupt)) void Timer(void)
{ 
    volatile unsigned char  v;

    ciaa_reg[ICR] = 0x1;
    chip_reg[INTENA] = I_PORTS;
    chip_reg[INTREQ] = I_AUD0;
    chip_reg[INTENA] = I_SET | I_AUD0;
    chip_reg[DMACON] = 1<<15 | 1;
//  chip_reg[COLOR0] = 0x0000;
}
__attribute__ ((interrupt)) void Audio(void)
{ 
    chip_reg[INTENA] = I_AUD0;   
    chip_reg[AUD0LEN] = 2;
    *audsrc = (uintptr_t)&silence;
    aud_lock = 0;
} 

volatile unsigned short vsyncCounter;

__attribute__ ((interrupt)) void VSync(void)
{
    static unsigned char    expected_cbuf = 0;
    unsigned char           cbuf = wbuf^(unsigned char)1;
    uintptr_t               pic = (uintptr_t)&buffer[cbuf][2818];

    if (cbuf != expected_cbuf)
        missed = 1;
    else
        expected_cbuf = wbuf;

    vsyncCounter++;
    chip_reg[BPL1PTH] = pic >> 16;
    chip_reg[BPL1PTL] = pic;
    chip_reg[BPL2PTH] = pic+44 >> 16;
    chip_reg[BPL2PTL] = pic+44;
    chip_reg[BPL3PTH] = pic+88 >> 16;
    chip_reg[BPL3PTL] = pic+88;
    chip_reg[BPL4PTH] = pic+132 >> 16;
    chip_reg[BPL4PTL] = pic+132;

    chip_reg[INTREQ] = I_VERTB;
}

void init(void)
{
    int i;

    for (i = 0; i < 16; i++)
        chip_reg[COLOR0+i] = *(&_binary_colors_pal_start + i);

    i = 0;
    for (int m = 1; m < 17; m++)
        for (int n = 1; n < 21; n++) {
            tilemap[0][22 * m + n]
            = tilemap[1][22 * m + n] = *(&_binary_tilemap_raw_start + i) - 1 | 1<<7;
            i++;
        }

    lineoff[0] = 0;
    for (i = 1; i < 256 + 16; i++)
        lineoff[i] = lineoff[i - 1] + 44 * 4;

    chip_reg[DIWSTRT] = 0x2c81;
    chip_reg[DIWSTOP] = 0x2cc1;
    chip_reg[DDFSTRT] = 0x38;
    chip_reg[DDFSTOP] = 0xd0;

    chip_reg[BPL1MOD] = 136;
    chip_reg[BPL2MOD] = 136;

    chip_reg[BPLCON0] = 0x4200;
    chip_reg[BPLCON1] = 0;
    chip_reg[BPLCON2] = 0;

    chip_reg[BLTAFWM] = 0xffff;
    chip_reg[BLTALWM] = 0xffff;

    chip_reg[AUD0VOL] = 64;
    chip_reg[AUD0PER] = 443;

    chip_reg[DMACON] = 1<<15 | 1<<9 | 1<<8 | 1<<6;
    chip_reg[INTENA] = I_SET | I_INTEN | I_VERTB;
    
    ciaa_reg[PRA] = 0x0;
}

void restore_bg(void)
{
    int             i, j, o;
    unsigned char   code;

    chip_reg[BLTCON0] = 0x09f0;
    chip_reg[BLTCON1] = 0;
    chip_reg[BLTAMOD] = 82;
    chip_reg[BLTDMOD] = 42;
    tile_p = buffer[wbuf] + 2818;

    o = 22;
    for (i = 1; i < 17; i++, o += 22) {
        for (j = 1; j < 21; j++) {
            code = tilemap[wbuf][o + j];
            if(code & 1<<7) {    // tile possibly dirty
                code &= 0x7f;
                *dstreg = (uintptr_t)tile_p; 
                *srcreg = (uintptr_t)(code*2 + &_binary_tileset_raw_start);

                dmaconr = chip_reg[DMACONR];
                while (chip_reg[DMACONR] & 1<<14);
                chip_reg[BLTSIZE] = (64 << 6) + 1;  // 1 x 16 words
                tilemap[wbuf][o + j] = code;
            }
            tile_p += 2;
        }
        tile_p += 2776;             // -40 + 16*4*44
    }
}
 
void main(void)
{
    unsigned short current_cnt;

    init();
    create_obj(13, 144, 184, 16, 16, 8, -8); 
    create_obj(13, 160, 200, 16, 16, -8, -8); 
    create_obj(13, 180, 200, 16, 16, 4, -16); 

    current_cnt = vsyncCounter;
    while (vsyncCounter == current_cnt);

    current_cnt = vsyncCounter;

    for (;;) {
        restore_bg();
        update_objs();
        render_objs();

        dmaconr = chip_reg[DMACONR];
        while (chip_reg[DMACONR] & 1<<14);

        etime = current_cnt;
        wbuf ^= (unsigned char)1; 

        current_cnt = vsyncCounter;
        while (vsyncCounter == current_cnt);

        current_cnt = vsyncCounter;
        etime = current_cnt - etime;
    }
}
