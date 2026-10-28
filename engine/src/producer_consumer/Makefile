CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
PTHREAD_FLAGS ?= -pthread

.PHONY: all clean test

all: producer_consumer

producer_consumer: producer_consumer.c
	$(CC) $(CFLAGS) $(PTHREAD_FLAGS) -o $@ $<

clean:
	rm -f producer_consumer tests/activity-test.log

test: producer_consumer
	@mkdir -p tests
	@set -eu; \
	  ./producer_consumer --help >/dev/null; \
	  ./producer_consumer 25 tests/activity-test.log > /dev/null; \
	  test "$$(wc -l < tests/activity-test.log)" -eq 26; \
	  grep -q '^timestamp[[:space:]]sequence[[:space:]]message$$' tests/activity-test.log; \
	  if ./producer_consumer 0 >/dev/null 2>&1; then echo "FAIL: accepted zero events"; exit 1; fi; \
	  if ./producer_consumer nope >/dev/null 2>&1; then echo "FAIL: accepted non-numeric count"; exit 1; fi; \
	  if ./producer_consumer 100001 >/dev/null 2>&1; then echo "FAIL: accepted count above limit"; exit 1; fi; \
	  if ./producer_consumer -1 >/dev/null 2>&1; then echo "FAIL: accepted negative count"; exit 1; fi; \
	  echo "FR3 tests passed."
