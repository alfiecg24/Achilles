# Licenses

This directory contains the licenses for the third-party code used in this project. While there may be very little code from certain projects, there are others that have been used extensively, and as such I have included the licenses for all of them. I have tried to outline which project provided which code below, but if you find any issues with this, please let me know, as I cannot pinpoint the source of all code.

## gaster
I have used code from [gaster](https://github.com/0x7FF/gaster) extensively. The USB code from the project was implemented into Achilles to save me hours of time writing my own system, when gaster had an almost-perfect implementation of such a system.

Gaster also provided exploit assembly code for the checkm8 exploit, which you can find in the [src/exploit/payloads/gaster](../../src/exploit/payloads/gaster) directory. The checkm8 implementation from the project also heavily influenced the implementation in Achilles, and I have used the same method of exploitation as gaster. However, gaster based it's implementation off of the ipwndfu implementation - but was still extremely helpful regardless.

## ipwndfu
[ipwndfu](https://github.com/axi0mX/ipwndfu) contains the original checkm8 exploit implementation, and was extremely influential in helping me understand the exploit. There is, as it stands, very little (if any) code from ipwndfu in Achilles, but I have included the license for it anyway.

## palera1n
[palera1n](https://github.com/palera1n/palera1n) has provided utilties such as the logging system used in Achilles, the code for interacting with PongoOS, and other small features (such as putting the device in recovery mode) and also their DFU helper, which mine is heavily based off of.

Additionally, I have included the palera1n kernel patchfinder binary, the ramdisk and the overlay (binpack) used in the jailbreak in order to allow for jailbreaking with palera1n through Achilles. These files can be found in the [src/kernel/patchfinder](../../src/kernel/patchfinder/) and [src/userland](../../src/userland/)  directories.

## PongoOS
[PongoOS](https://github.com/checkra1n/PongoOS) and the checkra1n jailbreak has provided the YoloDFU payloads used in Achilles, as well as the PongoOS binary used to boot the device. The PongoOS binary and the YoloDFU payloads can be found in the [src/boot/payloads/checkra1n](../../src/boot/payloads/checkra1n) directory.

## LZ4
The [LZ4](https://github.com/lz4/lz4) compression algorithm is used to compress PongoOS before sending it to the device (we actually use LZ4HC as it has higher compression than the regular LZ4 algorithm).