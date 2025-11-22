# QEMU pvpanic device support

PvPanic was introduced in QEMU 1.5. It is a paravirtualized device
that allows a guest operating system to notify the hypervisor
when a Linux kernel panic or fatal error occurs inside the VM.

## Motivation

The motivation for implementing pvpanic support in EDK2 is to
enable firmware to communicate with the hypervisor when critical
system conditions are reported. In virtualised environments, when
guest firmware or the operating system encounters a critical error,
the hypervisor does not have direct access to the event. Thanks
to the implementation of pvpanic in EDK2, firmware can notify
the host of unexpected crashes or fatal errors that occur during
boot. This improves observability, simplifies debugging, and
enables the use of automatic recovery mechanisms for early boot
failures.

## Build the firmware

Support for the qemu pvpanic is a compile time option.  It
is disabled by default and can be enabled using the QEMU_PV_PANIC
option: '-D QEMU_PV_PANIC=TRUE'.

Supported platforms:

 * OvmfPkg/OvmfPkgX64.dsc
 * ArmVirtPkg/ArmVirtQemu.dsc
 * ArmVirtPkg/ArmVirtQemuKernel.dsc

## Using the qemu pvpanic

Pvpanic is implemented as a qemu device and exists in two variants.
The first is `pvpanic` for the x64 platform with an 8-bit hard-coded
I/O port 0x505(`gEfiMdePkgTokenSpaceGuid.PcdPvPanicIoPort`). The
second is `pvpanic-mmio`, used by all other platforms, also
with a 16-bit hard-coded
port 0x9060000(`gEfiMdePkgTokenSpaceGuid.PcdPvPanicMmioAddr`).

Simplest way to use it is this:

```
qemu-system-x86_64 \
    -bios OVMF.fd \
    -device pvpanic \
    $otherargs
```
