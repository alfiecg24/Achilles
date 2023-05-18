# Boot

This directory contains the code used after the BootROM has been exploited, up until (not including) booting the AlfieLoader environment is loaded. This means it is responsible for the following things:
* Performing patches in the ROM
* Performing patches in iBoot
* Preparing to boot the AlfieLoader environment

ROM patches are written and documented in `rom/`. iBoot patches are written and documented in `iboot/`. Preparation for the AlfieLoader environment is done in `boot.S` and `loaderMain.c`.