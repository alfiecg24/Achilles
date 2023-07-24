# Userland

This directory contains the source code for the userland programs that are used in AlfieLoader. These programs are responsible for the following things:
* Bootstrap the iOS environment
* Deploy the jailbreak
* Set up the jailbreak environment

The `binpack` directory holds the `binpack.dmg` overlay, which drops into the filesystem the necessary files for the jailbreak - an example being the files necessary to initiate SSH on the device, as well as shell binaries (`ls`, `chmod` etc.). The `ramdisk` directory holds the `ramdisk.dmg` ramdisk, which (in the case of palera1n) executes the `jbinit` function to enable system hooks, setup the jailbreak environment and allow the jailbreak to function.

In the future, I plan to use my own ramdisk and overlay to deploy a custom jailbreak that is fully open-source and works alongside AlfieLoader.