/* should be used from working directory ../../ */
-b
-image
ROMS {
                PAGE 0:
                text: o = 0x0, l = 0x1000, files={src/pru/build/plc4trucksduck.bin}
                PAGE 1:
                data: o = 0x0, l = 0x1000, files={src/pru/generated/plc4trucksduck_data.bin}
}
