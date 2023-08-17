.PHONY: all dirs payloads libusb Achilles clean
CC=gcc
SOURCES=src/main.c src/exploit/*.c src/usb/*.c src/utils/*.c src/exploit/payloads/*.c src/boot/pongo/*.c src/boot/lz4/*.c
FRAMEWORKS=-framework IOKit -framework CoreFoundation -limobiledevice-1.0
OUTPUT=build/Achilles
CFLAGS=-Iinclude -Wunused

all: dirs payloads Achilles

dirs:
	@mkdir -p build

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
	@$(RM) -r src/boot/payloads/checkra1n/*.h
	
clean:
	@rm -rf build

libusb:
	@cd . && make payloads
	@$(CC) $(FRAMEWORKS) $(CFLAGS) -lusb-1.0 -DACHILLES_LIBUSB -o $(OUTPUT) $(SOURCES)
	@$(RM) -r include/exploit/payloads/gaster
	@$(RM) -r include/boot/payloads

Achilles: $(SOURCES)
	@$(CC) $(FRAMEWORKS) $(CFLAGS) -o $(OUTPUT) $(SOURCES)
	@$(RM) -r include/exploit/payloads/gaster
	@$(RM) -r include/boot/payloads