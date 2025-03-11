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
        make || exit 1
        
        # Copy the kernel image to the USB device
        while [ ! -e /dev/sdd1 ]; do
            echo "Waiting for USB device to be mounted..."
            sleep 1
        done
        sudo mount /dev/sdd1 /mnt/usb
        sudo cp build/kernel8.img /mnt/usb
        ls -al /mnt/usb | grep kernel8.img
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