#include <stdint.h>
#include <limits.h>
#include "chip.h"

#define NUM_OBJS    10

static int      avail = -1;
obj_t           objs[NUM_OBJS];
const rect_t    inf_rect = {SHRT_MAX, SHRT_MAX, SHRT_MAX, SHRT_MAX};
const rect_t    scr_rect = {0 + 16, 256 + 16, 320 + 16, 0 + 16};

short min(short x, short y)
{
    if (x < y)
        return x;
    else
        return y;
}

short max(short x, short y)
{
    if (x < y)
        return y;
    else
        return x;
}

int no_area(rect_t r)
{
    if (r.llx == r.urx || r.lly == r.ury)
        return 1;
    else
        return 0;
}

rect_t transl_rect(rect_t r, short dx, short dy)
{
    rect_t  tr;

    tr.llx = r.llx + dx;
    tr.urx = r.urx + dx;
    tr.lly = r.lly + dy;
    tr.ury = r.ury + dy;

    return tr;
}

rect_t intersect_rects(rect_t r1, rect_t r2)
{
    rect_t  r;

    if (r1.urx<r2.llx || r1.llx>r2.urx || r1.lly<r2.ury || r1.ury>r2.lly)
        return inf_rect;

    r.urx = min(r1.urx, r2.urx);
    r.llx = max(r1.llx, r2.llx);
    r.lly = min(r1.lly, r2.lly);
    r.ury = max(r1.ury, r2.ury);

    return r;
}

void update_objs(void)
{
    int     i, j, col;
    rect_t  r, rt;
    short   x, y;

    if (etime > 3)
        etime = 3;        

    for (;etime; etime--)
        for (i = 0; i <= avail; i++) {
            if (objs[i].tileid == 255)
                continue;

            objs[i].x += objs[i].vx;
            objs[i].y += objs[i].vy;
            x = objs[i].x / 8;
            y = objs[i].y / 8;
            r = transl_rect(objs[i].bbox, x, y);
            rt = intersect_rects(r, scr_rect);

            if (no_area(rt)) {
                objs[i].vx *= -1;
                objs[i].vy *= -1;
                play_sound();
            } 

            if (objs[i].no_react_time) {
                objs[i].no_react_time--;
                continue;
            }

            for (col = 0, j = i + 1; j <= avail; j++) {
                if (objs[j].tileid == 255)
                    continue;

                x = objs[j].x / 8;
                y = objs[j].y / 8;                
                rt = transl_rect(objs[j].bbox, x, y);
                rt = intersect_rects(r, rt);

                if (!no_area(rt)) {
                    col = 1;
                    play_sound();
                    objs[j].vx *= -1;
                    objs[j].vy *= -1;
                }
            }
            if (col) {
                objs[i].x -= objs[i].vx;
                objs[i].y -= objs[i].vy;
                objs[i].vx *= -1;
                objs[i].vy *= -1;
                objs[i].no_react_time = 4;
            }
        }
}

int create_obj(unsigned char tileid, short x, short y, short w, short h, short vx, short vy)
{
    if (avail == NUM_OBJS-1)
        return -1;

    avail++;

    objs[avail].tileid = tileid;
    objs[avail].x = x * 8;
    objs[avail].y = y * 8;
    objs[avail].vx = vx;
    objs[avail].vy = vy;
    objs[avail].bbox.llx = -w / 2;
    objs[avail].bbox.lly = h / 2;
    objs[avail].bbox.urx = w / 2;
    objs[avail].bbox.ury = -h / 2;
    objs[avail].no_react_time = 0;

    chip_reg[BLTCON0] = 0x09f0;
    chip_reg[BLTCON1] = 0;
    chip_reg[BLTAMOD] = 82;
    chip_reg[BLTDMOD] = 2;

    *dstreg = (uintptr_t)objs[avail].shape;
    *srcreg = (uintptr_t)(tileid*2 + &_binary_tileset_raw_start);

    dmaconr = chip_reg[DMACONR];
    while (chip_reg[DMACONR] & 1<<14);
    chip_reg[BLTSIZE] = (128 << 6) + 1;      // 64 lines + 64 mask lines

    return avail;
}

void freeze_obj_out(int objectid)
{
    objs[objectid].tileid = 255;
}

void render_obj(int oid)
{
    rect_t          r, rt;
    unsigned char   shift_x, shift_y;
    uintptr_t       addr;
    short           x, y, tile_x, tile_y;

    x = objs[oid].x / 8;
    y = objs[oid].y / 8;
    r = transl_rect(objs[oid].bbox, x, y);    
    rt = intersect_rects(r, scr_rect);
    if (no_area(rt))
        return;

    shift_x = r.llx & 0xf;
    shift_y = r.ury & 0xf;
    tile_x = r.llx >> 4;
    tile_y = r.ury >> 4;

    tilemap[wbuf][tile_y*22 + tile_x] |= 0x80;
    if (shift_x && shift_y) {
        tilemap[wbuf][tile_y*22 + tile_x + 1] |= 0x80;
        tilemap[wbuf][tile_y*22 + tile_x + 22] |= 0x80;
        tilemap[wbuf][tile_y*22 + tile_x + 23] |= 0x80;
    }
    else if (shift_x && !shift_y)
        tilemap[wbuf][tile_y*22 + tile_x + 1] |= 0x80;
    else if (!shift_x && shift_y)
        tilemap[wbuf][tile_y*22 + tile_x + 22] |= 0x80;

    chip_reg[BLTCON0] = 0x0fca | shift_x<<12;
    chip_reg[BLTCON1] = shift_x << 12;
    chip_reg[BLTAMOD] = 0;
    chip_reg[BLTBMOD] = 0;
    chip_reg[BLTCMOD] = 40;
    chip_reg[BLTDMOD] = 40;

    addr = (uintptr_t)(buffer[wbuf] + lineoff[r.ury] + tile_x*2);
    *dstreg = addr;
    *srcreg3 = addr;
    *srcreg = (uintptr_t)(objs[oid].shape[4]);
    *srcreg2 = (uintptr_t)(objs[oid].shape[0]);

    dmaconr = chip_reg[DMACONR];
    while (chip_reg[DMACONR] & 1<<14);
    chip_reg[BLTSIZE] = (64 << 6) + 2;      // 64 lines
}

void render_objs(void)
{
    int i;
    
    for (i = 0; i <= avail; i++) {
        if (objs[i].tileid == 255)
            continue;

        render_obj(i);
    }
}
