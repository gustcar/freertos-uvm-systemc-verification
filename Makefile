CC		:= gcc
CFLAGS	:= -std=c17 -Wall -Wextra -O2 -g -Isrc -Isrc/dut/common -Isrc/hal

BUILD_DIR := build

.PHONY: all build_a clean run

all: build_a

# Group A (vulnerable)
build_a: $(BUILD_DIR)/group_a.elf
$(BUILD_DIR)/group_a.elf: src/dut/group_a/main_a.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

run: build_a
	./$(BUILD_DIR)/group_a.elf

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)