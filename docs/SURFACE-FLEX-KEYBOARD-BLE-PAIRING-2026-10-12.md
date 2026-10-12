# Bluetooth OOB Pairing Protocol Analysis — SurfaceService.exe

**Binary:** `SurfaceService.exe` (2,594,160 bytes, ARM64 PE32+)
**Service:** Surface Integration Service (Windows)
**Date analyzed:** 2026-10-12
**Method:** Static analysis (capstone disassembly, PE import analysis)

## Executive Summary

SurfaceService.exe handles out-of-band (OOB) Bluetooth pairing for the Surface Pro
Flex Keyboard when attached via pogo pins. It communicates with the keyboard's
pogo-pin HID interface using **HID Feature reports** (not Bluetooth APIs directly).
The Bluetooth keys are exchanged via HID, then stored by the Windows Bluetooth stack
for later BLE connections when the keyboard is detached.

**Key finding:** The service imports `HID.DLL` (HidD_GetFeature, HidD_SetFeature,
HidD_GetAttributes, etc.) but does **NOT** import `bluetoothapis.dll` or any
Bluetooth-specific APIs. All pairing data flows through HID Feature reports.

## HID API Call Sites

### HidD_GetFeature (IAT 0x1401cf0e8, offset 0xe8)

| Address | Buffer | Length | Notes |
|---------|--------|--------|-------|
| 0x14005d098 | x1=[x21,#8] | (from earlier) | Handle in x0=[x19,#0x38] |
| 0x14005d720 | x1=[x20,#0x70] | (from earlier) | Handle in x0=[x19,#0x38] |
| 0x140071b54 | x1=x19 | w2=0x7e (126) | Handle in x0=x20; report ID from [sp,#0x52] |

### HidD_SetFeature (IAT 0x1401cf128, offset 0x128)

| Address | Buffer | Length | Notes |
|---------|--------|--------|-------|
| 0x14005cd34 | x1=[x19,#8] | w2=[x19,#0x10] (16-bit) | Handle in x0=[x20,#0x38] |
| 0x14008a82c | x1=[x20,#8] | w2=[x20,#0x10] (16-bit) | Handle in x0=[x21,#0x38] |

### Calling Convention (ARM64)

```c
BOOLEAN HidD_GetFeature(
    HANDLE HidDeviceObject,  // x0
    PVOID  ReportBuffer,     // x1 (first byte = Report ID)
    ULONG  ReportBufferLength // x2
);

BOOLEAN HidD_SetFeature(
    HANDLE HidDeviceObject,  // x0
    PVOID  ReportBuffer,     // x1 (first byte = Report ID)
    ULONG  ReportBufferLength // x2
);
```

## Detailed: GetFeature at 0x140071b54

```asm
0x140071b2c: ldrb w8, [sp, #0x52]   ; w8 = report ID (from stack)
0x140071b30: cbz  w8, #0x140071b7c  ; if report ID == 0, skip
0x140071b34: cmp  x11, #0x7d         ; check x11 == 125 (0x7d)
0x140071b38: b.ne #0x140071b7c       ; if not, skip
0x140071b3c: strb w8, [x19]          ; [buffer] = report ID
0x140071b40: adrp x8, #0x1401cf000
0x140071b44: ldr  x8, [x8, #0xe8]    ; x8 = HidD_GetFeature
0x140071b48: mov  w2, #0x7e          ; length = 126
0x140071b4c: mov  x1, x19            ; buffer = x19
0x140071b50: mov  x0, x20            ; handle = x20
0x140071b54: blr  x8                 ; HidD_GetFeature(handle, buffer, 126)
```

**Observations:**
- Report ID is dynamically determined (from stack, not hardcoded)
- Buffer length is 126 bytes (0x7e)
- There's a validation check (x11 must equal 125)
- The report ID is written as the first byte of the buffer before the call

## Detailed: SetFeature at 0x14005cd34

```asm
0x14005cd20: ldrh w2, [x19, #0x10]   ; w2 = length (16-bit from [x19+0x10])
0x14005cd24: ldr  x1, [x19, #8]      ; x1 = buffer (from [x19+8])
0x14005cd28: ldr  x0, [x20, #0x38]   ; x0 = handle (from [x20+0x38])
0x14005cd2c: adrp x8, #0x1401cf000
0x14005cd30: ldr  x8, [x8, #0x128]   ; x8 = HidD_SetFeature
0x14005cd34: blr  x8                 ; HidD_SetFeature(handle, buffer, length)
0x14005cd38: uxtb w8, w0             ; check return (BOOLEAN)
```

**Observations:**
- Buffer and length come from a context structure (x19)
- Buffer pointer at [x19+8], length (16-bit) at [x19+0x10]
- Handle at [x20+0x38]

## What We Know About the Protocol

### Transport
- **HID Feature reports** over the pogo-pin USB/HID interface
- **NOT** Bluetooth APIs — the service never talks to the Bluetooth stack directly
- The Windows Bluetooth stack likely picks up the keys via a separate mechanism
  (possibly the service writes them to the registry, or signals the BT stack)

### Report Structure
- First byte = Report ID (standard HID)
- At least one report uses 126-byte buffers
- Report IDs are dynamic (not hardcoded immediates in the call sites analyzed)

### Device Identification
The service likely enumerates HID devices via SetupAPI (it imports setupapi.dll)
and identifies the keyboard's pogo-pin interface by:
- VID/PID (Microsoft 045E, keyboard PID)
- Usage Page/Usage (from HID descriptor via HidP_GetCaps)
- Or device interface GUID

**Not yet determined:** The exact enumeration logic (which SetupDi calls, what
filters are applied).

## What's Still Unknown

### High Priority
1. **Report IDs**: The exact Feature report IDs used for key exchange. The IDs
   are loaded from memory/stack, not hardcoded — requires deeper data-flow
   analysis to trace.
2. **Data format**: Where in the 126-byte buffer are the Bluetooth keys?
   - LTK is 16 bytes — look for 16-byte fields
   - Bluetooth address is 6 bytes
   - There may be EDIV, Rand (for LE legacy pairing) or other key material
3. **Sequence**: Does it do GetFeature then SetFeature? Or multiple rounds?
   The 5 call sites suggest a multi-step handshake, but the order isn't clear
   from static analysis alone.
4. **Trigger**: What causes the service to initiate pairing? Likely a device
   arrival notification (WM_DEVICECHANGE or SetupDi callback) when the
   keyboard is attached.

### Medium Priority
5. **Key storage**: After the HID exchange, where do the keys go? Options:
   - Written to Windows Bluetooth registry keys
   - Passed to BTHPORT via IOCTL
   - Stored by the service and provided on-demand
6. **Pogo-pin HID descriptor**: What does the keyboard's wired HID interface
   look like? (Would need to capture via USBPcap or similar on Windows)

### Low Priority
7. **Error handling**: What happens if pairing fails? Retry logic?
8. **Other features**: The service likely does more than pairing (firmware
   updates, battery status, etc.) — out of scope for the BLE pairing task.

## Recommendations for Linux

### Option 1: Replicate the OOB Protocol (Complex)
1. Detect keyboard attachment via pogo pins (USB HID device appears)
2. Open the HID device
3. Send/receive the Feature reports with correct IDs and format
4. Extract the Bluetooth keys
5. Inject keys into Linux Bluetooth stack (via `bluetoothctl` or D-Bus
   `org.bluez.Device1` with pre-shared keys, or via `btmgmt`)

**Blocker:** Need the exact report IDs and data format (items 1-2 above).

### Option 2: Manual BLE Pairing (Simpler)
1. Put keyboard in discoverable mode (check for button combo on Flex Keyboard)
2. Use `bluetoothctl` to scan, pair, trust, connect
3. The keyboard should work as standard HID-over-GATT (VID 045E, PID 0C7A)

**Note:** If the Flex Keyboard has NO manual pairing mode (designed for
pogo-pin OOB only), this option won't work.

### Option 3: Extract Keys from Windows (One-Time)
1. On Windows, after successful OOB pairing, export the Bluetooth keys from
   registry: `HKLM\SYSTEM\CurrentControlSet\Services\BTHPORT\Parameters\Keys`
2. Import into Linux via `btmgmt` or BlueZ config
3. Keys persist across reboots (until keyboard is reset)

**Limitation:** Keys are device-specific; won't help other users.

## Confidence Levels

- **High**: Service uses HID Feature reports (not BT APIs) — confirmed via imports
- **High**: 5 HID call sites identified with addresses — confirmed via disassembly
- **High**: ARM64 calling convention (x0=handle, x1=buffer, x2=length) — standard
- **Medium**: 126-byte buffer is for key exchange — inferred from size (fits keys
  + addresses + metadata), not confirmed
- **Low**: Exact report IDs — not yet traced through data flow
- **Low**: Data format within reports — not yet analyzed
- **Unknown**: Full pairing sequence and trigger mechanism

## Files
- Binary: `SurfaceService.exe` (2,594,160 bytes, SHA256: TBD)
- This report: `oob-pairing-protocol.md`

## Next Steps
1. Trace report IDs: For each of the 5 call sites, perform backward data-flow
   analysis to find where the report ID byte originates.
2. Identify the HID device: Find the SetupDi enumeration code to see how the
   pogo-pin interface is identified (VID/PID/usage).
3. Dynamic analysis: On Windows with keyboard attached, use API Monitor or
   similar to capture the actual Feature report bytes during pairing.
4. The 126-byte report at 0x140071b54 is the best starting point — it has
   the clearest structure (explicit length, report ID from stack).
