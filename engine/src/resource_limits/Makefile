CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS = -Iinclude

TARGET = resource_limits_demo
TEST_TARGET = tests/test_resource_limits

SRC = src/resource_limits.c
TEST_SRC = tests/test_resource_limits.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SRC) src/main.c include/resource_limits.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/main.c $(SRC)

$(TEST_TARGET): $(TEST_SRC) $(SRC) include/resource_limits.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(TEST_SRC) $(SRC)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET)
