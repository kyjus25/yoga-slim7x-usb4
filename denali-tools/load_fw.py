#!/usr/bin/env python3
"""
Load UC firmware via /dev/mmio. 
Firmware format: (u32 offset, u32 word_count, u32 words...)
Offsets are relative to UC RAM (0x15600000).
"""
import os
import struct
import sys

sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

UC_BASE = 0x15600000
FW_PATH = "/home/kyjus25/x1-diag-mcu.bin"

def main():
    with open(FW_PATH, 'rb') as f:
        fw = f.read()
    print(f"Firmware: {len(fw)} bytes")
    
    m = MMIO()
    
    pos = 0
    records = 0
    total_words = 0
    while pos < len(fw):
        if pos + 8 > len(fw):
            print(f"Truncated at {pos}")
            break
        offset, count = struct.unpack("<II", fw[pos:pos+8])
        pos += 8
        if count == 0 or pos + count*4 > len(fw):
            print(f"Invalid record at {pos-8}: offset=0x{offset:x}, count={count}")
            break
        # Write the words
        for i in range(count):
            word = struct.unpack("<I", fw[pos:pos+4])[0]
            m.w32(UC_BASE + offset + i*4, word)
            pos += 4
        records += 1
        total_words += count
        if records % 10 == 0:
            print(f"  {records} records, {total_words} words...")
    
    print(f"Done: {records} records, {total_words} words ({total_words*4} bytes)")
    
    # Verify by reading back first few words
    print("\nVerifying first record...")
    offset, count = struct.unpack("<II", fw[0:8])
    for i in range(min(4, count)):
        expected = struct.unpack("<I", fw[8+i*4:12+i*4])[0]
        actual = m.r32(UC_BASE + offset + i*4)
        match = "OK" if expected == actual else "MISMATCH"
        print(f"  0x{offset+i*4:x}: expected 0x{expected:08x}, got 0x{actual:08x} [{match}]")
    
    m.close()

if __name__ == '__main__':
    main()
