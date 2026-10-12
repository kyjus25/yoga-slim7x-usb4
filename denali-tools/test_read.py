#!/usr/bin/env python3
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO
m = MMIO()
print("gpio163:", hex(m.r32(0x0f1a3000)))
print("gpio164:", hex(m.r32(0x0f1a4000)))
print("UC+0x0:", hex(m.r32(0x15600000)))
m.close()
