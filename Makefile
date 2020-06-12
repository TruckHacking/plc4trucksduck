PRU_CGT:=/usr/share/ti/cgt-pru
TI_AM335X:=/opt/ti-sdk-am335x-evm-07.00.00.00
PRU_SUPPORT:=/opt/pru-software-support-package

ARM_SRC_DIR=src/arm
ARM_BUILD_DIR=src/arm/build

PARENT_DTS_SOURCE=$(ARM_SRC_DIR)/BB-TRUCKCAPE-00A0.dts
PARENT_DTB_OFILE=$(ARM_BUILD_DIR)/BB-TRUCKCAPE-00A0.dtbo
# 16-byte DTBO cape name limit
THIS_DTS_SOURCE=$(ARM_SRC_DIR)/BB-PLC4TRUCKSDUC-00A0.dts
THIS_DTB_OFILE=$(ARM_BUILD_DIR)/BB-PLC4TRUCKSDUC-00A0.dtbo

DTB_OFILES = $(THIS_DTB_OFILE) $(PARENT_DTB_OFILE)

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

CFLAGS=--c99 --silicon_version=3 -o1 --keep_asm --asm_directory=$(PRU_GEN_DIR) --obj_directory=$(PRU_GEN_DIR) --pp_directory=$(PRU_GEN_DIR) -ss --symdebug:none --display_error_number --diag_remark=1119
INC=-I$(TI_AM335X)/example-applications/pru/include -I$(PRU_CGT)/usr/share/ti/cgt-pru/include -I$(PRU_SUPPORT)/include -I$(PRU_SUPPORT)/include/am335x
LFLAGS=
LIBS=--library=$(PRU_CGT)/usr/share/ti/cgt-pru/lib/libc.a

all: $(PRU_BINS) $(DTB_OFILES)

$(PARENT_DTB_OFILE): $(PARENT_DTS_SOURCE)
$(THIS_DTB_OFILE): $(THIS_DTS_SOURCE)

$(J17084TRUCKDUCK_OFILE): $(J17084TRUCKDUCK_SOURCES)
# cmdfile then ofile, order is important
$(J17084TRUCKDUCK_BINS): $(J17084TRUCKDUCK_CMDFILE) $(J17084TRUCKDUCK_OFILE)

$(PLC4TRUCKSDUCK_OFILE): $(PLC4TRUCKSDUCK_SOURCES)
# cmdfile then ofile, order is important
$(PLC4TRUCKSDUCK_BINS): $(PLC4TRUCKSDUCK_CMDFILE) $(PLC4TRUCKSDUCK_OFILE)

$(ARM_BUILD_DIR):
	mkdir -p $(ARM_BUILD_DIR)

$(ARM_BUILD_DIR)/%.dtbo: $(ARM_SRC_DIR)/%.dts | $(ARM_BUILD_DIR)
	dtc -O dtb -I dts -o $@ -b 0 -@ $<

$(PRU_GEN_DIR):
	mkdir -p $(PRU_GEN_DIR)

$(PRU_BUILD_DIR):
	mkdir -p $(PRU_BUILD_DIR)

$(PRU_GEN_DIR)/%.fw : $(PRU_SRC_DIR)/%.c | $(PRU_GEN_DIR)
	$(PRU_CGT)/usr/bin/clpru $(CFLAGS) $(INC) $^ --run_linker $(LFLAGS) $(LIBS) --library=$(PRU_SUPPORT)/examples/am335x/PRU_PRUtoARM_Interrupt/AM335x_PRU.cmd --output_file=$@ -m$(patsubst %.fw,%.map,$@)

$(PRU_BUILD_DIR)/%.bin $(PRU_GEN_DIR)/%_data.bin : $(PRU_SRC_DIR)/%.cmd $(PRU_GEN_DIR)/%.fw | $(PRU_BUILD_DIR) $(PRU_GEN_DIR)
	$(PRU_CGT)/usr/bin/hexpru $^

clean:
	rm -rf $(PRU_GEN_DIR) $(PRU_BUILD_DIR) $(ARM_BUILD_DIR)

check:
	~/.local/bin/flake8 src/arm/*.py

install: $(DTB_OFILES)
	echo manual > /etc/init/ecm.override
	echo manual > /etc/init/non_ecm.override
	install -m 600 $(PARENT_DTB_OFILE) /lib/firmware/
	install -m 600 $(THIS_DTB_OFILE) /lib/firmware/
	# TODO: install a upstart conf for plc4trucksduck also
	install -m 700 $(ARM_SRC_DIR)/j17084truckduck_host.py /usr/local/bin/
	install -m 600 $(PRU_BUILD_DIR)/j17084truckduck.bin /lib/firmware
	install -m 644 $(ARM_SRC_DIR)/j17084truckduck.conf /etc/init/
	@echo NOTE /// You must reboot before you can load $(THIS_DTB_OFILE) ///

run:
	-initctl stop ecm
	-initctl stop dpa
	echo BB-PLC4TRUCKSDUC > $(wildcard /sys/devices/bone_capemgr.?/slots)

.PHONY: all clean check install
