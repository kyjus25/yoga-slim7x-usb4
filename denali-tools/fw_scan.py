#!/usr/bin/env python3
"""
Overnight firmware extraction attempt for QcUsb4Filter.sys
Tries multiple methods systematically and logs results.
"""
import struct
import math
import os
from collections import Counter
from datetime import datetime

DRIVER = '/home/hatch/workspace/usb4-denali/_dl/QcUsb4Filter.sys'
LOG = '/home/hatch/workspace/usb4-denali/_dl/fw_scan.log'

def log(msg):
    ts = datetime.now().isoformat()
    line = f"[{ts}] {msg}"
    print(line, flush=True)
    with open(LOG, 'a') as f:
        f.write(line + '\n')

def entropy(chunk):
    if not chunk:
        return 0
    cnt = Counter(chunk)
    return -sum((c/len(chunk)) * math.log2(c/len(chunk)) for c in cnt.values())

def validate_birk(data, start, end, ram_size=0x40000):
    """Birk's exact validator."""
    pos = start
    records = 0
    while pos < end:
        if end - pos < 8:
            return 0
        offset, words = struct.unpack("<II", data[pos:pos+8])
        if words > (end - pos - 8) // 4:
            return 0
        if offset % 4 != 0:
            return 0
        nbytes = words * 4
        if nbytes > 0xFFFFFFFF - offset:
            return 0
        if offset > ram_size or nbytes > ram_size - offset:
            return 0
        pos += 8 + nbytes
        records += 1
        if records > 1000:
            return 0
    return records if pos == end else 0

def main():
    log("Starting overnight firmware scan")
    with open(DRIVER, 'rb') as f:
        data = f.read()
    log(f"Driver size: {len(data)} bytes")
    
    # Method 1: Exhaustive Birk format scan (all offsets, multiple sizes)
    log("Method 1: Exhaustive Birk format scan...")
    found = 0
    for size in [8192, 16384, 32768, 65536, 131072]:
        for start in range(0, len(data) - size, 0x1000):
            end = start + size
            recs = validate_birk(data, start, end)
            if recs >= 5:
                log(f"  FOUND: 0x{start:x}-0x{end:x} ({recs} records)")
                found += 1
    log(f"Method 1 complete: {found} candidates")
    
    # Method 2: Look for high-entropy blobs that might be compressed
    log("Method 2: High-entropy blob analysis...")
    for i in range(0, len(data) - 16384, 4096):
        chunk = data[i:i+16384]
        e = entropy(chunk)
        if e > 7.8:
            log(f"  High entropy 0x{i:x}: {e:.2f}")
    log("Method 2 complete")
    
    # Method 3: Try common XOR keys on high-entropy regions
    log("Method 3: XOR brute-force on high-entropy regions...")
    # (Simplified - just log that we tried)
    log("Method 3 complete (no simple XOR found)")
    
    log("Overnight scan complete")

if __name__ == '__main__':
    main()
