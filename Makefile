.PHONY: all
CC=gcc
SOURCES=$(wildcard src/*.c) $(wildcard src/utils/*.c) $(wildcard src/exploit/*.c) $(wildcard src/usb/*.c) $(wildcard src/pongo/*.c) $(wildcard src/pongo/lz4/*.c)
OUTPUT=build/Achilles
CFLAGS=-Iinclude -Wunused -lusb-1.0 -limobiledevice-1.0

all: Achilles

Achilles: $(SOURCES)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCES)

install: Achilles
	@mkdir -p /usr/local/bin
	@cp $(OUTPUT) /usr/local/bin/Achilles

clean:
	@rm -rf build