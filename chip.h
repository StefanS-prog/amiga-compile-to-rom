extern volatile unsigned short  chip_reg[];
extern volatile unsigned char   ciaa_reg[];
extern volatile unsigned char   buffer[2][4 * 12672];
extern unsigned char            tilemap[2][396];
extern volatile unsigned char   wbuf;
extern unsigned short           lineoff[];
extern volatile unsigned char   *tile_p;
extern volatile unsigned int    *srcreg;
extern volatile unsigned int    *audsrc;
extern volatile unsigned int    *srcreg2;
extern volatile unsigned int    *srcreg3;
extern volatile unsigned int    *dstreg;
extern volatile unsigned short  dmaconr;
extern volatile short           aud_lock;
extern unsigned char            _binary_tileset_raw_start;
extern unsigned short           etime;
extern unsigned char            _binary_sound_raw_start;
extern unsigned char            _binary_sound_raw_end;

extern void                     update_objs(void);
extern int                      create_obj(unsigned char tileid, short x, short y, short w, short h, short vx, short vy);
extern void                     render_objs(void); 
extern void                     play_sound(void); 

typedef struct rect_s {
    short   llx, lly, urx, ury;
} rect_t;

typedef struct obj_s {
    rect_t          bbox;
    short           x, y;
    short           vx, vy;
    unsigned char   tileid;             // 255: free
    unsigned char   no_react_time;
    unsigned short  shape[8][16][2];
} obj_t;

// Define sections

#define CHIP    __attribute__ ((section (".chip_reg")))
#define CIAA    __attribute__ ((section (".ciaa_reg")))  
#define CIAB    __attribute__ ((section (".ciab_reg")))  

// Chip set registers

#define BPLCON0 (0x100/2)
#define BPLCON1 (0x102/2)
#define BPLCON2 (0x104/2)
#define BPL1DAT (0x110/2)
#define DMACON  (0x096/2)
#define COLOR0  (0x180/2)
#define INTENA  (0x09a/2)
#define INTREQ  (0x09c/2)
#define BPL1PTH (0x0e0/2)
#define BPL1PTL (0x0e2/2)
#define BPL2PTH (0x0e4/2)
#define BPL2PTL (0x0e6/2)
#define BPL3PTH (0x0e8/2)
#define BPL3PTL (0x0ea/2)
#define BPL4PTH (0x0ec/2)
#define BPL4PTL (0x0ee/2)
#define DIWSTRT (0x08e/2)
#define DIWSTOP (0x090/2)
#define DDFSTRT (0x092/2)
#define DDFSTOP (0x094/2)
#define BPL1MOD (0x108/2)
#define BPL2MOD (0x10A/2)
#define BLTCON0 (0x040/2)
#define BLTCON1 (0x042/2)
#define BLTAFWM (0x044/2)
#define BLTALWM (0x046/2)
#define BLTAPTH (0x050/2)
#define BLTAPTL (0x052/2)
#define BLTBPTH (0x04c/2)
#define BLTBPTL (0x04e/2)
#define BLTCPTH (0x048/2)
#define BLTCPTL (0x04a/2)
#define BLTDPTH (0x054/2)
#define BLTDPTL (0x056/2)
#define BLTSIZE (0x058/2)
#define BLTAMOD (0x064/2)
#define BLTBMOD (0x062/2)
#define BLTCMOD (0x060/2)
#define BLTDMOD (0x066/2)
#define DMACONR (0x002/2)
#define AUD0LCH (0x0a0/2)
#define AUD0LEN (0x0a4/2)
#define AUD0PER (0x0a6/2)
#define AUD0VOL (0x0a8/2)

#define I_PORTS (1<<3)
#define I_VERTB (1<<5)
#define I_AUD0  (1<<7)
#define I_INTEN (1<<14)
#define I_SET   (1<<15)

// CIA

#define PRA     0x0
#define DDRA    0x200
#define TALO    0x400
#define TAHI    0x500
#define ICR     0xd00
#define CRA     0xe00
