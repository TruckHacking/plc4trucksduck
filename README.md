# PLC4TRUCKSduck

A PLC writing tool for the Truck Duck beaglebone based heavy vehicle diagnostic and debugging tool.

For information on the Truck Duck, see https://github.com/TruckHacking. A great big thank you to @haystack-ia and @sixvolts for creating this wonderful open tool.

This repo also contains also a reimplementation of the J1708 Truck Duck feature under new license. This should be a drop-in replacement for J1708 on the Truck Duck and is compatible with scripts from https://github.com/TruckHacking/py-hv-networks and https://github.com/JamesWJohnson/TruckCapeProjects

All sources here -- with the exception of `src/arm/BB-TRUCKCAPE-00A0.dts` -- are licensed under the [Creative Commons `BY-NC-ND` 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) for non-commercial use. Alternative licenses for commercial use are available on request; please contact urban.jonson@nmfta.org . The device tree overlay source file `src/arm/BB-TRUCKCAPE-00A0.dts` is imported from the Truck Duck project with modifications and is licensed under GPL v2.

## Installing

1. starting with the truckhacking.github.io image http://truckduck.sixvolts.org/images/truck-duck-25aug2016.img.xz
2. install TI PRU C compiler `wget http://downloads.ti.com/codegen/esd/cgt_public_sw/PRU/2.1.1/ti_cgt_pru_2.1.1_armlinuxa8hf_busybox_installer.sh; sudo bash -x ti_cgt_pru_2.1.1_armlinuxa8hf_busybox_installer.sh` . Default install location is `/usr/share/ti/cgt-pru/lib/`
3. get pru-support-package onto the beaglebone from git: `cd /opt; sudo git clone https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/`
4. If you used locations other than the default, set Makefile variables `PRU_CGT` and `PRU_SUPPORT` (via edits or via env) to point to above.
4. Because the pypruss package was only working in python2.7, this tool (like the j1708 Truck Duck tools) uses both python2.7 and python3 (3.5 tested here). Install both the python2.7 `requirements-2.7.txt` and python3 `requirements.txt` dependencies. You may need to first install python2.7 pip: `sudo apt-get install python-pip`. Then `sudo python2.7 -m pip install -r requirements-2.7.txt` and `sudo python3 -m pip install -r requirements.txt`
5. Build the PRU binaries and device tree overlays with `make`
6. Install them, the services for J1708 and PLC and disable the old Truck Duck J1708 stuff with `sudo make install`
7. First time installs will require a reboot at this point
8. You can confirm that the PRU is running the J1708 and PLC code checking `initctl status j17084truckduck` and `initctl status plc4trucksduck`

No hardware modifications are necessary to use J1708 with this package. To use PLC, a capacitor and a couple bodge wires must be installed on your Truck Duck.

## Hardware Modifications

To send PLC signals some hardware modifications are needed. The Truck Duck power supply is very good at attenuating PLC signals on any 12V supply line it is attached to. For that reason we provide a simple modification to perform that will work only in the case that the Truck Duck is connected to 5V power and also a more involved set of modifications for that will work in either case of powering the Truck Duck from 5V or from 12V.

Also, these basic modifications result in a small amplitude PLC signal (1-2VPP) which will be sufficient for bench testing or small vehicle networks but more juice will be required for many applications. For this reason we also provide slightly more complicated modifications which will re-purpose unused components on the Truck Duck to create higher amplitude PLC signals.

We will aim to provide modification instructions for all of the following Truck Duck variations:
* Truck Duck (DEFCON 24)
* University of Tulsa Truck Cape (TODO)
* Truck Duck v1.5
* Truck Duck MEGA (TODO)

### Truck Duck Powered by 5V

If the Truck Duck is powered from the 5V barrel jack on the Beagle Bone then there is no need to filter the input to the Truck Duck 12V supply as in the next section.

#### Truck Duck (DEFCON 24)

The J1708_2 channel on the Truck Duck is non-functional. Re-purpose the 17-2H or 17-2L screw terminals on P3 to be the PLC output by first removing (or rotating to disconnect) R15 and R16. Add a 0.1uF capacitor to the P3-side pad of the removed resistor (e.g. R16 below) and connect the capacitor to P9.29 with a flying lead.

