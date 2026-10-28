CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS ?= -Iinclude
.PHONY: all clean test
all: banker
banker: src/main.c src/banker.c include/banker.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/main.c src/banker.c
tests/test_banker: tests/test_banker.c src/banker.c include/banker.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_banker.c src/banker.c
test: tests/test_banker banker
	./tests/test_banker
	@printf '1\n1\n1\n0\n0\n1\n0\n0\n0\n' | ./banker >/dev/null
	@echo "FR5 CLI smoke test passed."
clean:
	rm -f banker tests/test_banker
