# plc4trucksduck

A PLC writing tool for the Truck Duck beaglebone based heavy vehicle diagnostic and debugging tool.

For information on the TruckDuck, see https://github.com/TruckHacking. A great big thank you to @haystack-ia and @sixvolts for creating this wonderful open tool.

This repo also contains also a reimplementation of the J1708 Truck Duck feature under new license. This should be a drop-in replacement for J1708 on the TruckDuck and is compatible with scripts from https://github.com/TruckHacking/py-hv-networks and https://github.com/JamesWJohnson/TruckCapeProjects

All sources here -- with the exception of `src/arm/BB-TRUCKCAPE-00A0.dts` -- are licensed under the [Creative Commons `BY-NC-ND` 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) for non-commercial use. Alternative licenses for commercial use are available on request; please contact urban.jonson@nmfta.org . The device tree overlay source file `src/arm/BB-TRUCKCAPE-00A0.dts` is imported from the Truck Duck project with modifications and is licensed under GPL v2.

## Installing

1. starting with the truckhacking.github.io image http://truckduck.sixvolts.org/images/truck-duck-25aug2016.img.xz
2. install TI PRU C compiler `ti_cgt_pru_2.1.1_armlinuxa8hf_busybox_installer.sh` from http://downloads.ti.com/codegen/esd/cgt_public_sw/PRU/2.1.1/ti_cgt_pru_2.1.1_armlinuxa8hf_busybox_installer.sh
3. install SDK: both http://downloads.ti.com/sitara_linux/esd/AM335xSDK/exports/ti-sdk-am335x-evm-07.00.00.00-Linux-x86-Install.bin and http://software-dl.ti.com/sitara_linux/esd/PRU-SWPKG/01_00_00_00/exports/pru-addon-v1.0-Linux-x86-Install.bin on a 32-bit x86 capable system
4. transfer the resulting install directories to the beaglebone
5. get pru-support-package onto the beaglefone from git: `git clone https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/`
6. set Makefile variables `PRU_CGT`, `TI_AM335X` and `PRU_SUPPORT` (via edits or via env) to point to above

## Features

* J1708 read and write on the Deutsch-9 and/or DB-15 J1708_LO and J1708_HI pins of the Truck Duck
  * no arbitration implemented
  * no error handling or reporting
  * known to occasionally return extra bytes on J1708 frames received
  * known to occasionally lock-up / stop receiving

* PLC write on Truck Duck expansion
  * requires an AC coupling circuit from P9.29 to the power lines of the target such that 100Khz and above is passed. Details TODO
  * no arbitration implemented
  * no frame detect implemented TODO
  * minimal error handling
  * known to loose sync while sending TODO
  * known to be using the dumbest PWM method possible
  * known to have timings out of spec TODO
  
* PLC read on Truck Duck expansion
  * requires an ADC populated on an expansion board to the Truck Duck TODO
