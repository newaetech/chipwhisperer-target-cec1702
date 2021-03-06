# ChipWhisperer CEC1702 Target #

This repo contains additional information for the CEC1702 target. Note this is only some appendix information, as you'll also want to see the following for full details:

**BEFORE USE, GO TO `Project>Edit Project` and make sure `Data Type Size` is set to `other (int 4 byte)`. If this is not done, the resulting firmware will not modify the efuse bits correctly.**


* NewAE Wiki Page: [wiki.newae.com/CW308T-CEC1702](https://wiki.newae.com/CW308T-CEC1702)
* Firmware example for hardware AES, part of main ChipWhisperer firmware example repo: [chipwhisperer/hardware/victims/firmware/CEC1702](https://github.com/newaetech/chipwhisperer/tree/develop/hardware/victims/firmware/CEC1702)

## E-Fuse Project Example ##

The "efuse" directory contains an example of setting EFUSE data. This is best done by using an external JTAG programming to load the file into internal SRAM & running the program.

The example application only disables ATE mode (used by us in production). But you can use this program to enable SPI flash encryption, setup ECC certificates, etc.
