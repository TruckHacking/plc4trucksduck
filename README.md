# PLC4TRUCKSduck

A PLC writing tool for the Truck Duck beaglebone based heavy vehicle diagnostic and debugging tool.

For information on the Truck Duck, see https://github.com/TruckHacking. A great big thank you to @haystack-ia and @sixvolts for creating this wonderful open tool.

For receiving PLC4TRUCKS signals, use an SDR and the https://github.com/ainfosec/gr-j2497 project. Also, the https://github.com/ainfosec/pretty_j1587 project can give you detailed decodings of those signals received.

This repo also contains also a reimplementation of the J1708 Truck Duck feature under new license. This should be a drop-in replacement for J1708 on the Truck Duck and is compatible with scripts from https://github.com/TruckHacking/py-hv-networks and https://github.com/JamesWJohnson/TruckCapeProjects

All sources here -- with the exception of `src/arm/BB-TRUCKCAPE-00A0.dts` -- are licensed under an MIT license. Alternative licenses for commercial use are available on request; please contact urban.jonson@nmfta.org . The device tree overlay source file `src/arm/BB-TRUCKCAPE-00A0.dts` is imported from the Truck Duck project with modifications and is licensed under GPL v2.

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

Also, these basic modifications result in a small amplitude PLC signal (1-2VPP) which will be sufficient for bench testing or small vehicle networks but more juice will be required for many applications. For this reason we will also -- eventually -- provide slightly more complicated modifications which will re-purpose unused components on the Truck Duck to create higher amplitude PLC signals (TODO).

We will aim to provide modification instructions for all of the Truck Duck variations. There are two options for each of the modifications, depending on how you intend to power your Truck Duck. If you intend to power your Truck Duck via the BBB DC 5V barrel jack connector then (in most cases) the modifications are more straightforward.

| Truck Duck Type | Powered by 5V Mods | Powered by Vechile Mods |
|-----------------|--------------------|-------------------------|
| Truck Duck (DEFCON 24)  | Connect a 0.1uF capacitor between P9.29 and unused screw terminal for PLC output. | Also put an inductor between the PTC fuse at power supply input and connect the PLC output directly to vehicle power on connectors. |
| University of Tulsa Truck Cape  |  Disconnect both C10 power supply filtering capacitor and input protection diode D1 from vehicle power. Connect a 0.1uF capacitor between P9.29 and vehicle power connector.  |  Also put an inductor between the D1 input and vehicle power and connect C10 to the low side of PTC1. |
| Truck Duck v1.5  | Connect a 0.1uF capacitor between P9.29 and unused screw terminal for PLC output. | Also put an inductor between the PTC fuse at power supply input and connect the PLC output directly to vehicle power on connectors. |
| Truck Duck MEGA (TODO)  |   |    |

Please see the following subsections for more details.

### Truck Duck Powered by 5V

If the Truck Duck is powered from the 5V barrel jack on the Beagle Bone then there is no need to filter the input to the Truck Duck 12V supply as in the next section.

#### Powered by 5V: Truck Duck (DEFCON 24)

The J1708_2 channel on the Truck Duck is non-functional. Re-purpose the 17-2H or 17-2L screw terminals on P3 to be the PLC output by first removing (or rotating to disconnect) R15 and R16. Add a 0.1uF capacitor to the P3-side pad of the removed resistor (e.g. R16 below) and connect the capacitor to P9.29 with a flying lead.

<img src="media/5V_power_td1.0_mods.jpg?raw=true" align=center width=300>

#### Powered by 5V: Truck Duck 1.5

There is an unused screw terminal on P4. Attach a 0.1uF capacitor to the underside of P4's screw terminal pin and connect the capacitor to P9.29 with a flying lead (threading through a via to cross the board).

<img src="media/5V_power_td1.5_mods1.jpg?raw=true" align=center width=300>
<img src="media/5V_power_td1.5_mods2.jpg?raw=true" align=center width=300>

#### Powered by 5V: University of Tulsa Truck Cape

The Truck Cape has an power input filtering capacitor (C10) which, if left connected, will attenuate all PLC4TRUCKS signals on the vehicle power bus to which it is connected. Even if you intend to power your Truck Cape from 5V only, you need to disconnect this capacitor and the rest of vehicle power input.

Desolder and lift the capacitor leg of C10 that is connected to vehicle power -- marked by a square plated through-hole. Then desolder and lift the leg of D1 that is connected to vehicle power -- the side which is _not_ marked by a square plated through-hole.

<img src="media/5V_truckcape_leglifts.jpg?raw=true" align=center width=300>

