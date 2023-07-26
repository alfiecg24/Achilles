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
	@cd src/exploit/payloads/gaster && make
	@cd ../../../../
	
clean:
	@rm -rf build

libusb:
	@cd src/exploit/payloads/gaster && make
	@cd ../../../../
	@$(CC) $(FRAMEWORKS) $(CFLAGS) -lusb-1.0 -DACHILLES_LIBUSB -o $(OUTPUT) $(SOURCES)

Achilles: $(SOURCES)
	@$(CC) $(FRAMEWORKS) $(CFLAGS) -o $(OUTPUT) $(SOURCES)