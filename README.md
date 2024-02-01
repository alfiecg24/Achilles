# Achilles

Achilles is a checkm8 utility for macOS and Linux that offers a selection of tools for vulnerable devices.

If you want to read more about the checkm8 exploit, check out my [blog post](https://alfiecg.uk/2023/07/21/A-comprehensive-write-up-of-the-checkm8-BootROM-exploit.html).

Linux support is currently experimental - failing to send the overwrite (and thus exploit failure) is a common issue that is yet to be resolved.

# Features
* Patch signature checks with gaster payloads
* Boot PongoOS on supported devices
* Jailbreak with supplied kernel patchfinder, ramdisk and overlay

# Tested devices
* iPhone 7, A10
* iPhone X, A11
* iPad Pro 10.5", A10X

# Usage
```
➜  ~ Achilles -h  
Options:
        -d - enable debug logging
        -v - enable verbose logging
        -q - enable quiet logging (removes all logging except for errors)
        -s - remove signature checks
        -p - boot to PongoOS and exit
        -j - jailbreak the device (requires -K)
        -V - enable verbose boot
        -S - enable serial output

        -u <UDID> - specify a device UDID
        -b <arguments> - additional boot arguments
        -k <Pongo.bin> - override PongoOS image
        -K <kpf> - override kernel patchfinder
        -R <ramdisk.dmg> - ramdisk to boot
        -O <overlay.dmg> - overlay to boot

        -h - print this help message
Examples:
        Achilles -p
        Achilles -j -K kpf -R ramdisk.dmg -O overlay.dmg
        Achilles -s
```

# Building
Achilles requires the following dependencies:
* libimobiledevice
* libusb

To build, run `make` in the root directory. This will output the final product to `build/Achilles`.

To install to `/usr/local/bin`, run `make install` (you will be prompted for your password).

# Credits
* [checkra1n](https://checkra.in) - YoloDFU payloads and PongoOS ([license](https://github.com/checkra1n/PongoOS/tree/master/LICENSE.md))
* [0x7ff](https://github.com/0x7FF) - gaster ([license](https://github.com/0x7ff/gaster/tree/main/LICENSE))
* [Mineek](https://github.com/Mineek) - openra1n (unlicensed)
* [axi0mX](https://github.com/axi0mX) - checkm8 exploit ([license](https://github.com/axi0mX/ipwndfu/tree/master/LICENSE))
* [palera1n](https://palera.in) - DFU helper, libimobiledevice code ([license](https://github.com/palera1n/palera1n/tree/main/LICENSE))
