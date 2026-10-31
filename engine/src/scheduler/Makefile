CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS ?= -Iinclude -I../job_manager/include
LDLIBS ?= -lm

.PHONY: all clean test
all: cpu_scheduler

libcampuscompute_scheduler.so: src/scheduler.c include/scheduler.h ../job_manager/src/job_manager.c ../job_manager/include/job_manager.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared -o $@ src/scheduler.c ../job_manager/src/job_manager.c $(LDLIBS)

cpu_scheduler: src/main.c src/scheduler.c include/scheduler.h ../job_manager/src/job_manager.c ../job_manager/include/job_manager.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/main.c src/scheduler.c ../job_manager/src/job_manager.c $(LDLIBS)

tests/test_scheduler: tests/test_scheduler.c src/scheduler.c include/scheduler.h ../job_manager/src/job_manager.c ../job_manager/include/job_manager.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_scheduler.c src/scheduler.c ../job_manager/src/job_manager.c $(LDLIBS)

test: tests/test_scheduler cpu_scheduler
	./tests/test_scheduler
	@printf '1\n2\nP1\n0\n5\n2\nP2\n1\n3\n1\n3\n1\n5\n0\n' | ./cpu_scheduler >/dev/null
	@echo "FR2 CLI smoke test passed."

clean:
	rm -f cpu_scheduler libcampuscompute_scheduler.so tests/test_scheduler
