# ChainloadApp

A UEFI application that chainloads a full UefiPayloadPkg firmware volume
from a running UEFI environment.

## Overview

ChainloadApp embeds `UEFIPAYLOAD.fd` and transfers control to it after
calling `ExitBootServices()`. This lets a fresh, self-contained UEFI
environment take over from a platform firmware image that cannot be
modified, e.g. for guest firmware development inside a VM whose outer
firmware is fixed.

## How it works

1. **Payload embedding.** `UEFIPAYLOAD.fd` is embedded as a C array in
   `EmbeddedPayload.h`, generated at build time by
   `ChainloadApp/GenPayloadHdr.py` from the compiled FV.

2. **HOB construction.** ChainloadApp builds the Hand-Off Blocks the
   payload's entry point expects:
   - `gUniversalPayloadAcpiTableGuid` (RSDP from the UEFI configuration table)
   - `gUniversalPayloadSmbiosTableGuid` (SMBIOS entry point, if present)
   - `gUniversalPayloadExtraDataGuid` (payload FV location)
   - `gUefiSerialPortInfoGuid` and `gUniversalPayloadSerialPortInfoGuid`
     (serial console configuration, derived from the ACPI SPCR table)
   - Memory map records converted from the UEFI memory map, with GCD
     MMIO regions surfaced as Reserved with `MEM_MAP_FLAG_MMIO` set

   A `gUniversalPayloadPciRootBridgeInfoGuid` HOB is *not* emitted; the
   payload's `PciHostBridgeLib` derives roots from ACPI MCFG.

3. **Control transfer.** After `ExitBootServices()`, ChainloadApp jumps to
   the payload's `_ModuleEntryPoint` with the HOB list address.

   On AArch64, ChainloadApp builds its own translation tables (in
   `EfiReservedMemoryType` pages) via `ArmConfigureMmu()` while boot
   services are still available, then installs them after
   `ExitBootServices()` and branches with the MMU and caches enabled.
   No data-cache maintenance is needed; the FV is invalidated from the
   instruction cache for I/D coherency. The payload's `HandOffToDxeCore()`
   adopts the live translation; CpuDxe later edits it in place.

## Supported architectures

- **X64**
- **AArch64**

## Building

`BuildChainloadEmbedded.sh` runs two build passes. The first builds
`UEFIPAYLOAD.fd`; the header generator then turns that FD into
`EmbeddedPayload.h`, and the second pass rebuilds ChainloadApp against
it. Both passes share `-D CHAINLOAD_DEFAULTS=TRUE`.

```bash
cd /path/to/edk2
source edksetup.sh
./UefiPayloadPkg/BuildChainloadEmbedded.sh
```

Output:
`Build/UefiPayloadPkgLegacy${ARCH}/${BUILD_TARGET}_${TOOL_CHAIN_TAG}/${ARCH}/ChainloadApp.efi`
— with the defaults below, that is
`Build/UefiPayloadPkgLegacyX64/RELEASE_GCC5/X64/ChainloadApp.efi`.

Environment variables:

| Variable | Default | Notes |
|---|---|---|
| `ARCH` | `X64` | `AARCH64` for arm64 |
| `BUILD_TARGET` | `RELEASE` | `DEBUG` / `NOOPT` |
| `TOOL_CHAIN_TAG` | `GCC5` | |
| `GCC5_AARCH64_PREFIX` | `aarch64-unknown-linux-gnu-` | AArch64 cross-toolchain prefix |

Example (AArch64, DEBUG):

```bash
ARCH=AARCH64 BUILD_TARGET=DEBUG \
GCC5_AARCH64_PREFIX=aarch64-linux-gnu- \
./UefiPayloadPkg/BuildChainloadEmbedded.sh
```

## Testing with QEMU

### X64 (OVMF)

```bash
qemu-system-x86_64 -m 1G -nographic -enable-kvm -bios OVMF_CODE.fd \
    -netdev user,tftp=Build/UefiPayloadPkgLegacyX64/RELEASE_GCC5/X64/,bootfile=ChainloadApp.efi,id=n \
    -device virtio-net-pci,netdev=n
```

### AArch64 (`-M virt`)

```bash
dd if=/dev/zero of=disk.img bs=1M count=64 && mkfs.vfat disk.img
mmd -i disk.img ::/EFI ::/EFI/BOOT
mcopy -i disk.img Build/UefiPayloadPkgLegacyAARCH64/RELEASE_GCC5/AARCH64/ChainloadApp.efi ::/EFI/BOOT/BOOTAA64.EFI

qemu-system-aarch64 -M virt -cpu cortex-a72 -m 1G -nographic \
    -bios QEMU_EFI.fd -drive file=disk.img,format=raw,if=virtio
```

## Files

- `ChainloadApp.c` / `ChainloadApp.inf` — the application
- `AArch64/PayloadEntry.S`, `X64/PayloadEntry.nasm` — handoff trampolines
- `GenPayloadHdr.py` — FD-to-C-array generator
- `EmbeddedPayloadStub.h` — placeholder header for the first build pass
- `../BuildChainloadEmbedded.sh` — two-pass build script

---

Copyright (c) 2026, Amazon.com, Inc. or its affiliates. All Rights Reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
