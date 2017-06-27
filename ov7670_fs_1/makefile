-include makefile.init

# various directories
ODIR=obj
SRCDIR=src


CC=$(CG_TOOL_ROOT)/bin/clpru
HEX=$(CG_TOOL_ROOT)/bin/hexpru

RM := rm -rf
RMDIR := rm -rf

INCLUDE= --include_path="$(PRU_PACKAGE_PATH)/include" --include_path="$(CG_TOOL_ROOT)/include" -I src -I ../prucam_rpmsg_0/src

ifeq ($(DEBUG), 1)
	CFLAGS = -g
else
	CFLAGS = -O2
endif

CFLAGS +=--define=am3359 --define=pru0 --diag_wrap=off --diag_warning=225 --display_error_number --endian=little --hardware_mac=on -k --asm_listing --cross_reference 

# libraries and library search path
LDIR = -i"$(CG_TOOL_ROOT)/lib" 
LIBS = \
"$(SRCDIR)/AM335x_PRU.cmd" \
$(GEN_CMDS__FLAG) \
-l"$(PRU_PACKAGE_PATH)/lib/rpmsg_lib.lib" \
-llibc.a

LDFLAGS = $(LDIR) $(LIBS)

# objects
_OBJ += main.obj pru_ddr_burst.obj
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
EXTOBJ = ../prucam_rpmsg_0/obj/prucam_reg_io.obj

# Add inputs and outputs from these tool invocations to the build variables
PROJECT = ov7670_fs_0

EXE_OUTPUTS += $(PROJECT).out 
EXE_OUTPUTS__QUOTED += "$(PROJECT).out" 
BIN_OUTPUTS += $(PROJECT).hex 
BIN_OUTPUTS__QUOTED += "$(PROJECT).hex" 

all: $(EXE_OUTPUTS)

directory:
	[ -d $(ODIR) ] || mkdir $(ODIR)

$(ODIR)/%.obj: $(SRCDIR)/%.c $(GEN_OPTS) $(GEN_HDRS) directory
	@echo 'Building file: $<'
	@echo 'Invoking: PRU Compiler'
	$(CC) -v3 $(CFLAGS) $(INCLUDE) --preproc_with_compile --preproc_dependency --pp_directory=$(ODIR) --obj_directory=$(ODIR) $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '
	
$(ODIR)/%.obj: $(SRCDIR)/%.asm $(GEN_OPTS) $(GEN_HDRS) directory
	@echo 'Building file: $<'
	@echo 'Invoking: PRU Compiler'
	$(CC) -v3 $(CFLAGS) $(INCLUDE) --preproc_with_compile --preproc_dependency --pp_directory=$(ODIR) --obj_directory=$(ODIR) $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

dep:
	cd ../prucam_rpmsg_0 && make 

$(EXE_OUTPUTS): $(OBJ) $(CMD_SRCS) $(GEN_CMDS) dep
	@echo 'Building target: $@'
	@echo 'Invoking: PRU Linker'
	$(CC) -v3 $(CFLAGS) $(INCLUDE) -z -m"$(PROJECT).map" --stack_size=0x100 --heap_size=0x100 --reread_libs --define=PRU_CORE=1 --warn_sections --xml_link_info="$(PROJECT)_linkInfo.xml" --rom_model -o $(EXE_OUTPUTS) $(OBJ) $(EXTOBJ) $(LDFLAGS)
	@echo 'Finished building target: $@'
	@echo ' '

$(BIN_OUTPUTS): $(EXE_OUTPUTS)
	@echo 'Invoking: PRU Hex Utility'
	$(HEX)  -o $(BIN_OUTPUTS__QUOTED) $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $@'
	@echo ' '


.PHONY: clean all directory dep clean_dep

clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)$(BIN_OUTPUTS__QUOTED)
	-$(RM) *.asm
	-$(RM) *_linkInfo.xml *.map
	-$(RMDIR) $(ODIR)
	-@echo 'Finished clean'
	-@echo ' '

clean_dep:
	cd ../prucam_rpmsg_0 && make clean 
