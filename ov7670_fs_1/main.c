/***************************************************************************************
 * MAIN.C
 *
 * Description: main source file for PRU development
 *
 * Rafael de Souza
 * (C) 2015 Texas Instruments, Inc
 *
 * Built with Code Composer Studio v6
 **************************************************************************************/

#include <stdint.h>
#include <am335x/pru_cfg.h>
#include <am335x/pru_intc.h>
#include <am335x/pru_ctrl.h>

#include "resource_table_empty.h"

#include "pru_ddr_burst.h"
#include "pru_camera_reg.h"

volatile register uint32_t __R30;
volatile register uint32_t __R31;

/* input bit & mask */
#define BIT(x)		(1u << x)

#define PCLK_MASK	BIT(16)
#define VSYNC_MASK	BIT(11)
#define HREF_MASK	BIT(10)
#define DATA_MASK	(0xff << 2)

#define PCLK		(__R31 & PCLK_MASK)
#define VSYNC		(__R31 & VSYNC_MASK)
#define HREF		(__R31 & HREF_MASK)
#define DATA		(__R31 & DATA_MASK)

/* helper macros */
#define WAIT_PCLK_LOW	while (PCLK);
#define WAIT_PCLK_HIGH	while (!PCLK);

#define WAIT_NEXT_POSEDGE_PCLK	{WAIT_PCLK_LOW; WAIT_PCLK_HIGH;}

#define DEBUG_TO_MEM(m) {*debug_pt = (uint32_t)m; debug_pt++;}
#define DEBUG_SESSTION	0

uint8_t	linedata[640 * 2];	//declare globally to use data ram

void image_acquisition(void *ram_pt) {
	int x = 0, y = 0, pxl_cnt = 0;

	while (!VSYNC);
	while (VSYNC);	//ensure vsync falling edge
	while (!VSYNC) {
		if (HREF) {
			do {
				WAIT_NEXT_POSEDGE_PCLK
				linedata[x] = DATA >> 2;
				x ++;
			} while (HREF);
			burst_sram_to_ddr(linedata, ram_pt, x);
			ram_pt = (void*)((uint32_t)ram_pt + x);
			pxl_cnt += x;
			y ++;
			x = 0;
		}
	}
}

void main(void) {
	uint32_t height, width, buffer_addr, next_buffer, ctrl;

	uint32_t *debug_pt;
	debug_pt = (uint32_t *)0x99000000;		//debug output region 0x99000000 - 0x990000ff

	uint32_t frame_no = 0;

	/* pru configuration */
	//setup ocp
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

	//setup r30[1] to output clk (200 / 8 / 1 = 25 MHz)
	CT_CFG.GPCFG1_bit.PRU1_GPO_MODE = 1;
	CT_CFG.GPCFG1_bit.PRU1_GPO_DIV0 = 14;
	CT_CFG.GPCFG1_bit.PRU1_GPO_DIV1 = 0;

	//setup r31 for parallel input
	CT_CFG.GPCFG1_bit.PRU1_GPI_MODE = 1;
	CT_CFG.GPCFG1_bit.PRU1_GPI_CLK_MODE = 0;

	/* reg initialisation */
	prucam_reg_get_value(REG_HEIGHT, &height);
	prucam_reg_get_value(REG_WIDTH, &width);
	prucam_reg_get_value(REG_BUFPT, &next_buffer);

	*debug_pt = 0x11;
	__delay_cycles(200000000);

	while (1) {
		if (__R31 & BIT(31)) {
			prucam_reg_get_value(REG_HEIGHT, &height);
			prucam_reg_get_value(REG_WIDTH, &width);
			prucam_reg_get_value(REG_BUFPT, &next_buffer);
			prucam_reg_get_value(REG_CTRL, &ctrl);
			CT_INTC.SICR = MSG_INT;	//clear interrupt here
		}

		if (ctrl)
			while (!(__R31 & BIT(30))) {
				buffer_addr = next_buffer;
				DEBUG_TO_MEM(buffer_addr);
				next_buffer = 0;
				if (!buffer_addr) break;
				image_acquisition((void *)buffer_addr);
				__R31 = ARM_INT + 16; //interrupt arm
				frame_no ++;
				DEBUG_TO_MEM(frame_no);
			}
	}

quit:
	DEBUG_TO_MEM(0xff);
	DEBUG_TO_MEM(frame_no);
	DEBUG_TO_MEM(0xee);
	__halt();
}


