################################################################################
# Automatically-generated file. Do not edit!
################################################################################

CG_TOOL_ROOT := /home/cjiang/opt/ti-cgt-pru_2.1.2

GEN_OPTS__FLAG :=
GEN_CMDS__FLAG :=

ORDERED_OBJS += \
"./main.obj" \
"./prucam_reg_io.obj" \
"../AM335x_PRU.cmd" \
$(GEN_CMDS__FLAG) \
-l"/home/cjiang/opt/ti-processor-sdk-linux-am335x-evm-03.00.00.04/example-applications/pru-icss-4.0.2/lib/rpmsg_lib.lib" \
-llibc.a \

-include ../makefile.init

RM := rm -rf
RMDIR := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables
EXE_OUTPUTS += \
PRUCAM_RPMsg_0.out \

EXE_OUTPUTS__QUOTED += \
"PRUCAM_RPMsg_0.out" \

BIN_OUTPUTS += \
PRUCAM_RPMsg_0.hex \

BIN_OUTPUTS__QUOTED += \
"PRUCAM_RPMsg_0.hex" \


# All Target
all: PRUCAM_RPMsg_0.out

# Tool invocations
PRUCAM_RPMsg_0.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: $@'
	@echo 'Invoking: PRU Linker'
	"/home/cjiang/opt/ti-cgt-pru_2.1.2/bin/clpru" -v3 -O2 --define=am3359 --define=pru0 --diag_wrap=off --diag_warning=225 --display_error_number --endian=little --hardware_mac=on -k --asm_listing --cross_reference -z -m"PRUCAM_RPMsg_0.map" --stack_size=0x100 --heap_size=0x100 -i"/home/cjiang/opt/ti-cgt-pru_2.1.2/lib" -i"/home/cjiang/opt/ti-cgt-pru_2.1.2/include" --reread_libs --define=PRU_CORE=1 --diag_wrap=off --display_error_number --warn_sections --xml_link_info="PRUCAM_RPMsg_0_linkInfo.xml" --rom_model -o "PRUCAM_RPMsg_0.out" $(ORDERED_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '

PRUCAM_RPMsg_0.hex: $(EXE_OUTPUTS)
	@echo 'Invoking: PRU Hex Utility'
	"/home/cjiang/opt/ti-cgt-pru_2.1.2/bin/hexpru"  -o "PRUCAM_RPMsg_0.hex" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)$(BIN_OUTPUTS__QUOTED)
	-$(RM) "main.d"
	-$(RM) "main.obj" "prucam_reg_io.obj"
	-$(RM) "prucam_reg_io.d"
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

