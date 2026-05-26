# Controller — app (src/*.c, cmd/*.c) and unit tests share COMMS_BACKEND / flags.
CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -pedantic
CPPFLAGS := -Iinclude
LDFLAGS :=

COMMS_BACKEND ?= SIM
BUILD_DIR := build
COMMS_BUFFER_SIZE ?= 128
COMMS_SIM_DATA_DIR ?= simulated_data
SIM_SEED ?= 42
SIM_FRAMES ?= 10

# --- shared backend config (app + tests use the same CPPFLAGS) ---
CPPFLAGS += -DCOMMS_BUFFER_SIZE=$(COMMS_BUFFER_SIZE)
ifeq ($(COMMS_BACKEND),HARDWARE)
  CPPFLAGS += -DCOMMS_BACKEND=COMMS_BACKEND_HARDWARE
  I2C_DEVICE ?= /dev/i2c-1
  CPPFLAGS += -DI2C_DEVICE=\"$(I2C_DEVICE)\"
else ifeq ($(COMMS_BACKEND),SIM)
  CPPFLAGS += -DCOMMS_BACKEND=COMMS_BACKEND_SIM \
              -DCOMMS_SIM_DATA_DIR=\"$(COMMS_SIM_DATA_DIR)\"
else
  $(error COMMS_BACKEND must be HARDWARE or SIM)
endif

APP_SRCS := $(wildcard src/*.c) $(wildcard cmd/*.c)
APP_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRCS))
APP_BIN := $(BUILD_DIR)/segling-controller

TEST_LIB_SRCS := src/ahrs.c src/ahrs_calibration.c src/utils.c
TEST_SRCS := tests/test.c tests/test_runner.c tests/test_ahrs_calibration.c \
             tests/test_utils_math.c
TEST_LIB_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_LIB_SRCS))
TEST_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRCS))
TEST_BIN := $(BUILD_DIR)/run_tests

.PHONY: all test check clean generate-sim-data

all: $(APP_BIN)

test: $(TEST_BIN)

check: $(TEST_BIN)
	$(TEST_BIN)

generate-sim-data:
	uv run python scripts/generate_sim_data.py --seed $(SIM_SEED) --frames $(SIM_FRAMES) --out-dir $(COMMS_SIM_DATA_DIR)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(APP_BIN): $(APP_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(APP_OBJS) $(LDFLAGS)

$(TEST_BIN): $(TEST_LIB_OBJS) $(TEST_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(TEST_LIB_OBJS) $(TEST_OBJS) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
