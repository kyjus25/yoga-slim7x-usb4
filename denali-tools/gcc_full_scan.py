#!/usr/bin/env python3
"""Check all 124 X1E clock offsets on Denali. Logs results."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
GCC = 0x100000

with open("/home/kyjus25/gcc_offsets.txt") as f:
    offsets = [int(line.strip(), 16) for line in f if line.strip()]

print(f"Checking {len(offsets)} offsets...", file=sys.stderr)

results = []
for off in offsets:
    try:
        v = m.r32(GCC + off)
        enabled = (v & 0x1) == 0x1
        results.append((off, v, enabled, True))
    except:
        results.append((off, 0, False, False))

m.close()

# Summary
readable = sum(1 for r in results if r[3])
enabled = sum(1 for r in results if r[3] and r[2])
print(f"Readable: {readable}/{len(offsets)}, Enabled: {enabled}", file=sys.stderr)

# Save full results
with open("/tmp/gcc_scan_results.txt", "w") as out:
    for off, val, en, ok in results:
        if ok:
            out.write(f"0x{off:05x}: 0x{val:08x} {'EN' if en else 'dis'}\n")
        else:
            out.write(f"0x{off:05x}: READ_FAIL\n")

print("Results saved to /tmp/gcc_scan_results.txt", file=sys.stderr)
