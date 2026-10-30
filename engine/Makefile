CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2

CPPFLAGS += -Isrc/job_manager/include
CPPFLAGS += -Isrc/scheduler/include
CPPFLAGS += -Isrc/banker/include
CPPFLAGS += -Isrc/process_manager/include
CPPFLAGS += -Isrc/resource_limits/include

LDLIBS += -pthread -lrt -lm

TARGET = campuscompute

CORE_SOURCES = \
	src/job_manager/src/job_manager.c \
	src/scheduler/src/scheduler.c \
	src/banker/src/banker.c \
	src/process_manager/src/process_manager.c \
	src/resource_limits/src/resource_limits.c

MODULE_DIRS = \
	src/job_manager \
	src/scheduler \
	src/producer_consumer \
	src/shared_memory \
	src/banker \
	src/process_manager \
	src/resource_limits

.PHONY: all modules test clean

all: modules $(TARGET)

modules:
	@set -e; \
	for dir in $(MODULE_DIRS); do \
		$(MAKE) -C $$dir all; \
	done

$(TARGET): src/main.c $(CORE_SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDLIBS)

test: all
	@set -e; \
	for dir in $(MODULE_DIRS); do \
		$(MAKE) -C $$dir test; \
	done
	bash tests/test_cli.sh
	@echo "All CampusCompute tests passed."

clean:
	rm -f $(TARGET)
	@set -e; \
	for dir in $(MODULE_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
