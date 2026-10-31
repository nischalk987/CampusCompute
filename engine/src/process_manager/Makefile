CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS ?= -Iinclude

.PHONY: all clean test

all: worker_failure_demo tests/test_process_manager

worker_failure_demo: src/main.c src/process_manager.c include/process_manager.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/main.c src/process_manager.c

tests/test_process_manager: tests/test_process_manager.c src/process_manager.c include/process_manager.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_process_manager.c src/process_manager.c

test: tests/test_process_manager worker_failure_demo
	./tests/test_process_manager
	./worker_failure_demo normal
	./worker_failure_demo failure

clean:
	rm -f worker_failure_demo tests/test_process_manager
