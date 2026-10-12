#!/usr/bin/env python3
"""Risky: Scan GCC for GDSC patterns not in X1E list.
Scans in small chunks, skips on failure. Run as root.
WARNING: May cause segfaults/hard locks. Justin approved.
"""
import os
import sys
import subprocess

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

# Known X1E GDSC offsets (to skip)
KNOWN = {
    0xa0004, 0x2c004, 0x13004, 0x58004, 0x3e000, 0x6b004, 0x6c000,
    0x2f004, 0x30000, 0x8e000, 0x31004, 0x8d004, 0x9e000, 0x77004,
    0x29004, 0x17004, 0x39004, 0xa1004, 0xa2004, 0x1900c, 0x5400c,
    0x9f004, 0x2b004, 0x11004, 0x50024, 0x2a024, 0xa3024,
}

GCC = 0x100000
# Scan 0x0 to 0x200000 in 0x10000 chunks (64KB each, 32 chunks total)
# Focus on areas not covered by known offsets

print("Scanning for unknown GDSCs...", file=sys.stderr)

found = []
for chunk_start in range(0, 0x200000, 0x10000):
    chunk_end = chunk_start + 0x10000
    # Skip if chunk is fully covered by known offsets? No, check anyway
    
    # Use subprocess to isolate crashes
    cmd = [
        "python3", "-c",
        f"""
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO
m = MMIO()
GCC = 0x100000
KNOWN = {KNOWN}
for off in range({chunk_start}, {chunk_end}, 4):
    if off in KNOWN:
        continue
    try:
        v = m.r32(GCC + off)
        # GDSC pattern: bit 0 set, upper bits look like 0x0022 or 0xf822
        if (v & 0x1) and ((v & 0xFFFF0000) in (0x00220000, 0xF8220000)):
            print(f"0x{{off:x}}: 0x{{v:08x}}")
    except:
        pass
m.close()
"""
    ]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.stdout:
            for line in result.stdout.strip().split('\n'):
                if line:
                    print(line)
                    found.append(line)
        if result.returncode != 0 and "139" in str(result.returncode):
            print(f"Chunk 0x{chunk_start:x}-0x{chunk_end:x}: SEGFAULT (skipped)", file=sys.stderr)
    except subprocess.TimeoutExpired:
        print(f"Chunk 0x{chunk_start:x}: TIMEOUT", file=sys.stderr)
    except Exception as e:
        print(f"Chunk 0x{chunk_start:x}: ERROR {e}", file=sys.stderr)

print(f"\nFound {len(found)} candidates", file=sys.stderr)
