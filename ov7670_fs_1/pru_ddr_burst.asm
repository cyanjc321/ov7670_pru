	.global burst_sram_to_ddr

;ARG0	SRAM_ADDR	r14 --> r6
;ARG1	DDR_ADDR	r15	--> r7
;ARG2	SIZE		r16

;DATA		r14

;STACK_SIZE	16
;BURST_SIZE	64

;degug 0x99100000

burst_sram_to_ddr:
	sub r2, r2, 16					;allocate stack
	sbbo &r4, r2, 0, 16				;push r4, r5 into stack

	add r4, r15, r16				;compute end address of ram
	add r6, r14, 0					;load sram pointer
	add r7, r15, 0					;load ddr pointer

loop0:
	lbbo &r14, r6, 0, 64			;load data from sram
	sbbo &r14, r7, 0, 64			;store data to ddr
	add r6, r6, 64					;increment sram addr
	add r7, r7, 64					;increment ddr addr
	qblt loop0, r4, r7
	
	lbbo &r4, r2, 0, 16				;pop r4, r5 from stack
	add r2, r2, 16					;deallocate stack
	jmp r3.w2						;return
