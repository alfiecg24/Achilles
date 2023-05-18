# AlfieLoader

## What is AlfieLoader?
A pre-boot execution environment for Apple mobile devices vulnerable to checkm8 - a work-in-progress project similar to [checkra1n](https://checkra.in/)/[PongoOS](https://github.com/checkra1n/PongoOS).

## What does it do?
In its current state, this project does the following things:
* Nothing - work has not begun yet and so far I have implemented a barebones project system. It has an organised project structure, a Makefile system and a logging system.

## What are the current limitations?
* I can only test this on an iPad Pro 10.5" (T8010)
* Only planned support is for A7-A11 devices
* A12+ devices cannot be supported unless a BootROM vulnerability is found and publicised

## What is planned for the future?
* Custom checkm8 BootROM exploit implementation
* ROM and iBoot dynamic patching
* Booting the AlfieLoader environment
* Kernel patching and booting said kernel
* A full userland jailbreak for all versions
* SEP exploitation with blackbird
* Support for downgrades/dualboots from the main AlfieLoader environment