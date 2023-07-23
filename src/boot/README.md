# Boot

This directory contains the code used after the BootROM has been exploited, which involves booting the execution environment. This means it is responsible for the following things:
* Preparing and compressing the data to send to the device
* Sending the data to the device in download mode
* Ensuring that the environment is booted correctly

The code in [pongo/](src/boot/pongo) is responsible for preparing and compressing the PongoOS binary, adding shellcode, sending to the device and booting it. `pongo_helper.c` also contains the `jailbreakBoot()` function, which is responsible for booting the device and jailbreaking with palera1n.