<img src="media/5V_power_td1.0_mods.jpg?raw=true" align=center width=100>

#### Truck Duck 1.5

There is an unused screw terminal on P4. Attach a 0.1uF capacitor to the underside of P4's screw terminal pin and connect the capacitor to P9.29 with a flying lead (threading through a via to cross the board).

<img src="media/5V_power_td1.5_mods1.jpg?raw=true" align=center width=100>
<img src="media/5V_power_td1.5_mods2.jpg?raw=true" align=center width=100>

### Truck Duck Powered by 12V (or 5V)

The Truck Duck 12V power supply will greatly attenuate any PLC signals on the lines connected to it. Some amount of wire between the 12V input and the PLC transmitter appears to alleviate this problem. Experiments on the bench indicate that between 40 and 72 inches of wire should be between the Truck Duck 12V input and a PLC transmitter.

When you want to use the Truck Duck itself to send PLC signals onto the 12V lines to which it is connected, some length of wire needs to be inserted. This could probably also be done with an inductor instead. But we all have some hook-up wire laying around, so we can make an inductor. It doesn't even need to be precise for it to work.

#### Truck Duck 1.5

Cut the trace between the via and PTC1. Optionally re-cover with solder resist. If you aren't going to re-apply solder resist be very careful not to expose the copper ground plane when cutting the trace. You don't want to short the 12V input to ground accidentally when using your Truck Duck later.

Create the 'inductor' (piece of wire): wrap 40" of hook-up wire around a 3/8" drill bit, leave enough leads so that the wire can be soldered to the via and PTC1.

Solder the 'inductor' to the via side of PTC1 and to the via.

Attach a 0.1uF capacitor to the underside pin of '12V IN' on P3. Connect the capacitor to P9.29 with a flying lead, wrapping around the board edge. Secure the flying lead with glue on both sides of the board near the board edge.

<img src="media/12V_power_td1.5_mods1.jpg?raw=true" align=center width=100>
<img src="media/12V_power_td1.5_mods2.jpg?raw=true" align=center width=100>
<img src="media/12V_power_td1.5_mods3.jpg?raw=true" align=center width=100>

### Longer Range PLC Signals

TODO

## Usage

Utility commands `j1708dump.py` and `j1708send.py` are installed to enable command-line scripting. But also anything built with https://github.com/TruckHacking/py-hv-networks and https://github.com/JamesWJohnson/TruckCapeProjects will work. The J1708 (1) interface is unchanged and PLC is substituted for the J1708 (2) interface.

```sh
$ j1708dump.py --help
usage: j1708dump.py [-h] [--interface [{j1708,j1708_2,plc}]]
                    [--show-checksums [{true,false}]]
                    [--validate [{true,false}]] [--show [SHOW]]
                    [--hide [HIDE]]

frame dumping utility for J1708 and PLC on truckducks

optional arguments:
  -h, --help            show this help message and exit
  --interface [{j1708,j1708_2,plc}]
                        choose the interface to dump from
  --show-checksums [{true,false}]
                        show frame checksums
  --validate [{true,false}]
                        discard frames with invalid checksums
  --show [SHOW]         specify a candump-like filter; frames matching this
                        filter will be shown. Processed before hide filters.
                        e.g. "ac:ff" to show only MID 0xAC frames
  --hide [HIDE]         specify a candump-like filter; frames matching this
                        filter will be hidden. e.g. "89:ff" to hide MID 0x89
                        frames
```

```sh
$ j1708send.py --help
usage: j1708send.py [-h] [--interface [{j1708,j1708_2,plc}]]
                    [--checksums [{true,false}]]
                    hexbytes

frame sending utility for J1708 and PLC on truckducks

positional arguments:
  hexbytes              a j1708 or plc message to send e.g. '0a00' or '0a,00'
                        or '0a#00' or '(123.123) j1708 0a#00'

optional arguments:
  -h, --help            show this help message and exit
  --interface [{j1708,j1708_2,plc}]
                        choose the interface to send frames to
  --checksums [{true,false}]
                        add checksums to frames sent
```


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
