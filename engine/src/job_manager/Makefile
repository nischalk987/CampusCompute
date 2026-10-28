CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS ?= -Iinclude
.PHONY: all clean test
all: job_manager
job_manager: src/main.c src/job_manager.c include/job_manager.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/main.c src/job_manager.c
tests/test_job_manager: tests/test_job_manager.c src/job_manager.c include/job_manager.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_job_manager.c src/job_manager.c
test: tests/test_job_manager job_manager
	./tests/test_job_manager
	@printf '1\nalpha\n0\n3\n2\n0\n5\nalpha\n3\nalpha\n4\nalpha\n5\nalpha\n5\n0\n' | ./job_manager >/dev/null
	@echo "FR1 CLI smoke test passed."
clean:
	rm -f job_manager tests/test_job_manager
