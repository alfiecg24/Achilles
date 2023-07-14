# Boot

This directory contains the code used after the BootROM has been exploited, which involves booting the execution environment. This means it is responsible for the following things:
* Preparing and compressing the data to send to the device
* Sending the data to the device in download mode
* Ensuring that the environment is booted correctly

ROM patches are written and documented in `rom/`. iBoot patches are written and documented in `iboot/`. Preparation for the AlfieLoader environment is done in `boot.S` and `loaderMain.c`.