Then connected P9.29 to the vehicle power. Add a 0.1uF capacitor to the DB-15 pin marked `12V` and connect that capacitor to P9.29 with a short length of bodge wire. Secure the wire with adhesive (ignore the brown and orange bodge wires in the photo below -- they came factory installed).

<img src="media/5V_truckcape_addcap.jpg?raw=true" align=center width=300>
<img src="media/5V_truckcape_bodge.jpg?raw=true" align=center width=300>

### Truck Duck Powered by Vehicle (12V)

The Truck Duck 12V power supply will greatly attenuate any PLC signals on the lines connected to it. Some amount of wire between the 12V input and the PLC transmitter appears to alleviate this problem. Experiments on the bench indicate that between 40 and 72 inches of wire should be between the Truck Duck 12V input and a PLC transmitter.

When you want to use the Truck Duck itself to send PLC signals onto the 12V lines to which it is connected, some length of wire needs to be inserted. This could probably also be done with an inductor instead. But we all have some hook-up wire laying around, so we can make an inductor. It doesn't even need to be precise for it to work.

#### Powered by Vehicle: Truck Duck (DEFCON 24)

An inductor needs to be inserted between the power supply input and vehicle power. It is convenient to do this at the high side of the PTC fuse by cutting a trace. Only a small amount of inductance is required which can be satisfied by adding a coiled piece of wire.

<img src="media/12V_power_td1.0_mods.jpg?raw=true" align=center width=300>

The modification steps are identical to the *Powered by Vehicle: Truck Duck 1.5*, please see below.

#### Powered by Vehicle: Truck Duck 1.5

Cut the trace between the via and PTC1. Optionally re-cover with solder resist. If you aren't going to re-apply solder resist be very careful not to expose the copper ground plane when cutting the trace. You don't want to short the 12V input to ground accidentally when using your Truck Duck later.

Create the 'inductor' (piece of wire): wrap 40" of hook-up wire around a 3/8" drill bit, leave enough leads so that the wire can be soldered to the via and PTC1.

Solder the 'inductor' to the via side of PTC1 and to the via.

Attach a 0.1uF capacitor to the underside pin of '12V IN' on P3. Connect the capacitor to P9.29 with a flying lead, wrapping around the board edge. Secure the flying lead with glue on both sides of the board near the board edge.

<img src="media/12V_power_td1.5_mods1.jpg?raw=true" align=center width=300>
<img src="media/12V_power_td1.5_mods2.jpg?raw=true" align=center width=300>
<img src="media/12V_power_td1.5_mods3.jpg?raw=true" align=center width=300>

#### Powered by Vehicle: University of Tulsa Truck Cape

An inductor needs to be added between power supply filtering input and vehicle power. The disconnected C10 input filtering capacitor also should be reconnected on the low side of the added inductor. Only a small amount of inductance is required which could be satisfied by adding a coiled piece of wire; however, for this set of modifications we are showing adding a 100uH SMD inductor by glueing it dead-bug to the board.

<img src="media/12V_truckcape_gluedinduction.jpg?raw=true" align=center width=300>

Connect vehicle power to one side of the inductor and then the lifted leg of D1 to the other side. Then, finally, reconnect the input filtering capacitor C10 to the high side of Z1.

<img src="media/12V_truckcape_reconnection.jpg?raw=true" align=center width=300>

### Longer Range PLC Signals

TODO

## Usage

Utility commands `j1708dump.py` and `j1708send.py` are installed to enable command-line scripting. These commands are now available in the https://github.com/TruckHacking/py-hv-networks project.


## Features

* J1708 read and write on the Deutsch-9 and/or DB-15 J1708_LO and J1708_HI pins of the Truck Duck
  * no arbitration implemented, only frame detect
  * no error handling or reporting
  * known to occasionally return extra bytes on J1708 frames received
  * known to occasionally lock-up / stop receiving
  * drops send frames on (UDP socket) buffer full
  * can send and receive 255 byte payloads
  * drop receive frames on RX buffer full TODO

* PLC write on Truck Duck expansion
  * requires an AC coupling circuit from P9.29 to the power lines of the target such that 100Khz and above is passed
  * no arbitration implemented
  * no frame detect implemented TODO
  * minimal error handling
  * known to be using the dumbest PWM method possible (but it works)
  * drops send frames on (UDP socket) buffer full
  * drop receive frames on RX buffer full
  * can send and receive 255 byte payloads
  * cannot send distinct PLC preamble ID and J1708 payload MID from command line TODO
  * not confirmed to be able to be received by all trailer brake controllers TODO

* PLC read on Truck Duck expansion
  * requires an ADC populated on an expansion board to the Truck Duck TODO
