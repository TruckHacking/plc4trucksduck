PRU_CGT:=/usr/share/ti/cgt-pru
TI_AM335X:=/opt/ti-sdk-am335x-evm-07.00.00.00
PRU_SUPPORT:=/opt/pru-software-support-package

PRU_SRC_DIR=src/pru
PRU_GEN_DIR=src/pru/generated
PRU_BUILD_DIR=src/pru/build

J17084TRUCKDUCK_SOURCES=$(PRU_SRC_DIR)/j17084truckduck.c
J17084TRUCKDUCK_OFILE=$(PRU_GEN_DIR)/j17084truckduck.fw
J17084TRUCKDUCK_BINS=$(PRU_BUILD_DIR)/j17084truckduck.bin $(PRU_GEN_DIR)/j17084truckduck_data.bin
J17084TRUCKDUCK_CMDFILE=$(PRU_SRC_DIR)/j17084truckduck.cmd

PLC4TRUCKSDUCK_SOURCES=$(PRU_SRC_DIR)/plc4trucksduck.c
PLC4TRUCKSDUCK_OFILE=$(PRU_GEN_DIR)/plc4trucksduck.fw
PLC4TRUCKSDUCK_BINS=$(PRU_BUILD_DIR)/plc4trucksduck.bin $(PRU_GEN_DIR)/plc4trucksduck_data.bin
PLC4TRUCKSDUCK_CMDFILE=$(PRU_SRC_DIR)/plc4trucksduck.cmd

PRU_BINS=$(J17084TRUCKDUCK_BINS) $(PLC4TRUCKSDUCK_BINS)

CFLAGS=--c99 --silicon_version=3 -o1 --keep_asm --asm_directory=$(PRU_GEN_DIR) --obj_directory=$(PRU_GEN_DIR) --pp_directory=$(PRU_GEN_DIR)
INC=-I$(TI_AM335X)/example-applications/pru/include -I$(PRU_CGT)/usr/share/ti/cgt-pru/include -I$(PRU_SUPPORT)/include -I$(PRU_SUPPORT)/include/am335x
LFLAGS=
LIBS=--library=$(PRU_CGT)/usr/share/ti/cgt-pru/lib/libc.a

all: $(PRU_BINS)

$(J17084TRUCKDUCK_OFILE): $(J17084TRUCKDUCK_SOURCES)
# cmdfile then ofile, order is important
$(J17084TRUCKDUCK_BINS): $(J17084TRUCKDUCK_CMDFILE) $(J17084TRUCKDUCK_OFILE)

$(PLC4TRUCKSDUCK_OFILE): $(PLC4TRUCKSDUCK_SOURCES)
# cmdfile then ofile, order is important
$(PLC4TRUCKSDUCK_BINS): $(PLC4TRUCKSDUCK_CMDFILE) $(PLC4TRUCKSDUCK_OFILE)

$(PRU_GEN_DIR):
	mkdir -p $(PRU_GEN_DIR)

$(PRU_BUILD_DIR):
	mkdir -p $(PRU_BUILD_DIR)

$(PRU_GEN_DIR)/%.fw : $(PRU_SRC_DIR)/%.c | $(PRU_GEN_DIR)
	$(PRU_CGT)/usr/bin/clpru $(CFLAGS) $(INC) $< --run_linker $(LFLAGS) $(LIBS) --library=$(PRU_SUPPORT)/examples/am335x/PRU_PRUtoARM_Interrupt/AM335x_PRU.cmd --output_file=$@ -m$(patsubst %.fw,%.map,$@)

$(PRU_BUILD_DIR)/%.bin $(PRU_GEN_DIR)/%_data.bin : $(PRU_SRC_DIR)/%.cmd $(PRU_GEN_DIR)/%.fw | $(PRU_BUILD_DIR) $(PRU_GEN_DIR)
	$(PRU_CGT)/usr/bin/hexpru $^

clean:
	rm -rf $(PRU_GEN_DIR) $(PRU_BUILD_DIR)

check:
	~/.local/bin/flake8 src/arm/*.py
.PHONY: all clean check
