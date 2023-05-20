CC=gcc
SOURCES=src/main.c src/exploit/*.c src/usb/*.c src/utils/*.c
FRAMEWORKS=-framework IOKit -framework CoreFoundation -limobiledevice-1.0.6
CFLAGS=-Iinclude
OUTPUT=build/AlfieLoader

all: dirs AlfieLoader

dirs:
	@mkdir -p build

clean:
	@rm -rf build

AlfieLoader: $(SOURCES)
	@$(CC) $(FRAMEWORKS) $(CFLAGS) -o $(OUTPUT) $(SOURCES)