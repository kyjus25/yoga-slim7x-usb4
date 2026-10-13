# USB4 Command Ring Format — Denali UC Testing

**Date:** 2026-10-12
**Status:** Research complete, ready for hardware testing
**UC Base:** `0x15600000` (powered on and responding)

## Register Interface

| Offset | Name | Access | Description |
|--------|------|--------|-------------|
| `0x2078` | RING_BASE | R/W | Physical address of the command ring buffer |
| `0x207c` | DOORBELL | R/W | Write to signal new commands |

**Ring entry size:** 16 bytes (from QcUsb4Filter8380.sys analysis)

## 16-Byte Command Entry Format

Based on Linux `drivers/thunderbolt/tb_msgs.h` (GPL-2.0) and `drivers/thunderbolt/ctl.c`.

The Qualcomm UC uses a simplified ring compared to Intel's NHI descriptor rings.
Each 16-byte entry is a complete command packet:

```
Offset  Size  Field           Value (for test command)
------  ----  -----           ------------------------
0       4     packet_type     0x00000001 (TB_CFG_PKG_READ)
4       4     route_hi        0x00000000 (bits 0-21: route high, bits 22-31: flags)
8       4     route_lo        0x00000000 (local router, route = 0)
12      4     address         0x04002000 (see breakdown below)
```

### Address Field Breakdown (`tb_cfg_address`)

```c
struct tb_cfg_address {
    u32 offset:13;  /* bits 0-12:  dword offset in config space */
    u32 length:6;   /* bits 13-18: number of dwords to read */
    u32 port:6;     /* bits 19-24: port number (0 for router) */
    u32 space:2;    /* bits 25-26: config space (2 = TB_CFG_SWITCH) */
    u32 seq:2;      /* bits 27-28: sequence number */
    u32 zero:3;     /* bits 29-31: must be zero */
};
```

For the test command (`0x04002000`):
- `offset` = 0 (bits 0-12)
- `length` = 1 (bits 13-18) → `1 << 13` = `0x2000`
- `port` = 0 (bits 19-24)
- `space` = 2 = `TB_CFG_SWITCH` (bits 25-26) → `2 << 25` = `0x4000000`
- `seq` = 0 (bits 27-28)
- `zero` = 0 (bits 29-31)

`0x4000000 | 0x2000` = `0x04002000`

### Packet Type Values (`enum tb_cfg_pkg_type`)

From `include/linux/thunderbolt.h`:
- `TB_CFG_PKG_READ = 1` — Config space read request
- `TB_CFG_PKG_WRITE = 2` — Config space write request
- `TB_CFG_PKG_ERROR = 3` — Error response
- `TB_CFG_PKG_RESET = 9` — Router reset

## Safe Test Command: Read Router Vendor ID

**Command:** Read 1 dword from local router config space, offset 0.

This is read-only and cannot change hardware state. Offset 0 of
`TB_CFG_SWITCH` space contains the router's vendor/device identification.

**16-byte ring entry (little-endian):**
```
Byte:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
Value: 01 00 00 00 00 00 00 00 00 00 00 00 00 20 00 04
```

As 32-bit words:
- `w0 = 0x00000001` (TB_CFG_PKG_READ)
- `w1 = 0x00000000` (route_hi = 0)
- `w2 = 0x00000000` (route_lo = 0, local router)
- `w3 = 0x04002000` (addr: offset=0, len=1, port=0, space=SWITCH)

## Test Procedure

### 1. Allocate Ring Buffer

Allocate a DMA-coherent buffer for the ring. Minimum 1 entry (16 bytes),
recommend 16 entries (256 bytes) for future use. Must be physically
contiguous.

```c
dma_addr_t ring_phys;
void *ring = dmam_alloc_coherent(dev, 256, &ring_phys, GFP_KERNEL);
```

### 2. Program Ring Base

Write the **physical address** to `UC+0x2078`:

```c
writel(lower_32_bits(ring_phys), uc_base + 0x2078);
```

Note: If the UC expects a 64-bit address, there may be a second register
at `0x207c` for the high bits. The current analysis shows `0x207c` as the
doorbell, suggesting a 32-bit address or an implicit high-bits register.
Verify by reading back `0x2078` after writing.

### 3. Write Command to Ring[0]

Copy the 16-byte command to the start of the ring buffer:

```c
u32 *entry = (u32 *)ring;
entry[0] = 0x00000001;  /* packet_type */
entry[1] = 0x00000000;  /* route_hi */
entry[2] = 0x00000000;  /* route_lo */
entry[3] = 0x04002000;  /* address */
/* Ensure writes are visible to hardware */
dma_wmb();
```

### 4. Ring Doorbell

Write to `UC+0x207c` to signal the UC:

