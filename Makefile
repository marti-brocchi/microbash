CC = gcc
CFLAGS = -std=gnu11 -Wall -pedantic -Werror -fsanitize=address -ggdb -Og

BIN_DIR = bin

LDFLAGS = -fsanitize=address -lreadline
MICROBASH = $(BIN_DIR)/microbash
BASH_OBJS = $(BIN_DIR)/bash.o
MAIN_OBJS = $(BIN_DIR)/main.o

EXECS = $(MICROBASH)

all: $(EXECS)

.PHONY: clean tgz

# Microbash
$(MICROBASH): $(BASH_OBJS) $(MAIN_OBJS) | $(BIN_DIR)
	$(CC) -o $@ $(BASH_OBJS) $(MAIN_OBJS) $(LDFLAGS)
$(BIN_DIR)/bash.o: microbash/microbash.h microbash/bash.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ microbash/bash.c
$(BIN_DIR)/main.o: microbash/microbash.h microbash/main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ microbash/main.c

# Directories
$(BIN_DIR):
	mkdir $(BIN_DIR)

# Utilities
clean:
	rm -rf $(EXECS) $(BIN_DIR)/*.o
tgz: clean
	cd ..; tar cvzf microbash.tgz microbash

