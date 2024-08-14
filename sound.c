#include "chip.h"
#include <stdint.h>

volatile short  aud_lock;

void req_delayed_aud_dma_en(void)
{
    volatile unsigned char  v;

//  chip_reg[COLOR0] = 0x0fff;

    ciaa_reg[CRA] = 0x08;       // one-shot mode
    v = ciaa_reg[ICR];          // reply to all interrupt requests
    aud_lock = 1;

    chip_reg[INTENA] = I_SET | I_PORTS;
    ciaa_reg[ICR] = 0x81;       // forward TA underflow to the chipset
    ciaa_reg[TALO] = 178;
    ciaa_reg[TAHI] = 0;         // starts timer
}

void play_sound(void)
{
    int len;

    if (aud_lock)
       return;

    chip_reg[DMACON] = 1;
    *audsrc = (uintptr_t)&_binary_sound_raw_start;
    len = &_binary_sound_raw_end - &_binary_sound_raw_start;
    chip_reg[AUD0LEN] = ((len + 1) & -2) / 2;
    req_delayed_aud_dma_en();
}
