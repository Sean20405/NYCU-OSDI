import struct
import serial
from time import sleep

kernel_path = 'build/kernel8.img'
port_path = '/dev/ttyUSB0'
baud_rate = 115200
block_size = 64

with open(kernel_path, 'rb') as f:
    kernel_data = f.read()

checksum = 0
for byte in kernel_data:
    checksum += byte
    checksum &= 0xffffffff

header = struct.pack('<III', 
    0x544F4F42,         # magic, "BOOT" in hex
    len(kernel_data),   # size
    checksum            # checksum
)

print('Kernel size:', hex(len(kernel_data)), 'checksum:', hex(checksum))
print('Header:', header)

with serial.Serial(port_path, baud_rate, timeout=1) as ser:
    ser.write(header)
    ser.flush()
    sleep(1)
    
    ser.write(kernel_data)
    ser.flush()

    print('Kernel sent')

