# Virtual RNG Device & Driver Exercise

## Project Overview
Implementation of a complete virtualization stack: a virtual Random Number Generator (RNG) hardware device emulated in QEMU, a Linux kernel driver, and a user-space application.

## Architecture
```
User Application (my-app.c)
        ↓ ioctl()
Kernel Driver (/dev/my_rng_driver)
        ↓ MMIO
Virtual Hardware (QEMU PCI Device)
```

## Components

### 1. Virtual Hardware (QEMU)
**File**: `qemu-8.2.0/hw/misc/my-rng.c`
- **Device**: PCI device (Vendor: 0x1234, Device: 0xcafe)
- **Registers**:
  - Offset 0: Read random number
  - Offset 4: Write seed value
- **Implementation**: Uses C stdlib `rand()` and `srand()`

**Build**:
```bash
cd qemu-8.2.0
make -j4 install
```

### 2. Kernel Driver
**File**: `linux-6.6/drivers/misc/my-rng.c`
- **Device**: Character device (major 250)
- **ioctl commands**:
  - `MY_RNG_IOCTL_RAND` (0x80047101): Get random number
  - `MY_RNG_IOCTL_SEED` (0x40047101): Seed the RNG
- **Memory mapping**: `ioremap()` at physical address 0xfebf1000

**Build**:
```bash
cd linux-6.6
make
```

### 3. User Application
**File**: `my-app.c` (in VM)
- Opens `/dev/my_rng_driver`
- Uses `ioctl()` to seed RNG and generate random numbers

**Build & Run (in VM)**:
```bash
apk add build-base
gcc my-app.c -o my-app
./my-app
```

## Quick Start

### 1. Launch VM
```bash
./launch-vm.sh
```

### 2. Setup Device Node (in VM)
```bash
mknod /dev/my_rng_driver c 250 0
```

### 3. Verify Device
```bash
# Check PCI device
apk add pciutils
lspci -v | grep -A 3 "1234:cafe"

# Check driver loaded
dmesg | grep my_rng_driver
```

### 4. SSH Access (optional)
From host:
```bash
ssh root@localhost -p 1022
```

## Device Specifications

| Component | Value |
|-----------|-------|
| Vendor ID | 0x1234 |
| Device ID | 0xcafe |
| PCI Class | 0x00ff (Unclassified) |
| MMIO Size | 4KB |
| Base Address | 0xfebf1000 (may vary) |
| Major Number | 250 |

## How It Works

1. **QEMU** emulates a PCI device with MMIO registers
2. **Kernel driver** maps device memory and registers character device
3. **User app** opens `/dev/my_rng_driver` and calls `ioctl()`
4. **Driver** translates ioctl to device register reads/writes
5. **Device** returns random numbers via `rand()` C function

## Example Output
```bash
./my-app
Round 0 number 0: 1804289383
Round 0 number 1: 846930886
Round 0 number 2: 1681692777
Round 0 number 3: 1714636915
Round 0 number 4: 1957747793
Round 1 number 0: 1804289383  # Same seed → same sequence
Round 1 number 1: 846930886
...
```

## Files Modified/Created

### QEMU
- `qemu-8.2.0/hw/misc/my-rng.c` - Virtual device implementation
- `qemu-8.2.0/hw/misc/Kconfig` - Build config
- `qemu-8.2.0/hw/misc/meson.build` - Build system

### Linux Kernel
- `linux-6.6/drivers/misc/my-rng.c` - Kernel driver
- `linux-6.6/drivers/misc/Makefile` - Build system
- `linux-6.6/init/main.c` - In-kernel test code (optional)

### VM
- `/dev/my_rng_driver` - Device node
- `my-app.c` - User-space test application

## Going Further

Potential improvements:
1. **Kernel Module** - Make driver loadable without kernel rebuild
2. **PCI Discovery** - Auto-detect device instead of hardcoded address
3. **64-bit RNG** - Improve throughput with larger values
4. **Better Algorithm** - Replace stdlib rand() with proper RNG
5. **DMA Support** - Bulk transfer for better performance

## Notes

- Base address (0xfebf1000) may differ - check with `lspci -v`
- ioctl numbers may vary - check with `dmesg | grep my_rng_driver`
- Device node must be recreated after each VM reboot
- Password for VM: (default Alpine root)
