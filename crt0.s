	.globl	_start
	.globl	atexit

	.extern hardwareInit
	.extern VSync
	.extern	main

vectors:
	dc.l	0x80000,  _start, Default, Default, Default, Default, Default, Default
	dc.l	Default, Default, Default, Default, Default, Default, Default, Default
	dc.l	Default, Default, Default, Default, Default, Default, Default, Default
	dc.l	Default, Default,   Timer,   VSync,   Audio, Default, Default, Default
	dc.l	Default, Default, Default, Default, Default, Default, Default, Default
	dc.l	Default, Default, Default, Default, Default, Default, Default, Default
	dc.l	Default, Default, Default, Default, Default, Default, Default, Default
	dc.l	Default, Default, Default, Default, Default, Default, Default, Default

.align 2	

Default:
    RTE

_start:
* Delay loop
    MOVE.L  #0x20000, D0
loop:
    SUBQ.L  #1, D0
    BGT.S   loop

* Set up port A of first CIA (set outputs, turn off mirror and LED)
    MOVE.B  #3, 0xbfe201 
    MOVE.B  #2, 0xbfe001

* Set RAM to zero (clears also BSS)
    LEA     64*4, A0
    LEA     0x80000, A1
loopram:
    MOVE.W  #0, (A0)+
    CMPA.L  A1, A0
    BNE     loopram

* Copy vector table
    LEA     0, A0
    LEA     64*4, A1
    LEA     vectors, A2
loopvec:
    MOVE.W  (A2)+, (A0)+
    CMPA.L  A1, A0
    BNE     loopvec
   
* Copy initialized data from ROM to RAM
    LEA     _data, A0
    LEA     _edata, A1
    LEA     _etext, A2
loopdat:
    CMPA.L  A1, A0
    BEQ     cpydone
    MOVE.W  (A2)+, (A0)+
    BRA     loopdat
cpydone:
    BSR     hardwareInit

* Enable interrupts, keep supervisor mode enabled
    MOVE.W  #0x2000, SR

* Jump to mainloop
    BSR     main

endloop:
    BRA     endloop
