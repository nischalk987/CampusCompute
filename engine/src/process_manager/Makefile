CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS ?= -Iinclude

.PHONY: all clean test

all: tests/test_process_manager

tests/test_process_manager: tests/test_process_manager.c src/process_manager.c include/process_manager.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_process_manager.c src/process_manager.c

test: tests/test_process_manager
	./tests/test_process_manager

clean:
	rm -f tests/test_process_manager
