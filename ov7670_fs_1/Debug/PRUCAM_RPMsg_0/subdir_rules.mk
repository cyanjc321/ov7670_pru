################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
PRUCAM_RPMsg_0/prucam_reg_io.obj: /home/cjiang/workspace/ctomv_workspace/PRUCAM_RPMsg_0/prucam_reg_io.asm $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: PRU Compiler'
	"/home/cjiang/ti/ccsv6/tools/compiler/ti-cgt-pru_2.1.2/bin/clpru" -v3 --include_path="/home/cjiang/ti/ccsv6/ccs_base/pru/include" --include_path="/home/cjiang/workspace/ctomv_workspace/PRUCAM_RPMsg_0" --include_path="/home/cjiang/ti/ti-processor-sdk-linux-am335x-evm-02.00.02.11/example-applications/pru-icss-4.0.2/include" --include_path="/home/cjiang/ti/ccsv6/tools/compiler/ti-cgt-pru_2.1.2/include" -g --define=am3359 --define=pru0 --diag_wrap=off --diag_warning=225 --display_error_number --endian=little --hardware_mac=on --preproc_with_compile --preproc_dependency="PRUCAM_RPMsg_0/prucam_reg_io.d" --obj_directory="PRUCAM_RPMsg_0" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '


