
*-------------------------------------------------------*

	OPT		P=68040,L1,O+,W-

*-------------------------------------------------------*

*-------------------------------------------------------*
*	Initialisation functions			* 
*-------------------------------------------------------*

	xref		_screen_w,_screen_h,_odkud,_kam

*-------------------------------------------------------*
*	General functions				*
*-------------------------------------------------------*

	xdef		_rplanes

*-------------------------------------------------------*
	include		c2pmac.s
*-------------------------------------------------------*

push	macro
	move.\0		\1,-(sp)
	endm
	
pop	macro
	move.\0		(sp)+,\1
	endm

pushall	macro
	movem.l		d0-a6,-(sp)
	endm
	
popall	macro
	movem.l		(sp)+,d0-a6
	endm

*-------------------------------------------------------*
*	Initialise rendering display			*
*-------------------------------------------------------*
_rplanes:
*-------------------------------------------------------*
	pushall
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.local_regs		rs.l	15
*-------------------------------------------------------*
.local_rts		rs.l	1
*-------------------------------------------------------*
	move.l		_odkud,a0
	move.l		_kam,a1
*-------------------------------------------------------*
	movem.l		(a0)+,d1-d4
*-------------------------------------------------------*
	move.l		#$00FF00FF,d0	; 4
	splice.8	d1,d3,d0,d7	; 18
	splice.8	d2,d4,d0,d7	; 18
*-------------------------------------------------------*
	move.l		#$0F0F0F0F,d0	; 4
	splice.4	d1,d2,d0,d7	; 18
	splice.4	d3,d4,d0,d7	; 18
*-------------------------------------------------------*
	swap		d2		; 4(4:0)
	swap		d4		; 4(4:0)
	eor.w		d1,d2		; 2(2:0)
	eor.w		d3,d4		; 2(2:0)
	eor.w		d2,d1		; 2(2:0)
	eor.w		d4,d3		; 2(2:0)
	eor.w		d1,d2		; 2(2:0)
	eor.w		d3,d4		; 2(2:0)
	swap		d2		; 4(4:0)
	swap		d4		; 4(4:0)
*-------------------------------------------------------*
	move.l		#$33333333,d0	; 4
	splice.2	d1,d2,d0,d7	; 18
	splice.2	d3,d4,d0,d7	; 18
*-------------------------------------------------------*
	move.l		#$55555555,d0	; 4
	splice.1	d1,d3,d0,d7	; 18
	splice.1	d2,d4,d0,d7	; 18
*-------------------------------------------------------*
*	32-bit destination				*
*-------------------------------------------------------*
	swap		d4		; 4(4:0)
	eor.w		d2,d4		; 2(2:0)
	eor.w		d4,d2		; 2(2:0)
	eor.w		d2,d4		; 2(2:0)
	swap		d2		; 4(4:0)
	swap		d3		; 4(4:0)
	eor.w		d1,d3		; 2(2:0)
	eor.w		d3,d1		; 2(2:0)
	eor.w		d1,d3		; 2(2:0)
	swap		d1		; 4(4:0)
*-------------------------------------------------------*
	move.l		d4,a2
	move.l		d3,a3
	move.l		d2,a4
	move.l		d1,a5
*-------------------------------------------------------*
	move.w		_screen_h,d6
	subq.w		#1,d6
*-------------------------------------------------------*
.ylp:	move.w		_screen_w,d5
	lsr.w		#4,d5
	subq.w		#1,d5
*-------------------------------------------------------*
.xlp:	movem.l		(a0)+,d1-d4
*-------------------------------------------------------*
	move.l		#$00FF00FF,d0	; 4
	splice.8	d1,d3,d0,d7	; 18
	splice.8	d2,d4,d0,d7	; 18
*-------------------------------------------------------*
	move.l		a2,(a1)+
*-------------------------------------------------------*
	move.l		#$0F0F0F0F,d0	; 4
	splice.4	d1,d2,d0,d7	; 18
	splice.4	d3,d4,d0,d7	; 18
*-------------------------------------------------------*
	move.l		a3,(a1)+
*-------------------------------------------------------*
	swap		d2		; 4(4:0)
	swap		d4		; 4(4:0)
	eor.w		d1,d2		; 2(2:0)
	eor.w		d3,d4		; 2(2:0)
	eor.w		d2,d1		; 2(2:0)
	eor.w		d4,d3		; 2(2:0)
	eor.w		d1,d2		; 2(2:0)
	eor.w		d3,d4		; 2(2:0)
	swap		d2		; 4(4:0)
	swap		d4		; 4(4:0)
*-------------------------------------------------------*
	move.l		#$33333333,d0	; 4
	splice.2	d1,d2,d0,d7	; 18
	splice.2	d3,d4,d0,d7	; 18
*-------------------------------------------------------*
	move.l		a4,(a1)+
*-------------------------------------------------------*
	move.l		#$55555555,d0	; 4
	splice.1	d1,d3,d0,d7	; 18
	splice.1	d2,d4,d0,d7	; 18
*-------------------------------------------------------*
	move.l		a5,(a1)+
*-------------------------------------------------------*
*	32-bit destination				*
*-------------------------------------------------------*
	swap		d4		; 4(4:0)
	eor.w		d2,d4		; 2(2:0)
	eor.w		d4,d2		; 2(2:0)
	eor.w		d2,d4		; 2(2:0)
	swap		d2		; 4(4:0)
	swap		d3		; 4(4:0)
	eor.w		d1,d3		; 2(2:0)
	eor.w		d3,d1		; 2(2:0)
	eor.w		d1,d3		; 2(2:0)
	swap		d1		; 4(4:0)
*-------------------------------------------------------*
	move.l		d4,a2
	move.l		d3,a3
	move.l		d2,a4
	move.l		d1,a5
*-------------------------------------------------------*
	dbra		d5,.xlp
	tst.w		d6
	beq.s		.none
	dbra		d6,.ylp
*-------------------------------------------------------*
.none:	move.l		a2,(a1)+
	move.l		a3,(a1)+
	move.l		a4,(a1)+
	move.l		a5,(a1)+
*-------------------------------------------------------*
	popall
*-------------------------------------------------------*
	rts

*-------------------------------------------------------*
			text
*-------------------------------------------------------*