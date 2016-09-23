	.global prucam_reg_get_value
	.global prucam_reg_set_value

;ARG0	index	r14
;ARG1	value	r15


prucam_reg_get_value:
	add 	r0.b0, r14, 10			;compute shift so index 0 -> r0
    XIN 	0x0a, &r20.b0, 0x04		;
    SBBO    &r20, r15, 0, 4        	;

	jmp 	r3.w2					;return

prucam_reg_set_value:
	add 	r0.b0, r14, 15			;compute shift
	XOUT 	0x0a, &r15.b0, 0x04

	jmp 	r3.w2					;return

