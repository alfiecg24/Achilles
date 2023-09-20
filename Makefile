.PHONY: all clean tests
CC=gcc
SOURCES=src/main.c src/exploit/*.c src/usb/*.c src/utils/*.c src/exploit/payloads/*.c src/boot/pongo/*.c src/boot/lz4/*.c
TESTS_MAIN=tests/main.c
FRAMEWORKS=-framework IOKit -framework CoreFoundation -limobiledevice-1.0
OUTPUT=build/Achilles
TEST_OUTPUT=tests/build/Achilles-tests
CFLAGS=-Iinclude -Wunused
TEST_FLAGS=-DTESTS -e_tests
DEBUG=

all: dirs pongo payloads Achilles

dirs:
	@echo "Creating directories"
	@mkdir -p build

pongo:
	@echo "Building PongoOS"
	@cd src/PongoOS && make

payloads:
	@echo "Building payloads"
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
	@make dirs
	@make pongo
	@make payloads
	@echo "Building Achilles for libusb"
	@$(CC) $(FRAMEWORKS) $(CFLAGS) $(DEBUG) -lusb-1.0 -DACHILLES_LIBUSB -o $(OUTPUT) $(SOURCES)

tests:
	@mkdir -p tests/build
	@make payloads
	@echo "Building Achilles tests for IOKit"
	@$(CC) $(FRAMEWORKS) $(CFLAGS) $(DEBUG) $(TEST_FLAGS) -o $(TEST_OUTPUT)-IOKit $(TESTS_MAIN) $(SOURCES)
	@echo "Building Achilles tests for libusb"
	@$(CC) $(FRAMEWORKS) $(CFLAGS) $(DEBUG) $(TEST_FLAGS) -lusb-1.0 -DACHILLES_LIBUSB -o $(TEST_OUTPUT)-libusb $(TESTS_MAIN) $(SOURCES)

Achilles: $(SOURCES)
	@echo "Building Achilles for IOKit"
	@make payloads
	@$(CC) $(FRAMEWORKS) $(CFLAGS) $(DEBUG) -o $(OUTPUT) $(SOURCES)