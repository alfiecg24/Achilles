# Userland

This directory contains the source code for the userland programs that are used in AlfieLoader. These programs are responsible for the following things:
* Bootstrap the iOS environment
* Deploy the AlfieLoader jailbreak
* Set up the jailbreak environment

The `loader` directory contains the code for the loader application which initiates the "jailbreak" process and performs the bootstrap. The `basebin` directory contains necessary binaries for the device, such as the launchd hook and `jailbreakd`.