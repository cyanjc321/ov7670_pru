
#ifndef _PRU_CAMERA_REG_
#define _PRU_CAMERA_REG_

#define VGA_WIDTH       640
#define VGA_HEIGHT      480

enum prucam_reg_op {
	reg_read = 0,
	reg_write,
	buf_set,
	buf_get
};

enum prucam_reg_addr {
	REG_CTRL = 0x0,
	REG_HEIGHT,
	REG_WIDTH,
	REG_BUFPT,
	TOTAL_REG_NO
};

struct prucam_query {
	uint32_t 	op;
	uint32_t 	addr;
	uint32_t 	data;
};

int prucam_reg_read(unsigned int addr, unsigned int *data);
int prucam_reg_write(unsigned int addr, unsigned int data);
int prucam_buf_queue(unsigned int pt);

#if defined (pru0) || defined (pru1)
volatile __far uint32_t CT_BUFQ[16] __attribute__((cregister("PRU_SHAREDMEM", near), peripheral));

#define ACQ_EN		0

//interrupts
#define MSG_INT		18	//map to host 0
#define ARM_INT		19	//map to host 2

#ifdef __cplusplus
extern "C" {
#endif
	extern void prucam_reg_get_value(uint32_t addr, uint32_t *data);
	extern void prucam_reg_set_value(uint32_t addr, uint32_t data);
#ifdef __cplusplus
}
#endif
#endif

#endif
