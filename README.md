# Achilles

[![Time spent on Achilles](https://wakatime.com/badge/user/61592169-b9cf-4af8-b6fa-8ac7d4369b01/project/c122878a-9ce8-40b1-a337-e53a7ef5f337.svg)](https://wakatime.com/badge/user/61592169-b9cf-4af8-b6fa-8ac7d4369b01/project/c122878a-9ce8-40b1-a337-e53a7ef5f337)

## What is Achilles?
Achilles is a project that aims to provide a easy way to harness the power of the checkm8 BootROM exploit. It has three prominent features:
* Exploitation using gaster payloads to patch signature checks
* Booting PongoOS as a drop-in replacement for checkra1n 1337
* Jailbreaking rootless with palera1n

This project is nowhere near the maturity of the original versions of these projects, but was only created as a personal project to learn more about the BootROM and iOS bootchain. As such, I am in no way denouncing the above projects, and would in fact recommend using them over Achilles at the moment, as they are much more mature and stable. The project is still much in development, and so I do not recommend using it unless you know what you are doing.

Achilles has support for both IOKit and libusb as a USB backend. To compile with IOKit, ensure you are on a Mac and run `make`. For libusb, run `make libusb`. At the moment, macOS is the only supported operating system, but I plan to add support for Linux and Windows in the future - however, I do not have a Windows or Linux machine to test on, so this may take a while. If you want to help with this, please submit a pull request.

Jailbreaking with palera1n is highly experimental and Achilles does not support certain options that the palera1n program provides support for (such as `--force-revert`). I have ran into several issues while using Achilles to jailbreak my device, and as such I would recommend using the original palera1n over Achilles for jailbreaking purposes.

## Disclaimer
Any damage caused to your device is your own responsibility. I am not responsible for any damage caused to your device by using this software, and by using this software you agree to this and are running the acute risk of damaging your device. Please be careful. I have so far not had any issues with my device, but that does not mean that you will not - however, due to the nature of the checkm8 exploit, it is very unlikely that you will damage your device in a way that cannot be fixed by a DFU restore.

Additionally, this project is not affiliated with any other projects listed at the bottom of this page, or in `other/licenses` - including (but not limited to) checkra1n, palera1n, gaster, ipwndfu, openra1n and ra1npoc. Achilles is a completely seperate project, and any issues encountered with it should be reported in this repository, not the to developers of the credited projects.

Finally, you may find that there are similarities in here to other projects like this - and that is true. This was just a personal project for me to learn more about the BootROM and iOS bootchain. As a result, I relied a lot on open-source solutions such as [ipwndfu](https://github.com/axi0mX/ipwndfu), [gaster](https://github.com/0x7ff/gaster) and [PongoOS](https://github.com/checkra1n/PongoOS) to help me along the way. Check the [credits](#credits) section for more information.

## What is planned for the future?
At the moment, Achilles can boot PongoOS, and is able to patch signature checks on checkm8 devices using the gaster payloads, but I haven't tested all devices. However, in the future, I plan to gear the project more towards a checkra1n-like experience. This includes booting a pre-boot execution environment (such as PongoOS), and jailbreaking the device. However, the option for regular checkm8 exploitation will always be available.

First of all, I want to write my own jailbreaking tool off of the back of Achilles. This will likely be a developer-oriented rootless jailbreak, as palera1n should suffice for most users. Doing so will involve writing a custom program that performs a similar job to [jbinit](https://github.com/palera1n/jbinit) as well as a loader application and package installation.

As part of the general plan for Achilles in the future, I plan on creating an alternative to PongoOS. While this is a difficult task, and one that I certainly do not have the knowledge to take up at the moment, I hope that I can learn enough to be able to do this in the future. Thanks to the brilliant efforts of the checkra1n team and their open-source PongoOS, I have a great starting point to learn from and hopefully something that can aid me in this complex project.

Furthermore, I am looking to take a shot at SEP exploitation with blackbird, which will allow for downgrades and dualboots on checkm8 devices, as well as bypassing the SEP mitigations seen in recent major iOS versions.

For improvements planned for this project, check the [to-do list](TODO.md) to see what is in the works and what I plan to work on next.

## Known issues
Entering recovery mode programmatically can be rather hit or miss, so if you device continuously fails to enter recovery mode, enter it manually and then run Achilles. This is a known issue and I am working on a fix for it, however the error seems to be related to libimobiledevice, so may be difficult to debug.

There also seems to be an issue with the libusb build of Achilles, when booting PongoOS, where a segmentation fault will occasionally occur when the USB handle is opened again after the device enters download mode. I have not figured out how to deterministically trigger this issue, and as such cannot fix it at this time. As a work-around, use the IOKit build or just simple place the device back into DFU from download mode and run the program again.

## Dependencies
* [libimobiledevice](https://github.com/libimobiledevice/libimobiledevice)
* gobjcopy

The easiest ways to install these dependencies on macOS are as follows:
* `brew install libimobiledevice`

* `brew install binutils`
* `ln -s /path/to/homebrew/opt/binutils/bin/gobjcopy /usr/local/bin/gobjcopy`

You have to symlink `gobjcopy` to `/usr/local/bin` as the binutils installation is not symlinked by default (as it can cause conflicts with built-in binaries from Apple).

## Documentation
I have made an effort to document the code as much as possible, and in all header files you will find a description of a function's purpose, parameters and return value for each function exported in the header. Hopefully this can help anyone who wants to learn more about the BootROM and iOS bootchain understand what is going on a bit better.

Furthermore, I have included a detailed writeup on the vulnerabilities exploited in the checkm8 exploit, including the not very well documented memory leak and subsequent exploitation of it. I tried my best to provide definitions where possible and explain the process as clearly as I could in the effort to make it simple for anyone with a basic understanding of vulnerability exploitation to understand. Head over to the [docs](docs/) directory to check out the writeup.

If you find any issues with the documentation, please open an issue or submit a pull request to help improve it.

## Supported devices
The regular gaster exploit should support A7-11 as well as T2, and booting PongoOS should work on A8-11 - with that, I have confirmed that the following devices work on my setup (M1 MacBook Air + IOKit):
* iPhone X (A11) - regular exploit, PongoOS
* iPad Pro 10.5" (A10X) - regular exploit, PongoOS
* iPad Pro 12.9" (A10X) - regular exploit, PongoOS
* iPad Pro 9.7" (A9X) - regular exploit, PongoOS
* iPhone 6 (A8) - regular exploit, PongoOS

SoCs that still need testing are as follows:
* T2 (Macs) - regular exploit
* A10 (iPhone 7 (Plus), iPad 6) - regular exploit, PongoOS
* A9 (iPhone 6S (Plus), iPhone SE, iPad 5) - regular exploit, PongoOS
* A8X (iPad Air 2) - regular exploit, PongoOS
* A7 (iPhone 5S, iPad Air, iPad Mini 2/3) - regular exploit

If you want to test with your device, please do so and then submit an issue or pull request with your results. Make sure to include information about your host machine (OS version, model, USB backend), device (model, SoC) and iOS version (only necessary if having issues with PongoOS).

If anyone else has successes or failures with other devices, please let me know so I can update this list and/or try to fix any issues you may experience.

## Credits
The following projects, people and their open-source work has been instrumental in the development of this project:
* Axi0mX - checkm8, [ipwndfu](https://github.com/axi0mX/ipwndfu)
* checkra1n team - [checkra1n](https://checkra.in/), YoloDFU payloads
* palera1n team - [palera1n](https://palera.in/)
* 0x7FF - [gaster](https://github.com/0x7ff/gaster)
* kok3shidoll - [ra1npoc](https://github.com/kok3shidoll/ra1npoc)
* Mineek - [openra1n](https://github.com/mineek/openra1n)

All licenses for any code used in this project that is not my own can be found in the [other/licenses](other/licenses) directory.
