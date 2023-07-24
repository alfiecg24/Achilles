# Kernel

The kernel is the core of the operating system. It is responsible for managing the hardware and providing an interface for applications to use. This directory will contain the code for the kernel patchfinder used to perform the necessary patches on-the-fly.

The patchfinder patches out various checks and mitigations in the kernel. These include:
* Kernel ASLR
* KTRR (kernel text read-only region)
* KPP (kernel patch protection)
* AMFI and code signing

Without the patching process, jailbreaking would not be possible - as any unsigned code that attempted to be run would immediately be killed by code-signing mechanisms.