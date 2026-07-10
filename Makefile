CC		:= gcc
CFLAGS	:= -std=c17 -Wall -Wextra -O2 -g -Isrc -Isrc/dut/common -Isrc/hal

BUILD_DIR := build

.PHONY: all build_a clean run

all: build_a

SRC_HAL   := src/hal/hal.c
SRC_SHARED_A := src/dut/group_a/shared_data_a.c
SRC_DUT_A := src/dut/group_a/main_a.c

# Group A (vulnerable)
build_a: $(BUILD_DIR)/group_a.elf
$(BUILD_DIR)/group_a.elf: $(SRC_DUT_A) $(SRC_HAL) $(SRC_SHARED_A) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DUT_A) $(SRC_HAL) $(SRC_SHARED_A)

clean:
	rm -rf $(BUILD_DIR)

run: build_a
	./$(BUILD_DIR)/group_a.elf

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)