OUTPUT_NAME := kernel8
BUILD_DIR := build
SRCS_DIR := src
$(shell mkdir -p $(BUILD_DIR))
SRCS := $(wildcard $(SRCS_DIR)/*.c)
ASM_SRCS := $(wildcard $(SRCS_DIR)/*.S)
OBJS := $(patsubst $(SRCS_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
ASM_OBJS := $(patsubst $(SRCS_DIR)/%.S,$(BUILD_DIR)/%.o,$(ASM_SRCS))
ALL_OBJS := $(OBJS) $(ASM_OBJS)
CFLAGS := -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only -g 

.PHONY: default
default: $(BUILD_DIR)/$(OUTPUT_NAME).img
# default: test

$(BUILD_DIR)/$(OUTPUT_NAME).img: $(BUILD_DIR)/$(OUTPUT_NAME).elf
	aarch64-linux-gnu-objcopy -O binary $^ $@

$(BUILD_DIR)/$(OUTPUT_NAME).elf: $(ALL_OBJS)
	@echo "Linking: $@"
	@echo "Objects: $(ALL_OBJS)"
	aarch64-linux-gnu-ld -T $(SRCS_DIR)/linker.ld -o $@ $(ALL_OBJS)

# Compile assembly files
$(BUILD_DIR)/%.o: $(SRCS_DIR)/%.S
	@echo "Compiling: $<"
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: $(SRCS_DIR)/%.c
	@echo "Compiling: $<"
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

# Run on QEMU
.PHONY: run
run: $(BUILD_DIR)/$(OUTPUT_NAME).img
	qemu-system-aarch64 -M raspi3b -kernel $^ -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -display none -serial null -serial stdio

# Run on QEMU with GDB
.PHONY: run-gdb
run-gdb: $(BUILD_DIR)/$(OUTPUT_NAME).img
	qemu-system-aarch64 -M raspi3b -kernel $^ -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -display none -serial null -serial stdio -S -s 

.PHONY: clean
clean:
	rm -f $(BUILD_DIR)/*

test:
	@echo $(ALL_OBJS)
