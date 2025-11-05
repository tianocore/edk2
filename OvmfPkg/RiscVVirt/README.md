# Support for RISC-V QEMU virt platform

## Overview
RISC-V QEMU 'virt' is a generic platform which does not correspond to any real
hardware.

EDK2 for RISC-V virt platform is a payload (S-mode) for the previous stage M-mode
firmware like OpenSBI. It follows PEI less design.

The minimum QEMU version required is
**[8.1](https://wiki.qemu.org/Planning/8.1)** or with commit
[7efd65423a](https://github.com/qemu/qemu/commit/7efd65423ab22e6f5890ca08ae40c84d6660242f)
which supports separate pflash devices for EDK2 code and variable storage.

## Get edk2 sources

    git clone --recurse-submodule git@github.com:tianocore/edk2.git

## Build

### Using GCC toolchain
**Prerequisite**: RISC-V GNU compiler toolchain should be installed.

    export WORKSPACE=`pwd`
    export GCC5_RISCV64_PREFIX=riscv64-linux-gnu-
    export PACKAGES_PATH=$WORKSPACE/edk2
    export EDK_TOOLS_PATH=$WORKSPACE/edk2/BaseTools
    source edk2/edksetup.sh --reconfig
    make -C edk2/BaseTools
    source edk2/edksetup.sh BaseTools
    build -a RISCV64 --buildtarget RELEASE -p OvmfPkg/RiscVVirt/RiscVVirtQemu.dsc -t GCC5

### Using CLANGDWARF toolchain (clang + lld)
**Prerequisite**: LLVM toolchain with clang and lld should be installed.

    export WORKSPACE=`pwd`
    export CLANGDWARF_BIN=/usr/bin/
    export PACKAGES_PATH=$WORKSPACE/edk2
    export EDK_TOOLS_PATH=$WORKSPACE/edk2/BaseTools
    source edk2/edksetup.sh --reconfig
    make -C edk2/BaseTools
    source edk2/edksetup.sh BaseTools
    build -a RISCV64 --buildtarget RELEASE -p OvmfPkg/RiscVVirt/RiscVVirtQemu.dsc -t CLANGDWARF

After a successful build, two files namely **RISCV_VIRT_CODE.fd** and **RISCV_VIRT_VARS.fd** are created.

## Test
Below example shows how to boot openSUSE Tumbleweed E20.

1) RISC-V QEMU pflash devices should be of of size 32MiB.

    `truncate -s 32M RISCV_VIRT_CODE.fd`

    `truncate -s 32M RISCV_VIRT_VARS.fd`

2) Running QEMU

        qemu-system-riscv64 \
        -M virt,pflash0=pflash0,pflash1=pflash1,acpi=off \
        -m 4096 -smp 2 \
        -serial mon:stdio \
        -device virtio-gpu-pci -full-screen \
        -device qemu-xhci \
        -device usb-kbd \
        -device virtio-rng-pci \
        -blockdev node-name=pflash0,driver=file,read-only=on,filename=RISCV_VIRT_CODE.fd \
        -blockdev node-name=pflash1,driver=file,filename=RISCV_VIRT_VARS.fd \
        -netdev user,id=net0 \
        -device virtio-net-pci,netdev=net0 \
        -device virtio-blk-device,drive=hd0 \
        -drive file=openSUSE-Tumbleweed-RISC-V-E20-efi.riscv64.raw,format=raw,id=hd0

    Note: the `acpi=off` machine property is specified because Linux guest
    support for ACPI (that is, the ACPI consumer side) is a work in progress.
    Currently, `acpi=off` is recommended unless you are developing ACPI support
    yourself.

3) Running QEMU with direct kernel boot

    The following example boots the same guest, but loads the kernel image and
    the initial RAM disk (which were extracted from
    `openSUSE-Tumbleweed-RISC-V-E20-efi.riscv64.raw`) from the host filesystem.
    It also sets the guest kernel command line on the QEMU command line.

        CMDLINE=(root=UUID=76d9b92d-09e9-4df0-8262-c1a7a466f2bc
                 systemd.show_status=1
                 ignore_loglevel
                 console=ttyS0
                 earlycon=uart8250,mmio,0x10000000)

        qemu-system-riscv64 \
        -M virt,pflash0=pflash0,pflash1=pflash1,acpi=off \
        -m 4096 -smp 2 \
        -serial mon:stdio \
        -device virtio-gpu-pci -full-screen \
        -device qemu-xhci \
        -device usb-kbd \
        -device virtio-rng-pci \
        -blockdev node-name=pflash0,driver=file,read-only=on,filename=RISCV_VIRT_CODE.fd \
        -blockdev node-name=pflash1,driver=file,filename=RISCV_VIRT_VARS.fd \
        -netdev user,id=net0 \
        -device virtio-net-pci,netdev=net0 \
        -device virtio-blk-device,drive=hd0 \
        -drive file=openSUSE-Tumbleweed-RISC-V-E20-efi.riscv64.raw,format=raw,id=hd0 \
        -kernel Image-6.5.2-1-default \
        -initrd initrd-6.5.2-1-default \
        -append "${CMDLINE[*]}"

## Test with your own OpenSBI binary
Using the above QEMU command lines, **RISCV_VIRT_CODE.fd** is launched by the
OpenSBI binary that is bundled with QEMU. You can build your own OpenSBI binary
as well:

    OPENSBI_DIR=...
    git clone https://github.com/riscv/opensbi.git $OPENSBI_DIR
    make -C $OPENSBI_DIR \
        -j $(getconf _NPROCESSORS_ONLN) \
        CROSS_COMPILE=riscv64-linux-gnu- \
        PLATFORM=generic

then specify that binary for QEMU, with the following additional command line
option:

    -bios $OPENSBI_DIR/build/platform/generic/firmware/fw_dynamic.bin

Note that the above only makes a difference with software emulation (which you
can force with `-M accel=tcg`). With hardware virtualization (`-M accel=kvm`),
KVM services the SBI (Supervisor Binary Interface) calls internally, therefore
any OpenSBI binary specified with `-bios` is rejected.
