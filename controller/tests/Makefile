# Controller unit tests — master entrypoint is test.c
CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -pedantic
CPPFLAGS := -I../include
LDFLAGS := -lm

SRC_AHRS := ../src/ahrs.c
OBJS := ahrs.o test_runner.o test_ahrs_calibration.o test.o
BIN := run_tests

.PHONY: all check clean

all: $(BIN)

ahrs.o: $(SRC_AHRS) ../include/ahrs.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(SRC_AHRS) -o $@

test_runner.o: test_runner.c test_runner.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c test_runner.c -o $@

test_ahrs_calibration.o: test_ahrs_calibration.c ../include/ahrs.h test_runner.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c test_ahrs_calibration.c -o $@

test.o: test.c test_runner.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c test.c -o $@

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

check: $(BIN)
	./$(BIN)

clean:
	rm -f $(OBJS) $(BIN)
