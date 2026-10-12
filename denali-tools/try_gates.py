#!/usr/bin/env python3
"""Try enabling UC via gate registers, then test firmware write."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

UC_BASE = 0x15600000
m = MMIO()

print("Before:")
for off in [0xd064, 0x8018]:
    v = m.r32(UC_BASE + off)
    print(f"  UC+0x{off:05x}: 0x{v:08x}")

print("\nWriting 1 to gates...")
m.w32(UC_BASE + 0xd064, 0x1)
m.w32(UC_BASE + 0x8018, 0x1)

print("\nAfter:")
for off in [0xd064, 0x8018]:
    v = m.r32(UC_BASE + off)
    print(f"  UC+0x{off:05x}: 0x{v:08x}")

print("\nTesting UC RAM write...")
m.w32(UC_BASE + 0x0, 0x12345678)
v = m.r32(UC_BASE + 0x0)
print(f"  Wrote 0x12345678, read 0x{v:08x}")
if v == 0x12345678:
    print("  SUCCESS: UC RAM is accessible!")
else:
    print("  FAILED: UC RAM not accessible")

m.close()
