CC=gcc
SOURCES=src/main.c src/exploit/exploit.c src/exploit/dfu.c src/utils/log.c src/usb/usb.c
FRAMEWORKS=-framework IOKit -framework CoreFoundation
CFLAGS=-Iinclude
OUTPUT=build/AlfieLoader

all: dirs AlfieLoader

dirs:
	@mkdir -p build

clean:
	@rm -rf build

AlfieLoader: $(SOURCES)
	@$(CC) $(FRAMEWORKS) $(CFLAGS) -o $(OUTPUT) $(SOURCES)