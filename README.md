# NYCU Operating System Capstone (OSC, formerly OSDI)

>[!Note]
> #### Course Information
> - Lecturer：葉宗泰
> - Semester：113-2, 2025 Spring
> - Grading：
>     - Lab * 7 95%
>         - Lab 1 – 2 takes 10% each, 3 – 7 takes 15% each
>         - Late submissions: 10% off per week
>     - In-class quiz 5%
> - [Course Introduction](https://timetable.nycu.edu.tw/?r=main/crsoutline&Acy=113&Sem=2&CrsNo=535502&lang=zh-tw)
> - [Course Website](https://people.cs.nycu.edu.tw/~ttyeh/course/2025_Spring/IOC5226/outline.html)
> - [Lab website](https://nycu-caslab.github.io/OSC2025/)

## Overview

This repository contains a small operating system kernel and supporting tools targeting the Raspberry Pi 3 (bcm2710, aarch64),
which is the full lab of course "Operating System Capstone" in NYCU.

The project includes:

- A minimal kernel that include:
	- a simple shell and Mini UART setting (Lab 1)
	- exception handler (Lab 3)
	- memory allocation (Lab 4)
	- thread management, user process support, context switch (Lab 5)
	- system call (Lab 5)
	- virtual memory (Lab 6)
	- virtual file system (Lab 7)
- An initial ramfs (`initramfs.cpio`) and example user programs (`rootfs/` and `user/`).
- Top-level `Makefile` for building a kernel image suited for QEMU or real hardware.
- A simple bootloader project under `bootloader/`.

## Requirements

Develop and test on Linux or WSL2 (Ubuntu).

```bash
sudo apt update
sudo apt install -y build-essential make cpio \
	gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu \
	qemu-system-aarch64 \
	usbip minicom
```

Notes:
- `gcc-aarch64-linux-gnu` and `binutils-aarch64-linux-gnu` provide the cross-compilation tools used in the repository.
- [Optional] `usbip` is used for WSL2 to support USB. `minicom` is for connect to the kernel terminal more easily.

## Quick start

1. Install the requirements above (on Ubuntu/WSL2).
2. From the repository root run:

```bash
make
```

This builds objects in `build/` and creates `build/kernel8.img` and `build/kernel8.elf`.

### Run with QEMU

- `make run` — run in QEMU headless (serial output to stdio).
- `make run-gui` — run in QEMU with a GUI display.
- `make run-gdb` — start QEMU and pause for GDB (`-S -s`), listen on TCP:1234.

Examples:

```bash
make run            # headless, serial output to terminal
make run-gui        # GUI mode
make run-gdb        # run QEMU and wait for gdb connection

# Debug with GDB (in another terminal):
gdb build/kernel8.elf
(gdb) target remote :1234
```

### Run on real device (Raspberry Pi 3)

1. (For first time) Use a USB card reader to insert the SD card.
2. (For first time) Run `run.sh` to copy files to SD card.
3. (For first time) Insert the SD card back to Raspberry Pi 3 and setup UART wire.
4. Run `run.sh` again. This time it will open `minicom` for rpi3 terminal since there is no USB card reader device. (Remember to setup minicom to link to the correct serial port.)
5. Open another terminal. Send the kernel by running `python send_kernel.py`

>[!Note]
> Step 1-3 is needed only when this is the first time sending files to the SD card, or you re-compile the bootloader. If only the kernel is modified, you can just insert UART wire and start from step 4.

>[!Warning]
> The script `run.sh` is used for WSL developer. For more details, please go to [WSL related issues](#wsl-related-issues).

## Branches / Lab history

Each `LabX` branch corresponds to the work for Lab X. The commit history is also preserved. You can check the message and difference, they show how I finish the lab at every steps.

## Project structure

```
.
├─ bootloader/     # A small standalone bootloader and its Makefile to build the boot image
├─ include/        # Headers
├─ rootfs/         # Files to pack into `initramfs.cpio`
├─ src/            # kernel C and assembly source codes
├─ user/           # Simple user program that runs in user space in Lab 3
├─ run.sh          # Script to copy files to SD card by USB in WSL. 
├─ send_kernel.py  # Script to send kernel image to bootloader (implemented in Lab 2)
└─ bcm2710-rpi-3-b-plus.dtb  # Device tree blob
```

## WSL related issues

Since [WSL cannot access USB device](https://github.com/microsoft/WSL/issues/7770), compiling USB-related kernel is needed for reading and writing contents in the USB storage device (at least in my case). I list the reference I read to solve this issue below. You can follow the tutorial or ~~find a Linux device~~.

- [Tutorial] [让 WSL 2 支持 USB | Dev on Windows with WSL](https://dowww.spencerwoo.com/4-advanced/4-4-usb.html#%E5%90%91-linux-%E4%BE%A7%E6%B7%BB%E5%8A%A0-usb-%E6%94%AF%E6%8C%81)
- [Tutorial] [Windows Mount Linux Partition in WSL 掛載 存取 Linux USB SD Card 隨身碟 EXT4 - CasparTing’s Blog](https://casparting.github.io/windows/Windows_mount_Linux_partition_wsl/)
- [Microsoft Docs] [連接 USB 裝置 | Microsoft Learn](https://learn.microsoft.com/zh-tw/windows/wsl/connect-usb)

### Running script

The script `run.sh` is just to simplify the whole setting process. It includes:

- attaches a remote USB device via `usbip`,
- builds the bootloader and example user program,
- packs `rootfs/` into `initramfs.cpio`, and
- copies `bootloader/build/bootloader.img`, `config.txt`, `initramfs.cpio` and `bcm2710-rpi-3-b-plus.dtb` to the target device (e.g. an SD/USB boot partition).

#### Basic steps (conceptual):

1. Build the bootloader:

```bash
make -C bootloader
```

2. Build the example user program if needed:

```bash
make -C user
```

3. Repack `initramfs` after changes to `rootfs/`:

```bash
cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio
cd ..
```

4. Run the WSL helper script to attach the USB device and copy files:

```bash
./run.sh
```

>[!Warning]
> `run.sh` is an example and assumes a specific `usbip`/device setup and that the target partition appears as `/dev/sdd1`. Adjust device names and steps for your environment.
