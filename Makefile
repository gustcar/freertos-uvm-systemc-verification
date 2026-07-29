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

# --- UVM-SystemC Testbench ---
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O0 -g \
            -Isrc -Isrc/dut/common -Isrc/hal -Itb \
            -I/usr/local/include -I/usr/local/include/uvmsc

SYSLIB   := -L/usr/local/lib -L/usr/local/lib-linux64 \
            -lsystemc -luvm-systemc -lpthread

TB_CORE_SRCS	 := tb/sc_main.cpp tb/env/env.cpp

DUT_A_C_SRCS := src/hal/hal.c src/dut/group_a/shared_data_a $(SRC_TASKS_A)
DUT_A_OBJS   := $(patsubst %.c, $(BUILD_DIR)/%.o, $(DUT_A_C_SRCS))

DUT_B_C_SRCS := src/hal/hal.c src/dut/group_b/shared_data_b $(SRC_TASKS_B)
DUT_B_OBJS   := $(patsubst %.c, $(BUILD_DIR)/%.o, $(DUT_B_C_SRCS))

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $CFLAGS -c -o $@ $

$(BUILD_DIR)/tb_a.elf: $(TB_CORE_SRCS) $(DUT_A_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DDUT_GROUP_A -o $@ $(TB_CORE_SRCS) $(DUT_A_OBJS) $(SYSLIB)

$(BUILD_DIR)/tb_b.elf: $(TB_CORE_SRCS) $(DUT_B_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DDUT_GROUP_B -o $@ $(TB_CORE_SRCS) $(DUT_B_OBJS) $(SYSLIB)

.PHONY: build-tb-a build-tb-b run-tb-a run-tb-b
build-tb-a: $(BUILD_DIR)/tb_a.elf
build-tb-b: $(BUILD_DIR)/tb_b.elf

run-tb-a: build-tb-a
	./$(BUILD_DIR)/tb_a.elf race_condition_test

run-tb-b: build-tb-B
	./$(BUILD_DIR)/tb_b.elf protected_test

.PHONY: check-headers
check-headers:
	$(CXX) $(CXXFLAGS) -fsyntax-only -Isrc -Isrc/dut/common -Isrc/hal -Itb \
	    -I/usr/local/include tb/agents/sensor_agent/sensor_driver.h tb/agents/sensor_agent/sensor_monitor.h \
		tb/agents/sensor_agent/sensor_sequencer.h tb/agents/sensor_agent/sensor_seq_item.h \
		tb/agents/sensor_agent/sensor_agent.h tb/agents/comm_agent/comm_driver.h tb/agents/comm_agent/comm_monitor.h \
		tb/agents/comm_agent/comm_sequencer.h tb/agents/comm_agent/comm_seq_item.h tb/agents/comm_agent/comm_agent.h \
		tb/agents/actuator_agent/actuator_seq_item.h tb/agents/actuator_agent/actuator_monitor.h tb/agents/actuator_agent/actuator_agent.h \
		tb/coverage/coverage_bins.h tb/checker/data_integrity_checker.h tb/checker/timing_checker.h \
		tb/checker/deadlock_detector.h tb/scoreboard/concurrency_sb.h \
		tb/sequences/base_seq.h tb/sequences/race_condition_seq.h tb/sequences/priority_inversion_seq.h \
		tb/tests/race_condition_test.h tb/tests/protected_test.h

