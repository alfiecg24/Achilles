.PHONY: all dirs pongo payloads libusb Achilles clean
CC=gcc
SOURCES=src/main.c src/exploit/*.c src/usb/*.c src/utils/*.c src/exploit/payloads/*.c src/boot/pongo/*.c src/boot/lz4/*.c
FRAMEWORKS=-framework IOKit -framework CoreFoundation -limobiledevice-1.0
OUTPUT=build/Achilles
CFLAGS=-Iinclude -Wunused

all: dirs pongo payloads Achilles

dirs:
	@mkdir -p build

pongo:
	@cd src/PongoOS && make clean && make

payloads:
	@mkdir -p include/exploit/payloads/gaster/headers
	@cd src/exploit/payloads/gaster && make
	@cd ../../../../
	@cp -R src/exploit/payloads/gaster/headers/* include/exploit/payloads/gaster/headers
	@$(RM) -r src/exploit/payloads/gaster/headers
	@mkdir -p include/boot/payloads/checkra1n/headers
	@cd src/boot/payloads/checkra1n && make
	@cd ../../../../
	@cp -R src/boot/payloads/checkra1n/*.h include/boot/payloads/checkra1n/headers
	@$(RM) src/boot/payloads/checkra1n/*.h
	@cd src/userland/jbinit && make
	@cd ../../../
	@mkdir -p include/userland/jbinit
	@cp src/userland/jbinit/binpack.h include/userland/jbinit/binpack.h
	@cp src/userland/jbinit/ramdisk.h include/userland/jbinit/ramdisk.h
	@$(RM) -r src/userland/jbinit/binpack.h
	@$(RM) -r src/userland/jbinit/ramdisk.h
	@mkdir -p include/boot/pongo/headers
	@cd src/boot/pongo && xxd -iC Pongo-palera1n.bin > Pongo-palera1n.h
	@cp src/boot/pongo/Pongo-palera1n.h include/boot/pongo/headers/Pongo-palera1n.h
	@$(RM) src/boot/pongo/Pongo-palera1n.h
	@mkdir -p include/kernel/patchfinder
	@cd src/PongoOS/build && xxd -iC checkra1n-kpf-pongo > kpf.h
	@cp src/PongoOS/build/kpf.h include/kernel/patchfinder/kpf.h
	@$(RM) src/PongoOS/build/kpf.h
	@mkdir -p include/kernel/patchfinder
	@cd src/kernel/patchfinder && xxd -iC kpf-palera1n > kpf-palera1n.h
	@cp src/kernel/patchfinder/kpf-palera1n.h include/kernel/patchfinder/kpf-palera1n.h
	@$(RM) src/kernel/patchfinder/kpf-palera1n.h

	
clean:
	@rm -rf build
	@cd src/PongoOS && make clean

libusb:
	@cd . && make payloads
	@cd . && make pongo
	@$(CC) $(FRAMEWORKS) $(CFLAGS) -lusb-1.0 -DACHILLES_LIBUSB -o $(OUTPUT) $(SOURCES)
	@$(RM) -r include/exploit/payloads/gaster
	@$(RM) -r include/boot/payloads
	@$(RM) -r include/kernel/patchfinder
	@$(RM) -r include/userland/jbinit
	@$(RM) -r include/boot/pongo/headers

Achilles: $(SOURCES)
	@$(CC) $(FRAMEWORKS) $(CFLAGS) -o $(OUTPUT) $(SOURCES)
	@$(RM) -r include/exploit/payloads/gaster
	@$(RM) -r include/boot/payloads
	@$(RM) -r include/kernel/patchfinder
	@$(RM) -r include/userland/jbinit
	@$(RM) -r include/boot/pongo/headers