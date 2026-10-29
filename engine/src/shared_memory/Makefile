
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
LDLIBS = -pthread -lrt

.PHONY: all clean test

all: shared_memory

shared_memory: shared_memory.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f shared_memory

test: shared_memory
	./shared_memory --help
	bash tests/test_shared_memory.sh
	@echo "ALL FR4  test passed."
