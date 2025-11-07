#!/bin/bash

# This script will set up the USB device in WSL

# Load the USB modules
sudo modprobe usbcore
sudo modprobe usb-common
sudo modprobe hid-generic
sudo modprobe hid
sudo modprobe usbnet
sudo modprobe cdc_ether
sudo modprobe rndis_host
sudo modprobe usbserial
sudo modprobe usb-storage
sudo modprobe cdc-acm
sudo modprobe ftdi_sio
sudo modprobe usbip-core
sudo modprobe vhci-hcd
sudo modprobe cp210x

# Get the USB device
usbip_output=$(sudo usbip list --remote=192.168.240.1)
echo "$usbip_output"

# Parse to get usbids
usbids=$(echo "$usbip_output" | grep -oP '\d+-\d+')
echo "Found USB devices: $usbids"

for usbid in $usbids; {
    sudo usbip attach --remote=192.168.240.1 --busid=$usbid && echo "Attached USB device $usbid"

    is_card_reader=$(echo "$usbip_output" | grep -A 3 "$usbid" | grep -i "card reader")
    
    if [ -n "$is_card_reader" ]; then 
        echo "Device $usbid is a card reader"

        # Compile bootloader.img
        cd bootloader
        make || exit 1
        cd ../
        
        # Compile simple user program to test exception
        cd user
        make || exit 1
        cd ../

        # Pack the rootfs
        echo "Packing rootfs..."
        cd rootfs
        find . | cpio -o -H newc > ../initramfs.cpio
        cd ../

        # Copy the bootloader to the USB device
        while [ ! -e /dev/sdd1 ]; do
            echo "Waiting for USB device to be mounted..."
            sleep 1
        done
        sudo mount /dev/sdd1 /mnt/usb
        sudo cp bootloader/build/bootloader.img config.txt initramfs.cpio bcm2710-rpi-3-b-plus.dtb /mnt/usb

        # Check the file is newest
        ls -al /mnt/usb | grep -E 'bootloader.img|config.txt|initramfs.cpio|bcm2710-rpi-3-b-plus.dtb' | awk '{
            orig=$0
            timestamp=$6" "$7" "$8
            bold_green="\033[1;32m"
            reset="\033[0m"
            gsub(timestamp, bold_green timestamp reset)
            print
        }'
        sudo umount /dev/sdd1
        echo "Copied kernel image to USB device"
        
    else
        while [ ! -e /dev/ttyUSB0 ]; do
            echo "Waiting for USB device to be detected..."
            sleep 1
        done
        sudo minicom
    fi
}