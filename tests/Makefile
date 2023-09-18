.PHONY: all clean
CC=gcc
SOURCES=main.c log/log.c
OUTPUT=build/Achilles-tests

all: dirs tests

dirs:
	@mkdir -p build
	
clean:
	@rm -rf build

tests: $(SOURCES)
	@$(CC) -o $(OUTPUT) $(SOURCES)