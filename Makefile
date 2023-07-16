CC=gcc
SOURCES=src/main.c src/exploit/*.c src/usb/*.c src/utils/*.c src/exploit/payloads/*.c src/boot/pongo/*.c src/boot/lz4/*.c
FRAMEWORKS=-framework IOKit -framework CoreFoundation -limobiledevice-1.0.6
CFLAGS=-Iinclude -Wunused -Wunused-variable
OUTPUT=build/AlfieLoader

all: dirs AlfieLoader

dirs:
	@mkdir -p build

# TODO: Add `make docs` to generate PDFs from single Makefile
	
clean:
	@rm -rf build

AlfieLoader: $(SOURCES)
	@$(CC) $(FRAMEWORKS) $(CFLAGS) -o $(OUTPUT) $(SOURCES)