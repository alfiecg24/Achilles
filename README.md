# AlfieLoader

## What is AlfieLoader?
A (very early in development) pre-boot execution environment for Apple mobile devices vulnerable to checkm8 - a work-in-progress project similar to [checkra1n](https://checkra.in/)/[PongoOS](https://github.com/checkra1n/PongoOS). While it is not at this stage yet, it is the goal for this project.

## Why is it called AlfieLoader?
I wanted to get started with this project as soon as possible in the beginning, and did not want to spend hours contemplating different names. I subsequently decided to name it AlfieLoader, as my name is Alfie. While it may change in the future, it will remain as is until further notice.

## What does it do?
In its current state, this project does the following things:
* Has a USB/device handling system in place
* Runs the checkm8 exploit on T8011

## What are the current limitations?
* I can only test this on an iPad Pro 10.5" (T8011)
* Only planned support is for A7-A11 devices
* A12+ devices cannot be supported unless a BootROM vulnerability is found and publicised

## What is planned for the future?
* checkm8 BootROM exploit implementation for all devices
* ROM and iBoot dynamic patching
* Booting the AlfieLoader environment
* Kernel patching and booting said kernel
* A full userland jailbreak for all versions >= 15.0
* SEP exploitation with blackbird
* Support for downgrades/dualboots from the main AlfieLoader environment

## Dependencies
* [libimobiledevice](https://github.com/libimobiledevice/libimobiledevice)

You will need to specify the version of your libimobiledevice installation in the `Makefile`, in the `FRAMEWORKS` arguments. The default version is `1.0.6`. This will hopefully be resolved in the future so that the project can be built by simply running `make`.

## Documentation
I have made an effort to document the code as much as possible, and in all header files you will find a description of a function's purpose, parameters and return value for each function exported in the header. Hopefully this can help anyone who wants to learn more about the BootROM and iOS bootchain understand what is going on a bit better.

Furthermore, I have included a detailed writeup on the vulnerabilities exploited in the checkm8 exploit, including the not very well documented memory leak and subsequent exploitation of it. I tried my best to provide definitions where possible and explain the process as clearly as I could in the effort to make it simple for anyone with a basic understanding of vulnerability exploitation to understand. Head over to the [docs](docs/) directory to check out the writeup.

If you find any issues with the documentation, please open an issue or submit a pull request to help improve it.

## Disclaimer
You may find that there are similarities in here to other projects like this - and that is true. This was just a personal project for me to learn more about the BootROM and iOS bootchain. As a result, I relied a lot on open-source solutions such as [ipwndfu](https://github.com/axi0mX/ipwndfu), [gaster](https://github.com/0x7ff/gaster) and [PongoOS](https://github.com/checkra1n/PongoOS) to help me along the way.

## Credits
* Axi0mX - checkm8, [ipwndfu](https://github.com/axi0mX/ipwndfu)
