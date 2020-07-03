# plc4trucksduck

A PLC writing tool for the Truck Duck beaglebone based heavy vehicle diagnostic and debugging tool.

For information on the TruckDuck, see https://github.com/TruckHacking. A great big thank you to @haystack-ia and @sixvolts for creating this wonderful open tool.

This repo also contains also a reimplementation of the J1708 Truck Duck feature under new license. This should be a drop-in replacement for J1708 on the TruckDuck and is compatible with scripts from https://github.com/TruckHacking/py-hv-networks and https://github.com/JamesWJohnson/TruckCapeProjects

All sources here -- with the exception of `src/arm/BB-TRUCKCAPE-00A0.dts` -- are licensed under the [Creative Commons `BY-NC-ND` 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) for non-commercial use. Alternative licenses for commercial use are available on request; please contact urban.jonson@nmfta.org . The device tree overlay source file `src/arm/BB-TRUCKCAPE-00A0.dts` is imported from the Truck Duck project with modifications and is licensed under GPL v2.

## Installing

1. starting with the truckhacking.github.io image http://truckduck.sixvolts.org/images/truck-duck-25aug2016.img.xz
2. install TI PRU C compiler `wget http://downloads.ti.com/codegen/esd/cgt_public_sw/PRU/2.1.1/ti_cgt_pru_2.1.1_armlinuxa8hf_busybox_installer.sh; sudo bash -x ti_cgt_pru_2.1.1_armlinuxa8hf_busybox_installer.sh` . Default install location is `/usr/share/ti/cgt-pru/lib/`
3. get pru-support-package onto the beaglebone from git: `cd /opt; sudo git clone https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/`
4. If you used locations other than the default, set Makefile variables `PRU_CGT` and `PRU_SUPPORT` (via edits or via env) to point to above.
4. Because the pypruss package was only working in python2.7, this tool (like the j1708 TruckDuck tools) uses both python2.7 and python3 (3.5 tested here). Install both the python2.7 `requirements-2.7.txt` and python3 `requirements.txt` dependencies. You may need to first install python2.7 pip: `sudo apt-get install python-pip`. Then `sudo python2.7 -m pip install -r requirements-2.7.txt` and `sudo python3 -m pip install -r requirements.txt`
5. Build the PRU binaries and device tree overlays with `make`
6. Install them, the services for J1708 and PLC and disable the old TruckDuck J1708 stuff with `sudo make install`
7. First time installs will require a reboot at this point

## Features

* J1708 read and write on the Deutsch-9 and/or DB-15 J1708_LO and J1708_HI pins of the Truck Duck
  * no arbitration implemented, only frame detect
  * no error handling or reporting
  * known to occasionally return extra bytes on J1708 frames received
  * known to occasionally lock-up / stop receiving
  * can only send 42 byte payloads TODO
  * drops send frames on TX buffer full TODO
  * drop receive frames on RX buffer full TODO

* PLC write on Truck Duck expansion
  * requires an AC coupling circuit from P9.29 to the power lines of the target such that 100Khz and above is passed. Details TODO
  * no arbitration implemented
  * no frame detect implemented TODO
  * minimal error handling
  * known to be using the dumbest PWM method possible (but it works)
  * can only send 42 byte payloads TODO
  * drops send frames on TX buffer full TODO
  * drop receive frames on RX buffer full TODO
  * cannot send disting PLC preamble ID and J1708 payload MID TODO
  * not confirmed to be able to be received by all trailer brake controllers TODO

* PLC read on Truck Duck expansion
  * requires an ADC populated on an expansion board to the Truck Duck TODO
