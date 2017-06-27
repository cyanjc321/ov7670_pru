/*
 * pru_ddr_burst.h
 *
 *  Created on: 31 May 2016
 *      Author: cjiang
 */

#ifndef PRU_DDR_BURST_H_
#define PRU_DDR_BURST_H_

#ifdef __cplusplus
extern "C" {
#endif
	extern void burst_sram_to_ddr(void* sram_addr, void* ram_addr, uint32_t size);
#ifdef __cplusplus
}
#endif

#endif /* PRU_DDR_BURST_H_ */
