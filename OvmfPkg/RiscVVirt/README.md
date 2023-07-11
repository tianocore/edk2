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
