CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -g -std=c2x

# Define source files
MAIN_SRC = src/main.c src/gbix.c
SETUP_SRC = src/setup.c

# Define output binaries
MAIN_BIN = build/main
SETUP_BIN = build/setup

all: $(MAIN_BIN) $(SETUP_BIN)

# Build main executable with gbix
$(MAIN_BIN): $(MAIN_SRC)
	@mkdir -p build
	@$(CC) $(CFLAGS) -o $@ $^ -lm

# Build setup executable separately
$(SETUP_BIN): $(SETUP_SRC)
	@mkdir -p build
	@$(CC) $(CFLAGS) -o $@ $<

.PHONY: all
