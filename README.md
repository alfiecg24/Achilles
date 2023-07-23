# AlfieLoader

[![Time spent on AlfieLoader](https://wakatime.com/badge/user/61592169-b9cf-4af8-b6fa-8ac7d4369b01/project/c122878a-9ce8-40b1-a337-e53a7ef5f337.svg)](https://wakatime.com/badge/user/61592169-b9cf-4af8-b6fa-8ac7d4369b01/project/c122878a-9ce8-40b1-a337-e53a7ef5f337)

## What is AlfieLoader?
AlfieLoader is a project that aims to provide a easy way to harness the power of the checkm8 exploit. It has three prominent features:
* Exploitation using gaster payloads to patch signature checks
* Booting PongoOS as a drop-in replacement for checkra1n 1337
* Jailbreaking rootless with palera1n

This project is nowhere near the maturity of the original versions of these projects, but was only created as a personal project to learn more about the BootROM and iOS bootchain. As such, I am in no way denouncing the above projects, and would in fact recommend using them over AlfieLoader at the moment, as they are much more mature and stable.

## Why is it called AlfieLoader?
I wanted to get started with this project as soon as possible in the beginning, and did not want to spend hours contemplating different names. I subsequently decided to name it AlfieLoader, as my name is Alfie. While it may change in the future, it will remain as is until further notice.

## What does it do?
In its current state, this project does the following things:
* Runs the checkm8 exploit A7-A11 + T2 with gaster payloads
* Boots PongoOS on SoCs S8000-T8015 as a drop-in replacement for checkra1n 0.1337.X
* Allows for jailbreaking rootless with palera1n on supported devices

You can check the [supported devices](#supported-devices) section to see which devices confirmed to work and which ones still need testing.

## What are the current limitations?
* Only planned support is for A7-A11 devices
* A12+ devices cannot be supported unless a BootROM vulnerability is found and publicised

## What is planned for the future?
At the moment, AlfieLoader can boot PongoOS, and is able to patch signature checks on checkm8 devices using the gaster payloads, but I haven't tested all devices. However, in the future, I plan to gear the project more towards a checkra1n-like experience. This includes booting a pre-boot execution environment (such as PongoOS), and jailbreaking the device. However, the option for regular checkm8 exploitation will always be available.

As part of this, I plan on creating an alternative to PongoOS. While this is a difficult task, and one that I certainly do not have the knowledge to take up at the moment, I hope that I can learn enough to be able to do this in the future. Thanks to the brilliant efforts of the checkra1n team and their open-source PongoOS, I have a great starting point to learn from and hopefully something that can aid me in this complex project.

This will grant the ability to create a jailbreak for checkm8 devices on the latest versions of iOS, which will definitely be an extremely interesting project indeed, and is something I plan to pursue. Furthermore, I am looking to take a shot at SEP exploitation with blackbird, which will allow for downgrades and dualboots on checkm8 devices, as well as bypassing the SEP mitigations seen in recent major iOS versions.

## Dependencies
* [libimobiledevice](https://github.com/libimobiledevice/libimobiledevice)

You will need to specify the version of your libimobiledevice installation in the `Makefile`, in the `FRAMEWORKS` arguments. The default version is `1.0.6`. This will hopefully be resolved in the future so that the project can be built by simply running `make`.

## Documentation
I have made an effort to document the code as much as possible, and in all header files you will find a description of a function's purpose, parameters and return value for each function exported in the header. Hopefully this can help anyone who wants to learn more about the BootROM and iOS bootchain understand what is going on a bit better.

Furthermore, I have included a detailed writeup on the vulnerabilities exploited in the checkm8 exploit, including the not very well documented memory leak and subsequent exploitation of it. I tried my best to provide definitions where possible and explain the process as clearly as I could in the effort to make it simple for anyone with a basic understanding of vulnerability exploitation to understand. Head over to the [docs](docs/) directory to check out the writeup.

If you find any issues with the documentation, please open an issue or submit a pull request to help improve it.

## Supported devices
The regular gaster exploit should support A7-11 as well as T2, and booting PongoOS should work on A8-11 - with that, I have confirmed that the following devices work on my setup (M1 MacBook Air + IOKit):
* iPhone X (A11) - regular, PongoOS
* iPad Pro 10.5" (A10X) - regular, PongoOS
* iPad Pro 12.9" (A10X) - regular, PongoOS
* iPad Pro 9.7" (A9X) - regular, PongoOS
* iPhone 6 (A8) - regular, PongoOS

SoCs that still need testing are as follows:
* T2 (Macs) - regular
* A10 (iPhone 7 (Plus), iPad 6) - regular, PongoOS
* A9 (iPhone 6S (Plus), iPhone SE, iPad 5) - regular, PongoOS
* A8X (iPad Air 2) - regular, PongoOS
* A7 (iPhone 5S, iPad Air, iPad Mini 2/3) - regular

If you want to test with your device, please do so and them submit an issue or pull request with your results. Make sure to include information about your host machine (OS version, model), device (model, SoC) and iOS version (only necessary if having issues with PongoOS).

If anyone else has successes or failures with other devices, please let me know so I can update this list and/or fix any issues you may experience.
## Disclaimer
You may find that there are similarities in here to other projects like this - and that is true. This was just a personal project for me to learn more about the BootROM and iOS bootchain. As a result, I relied a lot on open-source solutions such as [ipwndfu](https://github.com/axi0mX/ipwndfu), [gaster](https://github.com/0x7ff/gaster) and [PongoOS](https://github.com/checkra1n/PongoOS) to help me along the way.

Any damage caused to your device is your own responsibility. I am not responsible for any damage caused to your device by using this software, and by using this software you agree to this and are running the acute risk of damaging your device. Please be careful. I have so far not had any issues with my device, but that does not mean that you will not - however, due to the nature of the checkm8 exploit, it is very unlikely that you will damage your device in a way that cannot be fixed by a DFU restore.

## Credits
* Axi0mX - checkm8, [ipwndfu](https://github.com/axi0mX/ipwndfu)
* checkra1n team - [checkra1n](https://checkra.in/), YoloDFU payloads
* palera1n team - [palera1n](https://palera.in/)
* 0x7FF - [gaster](https://github.com/0x7ff/gaster)
* kok3shidoll - [ra1npoc](https://github.com/kok3shidoll/ra1npoc)
* Mineek - [openra1n](https://github.com/mineek/openra1n)

All licenses for any code used in this project that is not my own can be found in the [other/licenses](other/licenses) directory.