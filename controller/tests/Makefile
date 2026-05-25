# Controller unit tests — master entrypoint is test.c
CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -pedantic
CPPFLAGS := -I../include
LDFLAGS := -lm

OBJS := ahrs.o ahrs_calibration.o utils.o test_runner.o test_ahrs_calibration.o \
        test_utils_math.o test.o
BIN := run_tests

.PHONY: all check clean

all: $(BIN)

ahrs.o: ../src/ahrs.c ../include/integrations/ahrs.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ../src/ahrs.c -o $@

ahrs_calibration.o: ../src/ahrs_calibration.c ../include/integrations/ahrs.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ../src/ahrs_calibration.c -o $@

utils.o: ../src/utils.c ../include/utils/math.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ../src/utils.c -o $@

test_runner.o: test_runner.c test_runner.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c test_runner.c -o $@

test_ahrs_calibration.o: test_ahrs_calibration.c ../include/integrations/ahrs.h test_runner.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c test_ahrs_calibration.c -o $@

test_utils_math.o: test_utils_math.c ../include/utils/math.h test_runner.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c test_utils_math.c -o $@

test.o: test.c test_runner.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c test.c -o $@

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

check: $(BIN)
	./$(BIN)

clean:
	rm -f $(OBJS) $(BIN)
