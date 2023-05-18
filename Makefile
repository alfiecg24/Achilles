CC=gcc
SOURCES=src/main.c src/exploit/exploit.c src/exploit/dfu.c src/utils/log.c src/usb/usb-utils.c
FRAMEWORKS=-framework IOKit -framework CoreFoundation
# Search for header files in include/
CFLAGS=-Iinclude

all: exploit

exploit: $(SOURCES)
	@mkdir -p build
	@$(CC) $(FRAMEWORKS) $(CFLAGS) -o build/AlfieLoader $(SOURCES)