```c
writel(1, uc_base + 0x207c);  /* signal 1 new command */
```

The exact doorbell value semantics are not confirmed from static analysis.
Candidates:
- **Write 1**: Signal one new entry (most likely)
- **Write ring index**: Current producer index (0 for first command)
- **Write bit 0**: Simple trigger bit

Start with `1`. If no response, try `0`.

### 5. Wait for Completion

Poll the ring entry for a response. The UC will overwrite the entry (or
write to a separate RX ring) with the reply.

**Reply format** for a read request is `TB_CFG_PKG_WRITE` (type 2):
```
Offset  Size  Field
0       4     packet_type = 0x00000002 (TB_CFG_PKG_WRITE as response)
4       4     route_hi with reply bit set (bit 31 = 0x80000000)
8       4     route_lo = 0x00000000
12      4     address (echo of request)
16      4     data[0] = vendor ID dword (if separate buffer)
```

**Important:** The reply bit is the highest bit of the `unknown` field in
`tb_cfg_header` (bit 31 of word 1). A valid reply will have:
```
w1 & 0x80000000 != 0
```

**Polling loop:**
```c
int timeout = 1000;  /* 1 second, 1ms polls */
while (timeout--) {
    dma_rmb();  /* ensure we see UC writes */
    u32 w1 = entry[1];
    if (w1 & 0x80000000) {
        /* Reply received! */
        u32 data = entry[4];  /* first data dword (if in same entry) */
        pr_info("UC replied: vendor=0x%08x\n", data);
        break;
    }
    mdelay(1);
}
if (timeout <= 0)
    pr_warn("UC did not respond to command\n");
```

### 6. Expected Results

**If the UC is fully functional:**
- Reply arrives within milliseconds
- `w1` has bit 31 set (`0x80000000`)
- `w0` = `0x00000002` (TB_CFG_PKG_WRITE as read response)
- Data dword contains router vendor/device ID

**If the UC is alive but not processing commands:**
- No reply (timeout)
- Possible causes:
  - Doorbell value incorrect (try 0 instead of 1)
  - Ring base address not accepted (check 64-bit addressing)
  - UC firmware not loaded (may need FW load before commands work)
  - Separate TX/RX rings (response goes to different buffer)

**If the UC hard-locks:**
- System hangs on doorbell write
- Indicates the UC was not ready for commands
- Recover via power-button reboot (previously authorized for hard-lock testing)

## Safety Notes

1. **This command is read-only.** It cannot modify UC state, configure ports,
   or trigger link training. It is the safest possible test.

2. **Do NOT send** `TB_CFG_PKG_RESET` (type 9) or `TB_CFG_PKG_WRITE` (type 2)
   until read commands work reliably. Writes can change router configuration.

3. **Hard-lock risk:** If the UC firmware is not loaded or the ring format
   is wrong, the doorbell write could hang the UC (not the whole SoC, since
   the UC is a separate microcontroller). The parent agent has authorized
   recoverable hard-lock testing for targeted register writes.

4. **Start with instance 0** (the primary USB4 controller). Instances 1 and 2
   use the same format with different GDSC/clock bases.

## References

- Linux `drivers/thunderbolt/tb_msgs.h` — `struct tb_cfg_header`,
  `struct tb_cfg_address`, `struct cfg_read_pkg` (GPL-2.0)
- Linux `drivers/thunderbolt/ctl.c` — `tb_cfg_read_raw()`, packet framing
  with `sof`/`eof`/`size`/`crc` (GPL-2.0)
- Linux `include/linux/thunderbolt.h` — `enum tb_cfg_pkg_type` (GPL-2.0)
- USB4 Specification — Section on Configuration Space and Router Operations
  (USB-IF, membership required for full text)
- `docs/DENALI-UC-BRINGUP-2026-10-12.md` — Denali UC power-on verification

## Open Questions

1. **Doorbell semantics:** Is it a count, an index, or a simple trigger?
   The Windows driver wrote to it, but the exact value pattern was not
   recovered from static analysis.

2. **Ring base addressing:** 32-bit or 64-bit? If 64-bit, where are the high
   bits? Possibly `0x2078` is low and an adjacent register holds high bits.

3. **RX path:** Does the UC write replies back to the same ring entry, or
   is there a separate RX ring? The `0x2078` register was described as
   "ring-base offset/read" suggesting it might be for TX only.

4. **Firmware:** Does the UC need firmware loaded before it processes
   commands? The `qcom/x1-diag-mcu.bin` (40,816 bytes) was extracted from
   the Windows driver but its loading mechanism is unknown.

5. **Interrupts:** Is there an MSI or wired interrupt for command completion,
   or is polling the only option? Check the UC interrupt status registers
   if polling fails.
