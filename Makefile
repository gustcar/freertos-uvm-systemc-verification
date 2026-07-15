CC		:= gcc
CFLAGS	:= -std=c17 -Wall -Wextra -O2 -g -Isrc -Isrc/dut/common -Isrc/hal -D_GNU_SOURCE
LDFLAGS := -lpthread

BUILD_DIR := build

.PHONY: all build-a build-b clean run run-a run-b

all: build-a

SRC_HAL			:= src/hal/hal.c

# Group A (vulnerable)
SRC_SHARED_A	:= src/dut/group_a/shared_data_a.c
SRC_TASKS_A		:= src/dut/group_a/sensor_task.c \
				   src/dut/group_a/control_task.c \
				   src/dut/group_a/comm_task.c \
				   src/dut/group_a/alarm_task.c \
				   src/dut/group_a/logger_task.c
SRC_DUT_A		:= src/dut/group_a/main_a.c

# Group B (protected)
SRC_SHARED_B	:= src/dut/group_b/shared_data_b.c
SRC_TASKS_B		:= src/dut/group_b/sensor_task_safe.c \
				   src/dut/group_b/control_task_safe.c \
				   src/dut/group_b/comm_task_safe.c \
				   src/dut/group_b/alarm_task_safe.c \
				   src/dut/group_b/logger_task_safe.c
SRC_DUT_B		:= src/dut/group_b/main_b.c

# Group A (vulnerable)
build-a: $(BUILD_DIR)/group_a.elf

$(BUILD_DIR)/group_a.elf: $(SRC_DUT_A) $(SRC_HAL) $(SRC_SHARED_A) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DUT_A) $(SRC_HAL) $(SRC_SHARED_A) $(SRC_TASKS_A) $(LDFLAGS)

# Group B (protected)
build-b: $(BUILD_DIR)/group_b.elf

$(BUILD_DIR)/group_b.elf: $(SRC_DUT_B) $(SRC_HAL) $(SRC_SHARED_B) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DUT_B) $(SRC_HAL) $(SRC_SHARED_B) $(SRC_TASKS_B) $(LDFLAGS)

# Run targets
run-a: build-a
	./$(BUILD_DIR)/group_a.elf

run-b: build-b
	./$(BUILD_DIR)/group_b.elf

run: run-a

# Clean
clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Targets:"
	@echo "make build-a  - Compile group A (vulnerable)"
	@echo "make build-b  - Compile group B (protected)"
	@echo "make run-a    - Compile and run group A (vulnerable)"
	@echo "make run-b    - Compile and run group B (protected)"
	@echo "make clean    - Remove build artifacts"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)