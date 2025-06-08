OUTPUT_NAME := program
SRCS := $(wildcard *.c)
ASM_SRCS := $(wildcard *.S)
OBJS := $(patsubst %.c,%.o,$(SRCS))
ASM_OBJS := $(patsubst %.S,%.o,$(ASM_SRCS))
ALL_OBJS := $(OBJS) $(ASM_OBJS)
CFLAGS := -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only -g 

.PHONY: default
default: $(OUTPUT_NAME).img
# default: test

$(OUTPUT_NAME).img: $(OUTPUT_NAME).elf
	aarch64-linux-gnu-objcopy -O binary $^ $@
	cp $@ ../rootfs

$(OUTPUT_NAME).elf: $(ALL_OBJS)
	@echo "Linking: $@"
	@echo "Objects: $(ALL_OBJS)"
	aarch64-linux-gnu-ld -T linker.ld -o $@ $(ALL_OBJS)

# Compile assembly files
%.o: %.S
	@echo "Compiling: $<"
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

# Compile C files
%.o: %.c
	@echo "Compiling: $<"
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.img *.elf
