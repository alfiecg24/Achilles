.PHONY: all
CC=gcc
SOURCES=$(wildcard src/*.c) $(wildcard src/usb/*.c) $(wildcard src/utils/*.c) $(wildcard src/exploit/*.c) $(wildcard src/pongo/*.c) $(wildcard src/pongo/lz4/*.c)
OUTPUT=build/Achilles
LIBS=-limobiledevice-1.0 -lusb-1.0 -lcrypto
CFLAGS=-Iinclude -Wall

all: Achilles

Achilles: $(SOURCES)
	@mkdir -p build
	$(CC) $(SOURCES) $(LIBS) $(CFLAGS) -o $(OUTPUT)

install: Achilles
	@mkdir -p /usr/local/bin
	@sudo cp $(OUTPUT) /usr/local/bin/Achilles

clean:
	@rm -rf build