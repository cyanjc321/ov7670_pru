################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
PRUCAM_RPMsg_0/prucam_reg_io.obj: ../../prucam_rpmsg_0/prucam_reg_io.asm $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: PRU Compiler'
	"$(CG_TOOL_ROOT)/bin/clpru" -v3 -O2 --include_path="$(PRU_PACKAGE_PATH)/include" --include_path="../../prucam_rpmsg_0" --include_path="$(CG_TOOL_ROOT)/include" --define=am3359 --define=pru0 --diag_wrap=off --diag_warning=225 --display_error_number --endian=little --hardware_mac=on --preproc_with_compile --preproc_dependency="PRUCAM_RPMsg_0/prucam_reg_io.d" --obj_directory="PRUCAM_RPMsg_0" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '


