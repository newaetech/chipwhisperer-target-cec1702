# CEC1702-EFUSE
Firmware to modify EFUSE bits in CEC1702

**BEFORE USE, GO TO `Project>Edit Project` and make sure `Data Type Size` is set to `other (int 4 byte)`. If this is not done, the resulting firmware will not modify the efuse bits correctly.**

To use, modify efuse_data.h with the index and value of the bytes your want to modify. For example, the first index/value pair disables ATE mode by setting bit 7 of byte 35 to 1. This table should be terminated by index 0xDEAD and value 0xFF, as is shown in the table.

It is strongly recommended that the user read [the CW308T-CEC1702 wiki page](https://wiki.newae.com/CW308T-CEC1702) before running or modifying this firmware (in particular the ATE and EFUSE section) as modifying EFUSE has the potential to irreversibly harm the device.

## Modifications from mikroC's firmware
This firmware has been modified from mikroC's original example due to differences between development boards and the CW308 target board. Both their original code and the instructions given in the CEC1702's datasheet assumed that the firmware has a way to ground and power the VREF pin, as FSOURCE_EN_READ and FSOURCE_EN_PRGM are both 1 when changing from read to program (and vice versa), which shorts the ground and power pins.

Instead, this firmware sets both bits low and checks them before setting the other high.

## Other Notes
The firmware check which locations and values the user is attempting to modify, so if you need to change other bits, you'll need to edit this check.
