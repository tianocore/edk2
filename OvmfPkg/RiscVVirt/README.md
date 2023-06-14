# Support for RISC-V qemu virt platform

## Overview
RISC-V qemu 'virt' is a generic platform which does not correspond to any real
hardware.

EDK2 for RISC-V virt platform is a payload (S-mode) for a previous stage M-mode
firmware like opensbi. It follows PEI less design.

## Build
    export WORKSPACE=`pwd`
    export GCC5_RISCV64_PREFIX=riscv64-linux-gnu-
    export PACKAGES_PATH=$WORKSPACE/edk2
    export EDK_TOOLS_PATH=$WORKSPACE/edk2/BaseTools
    source edk2/edksetup.sh
    make -C edk2/BaseTools
    source edk2/edksetup.sh BaseTools
    build -a RISCV64 --buildtarget RELEASE -p OvmfPkg/RiscVVirt/RiscVVirtQemu.dsc -t GCC5

## Test
1) RISC-V qemu pflash devices should be of of size 32MiB.

    `truncate -s 32M Build/RiscVVirtQemu/RELEASE_GCC5/FV/RISCV_VIRT_CODE.fd`

    `truncate -s 32M Build/RiscVVirtQemu/RELEASE_GCC5/FV/RISCV_VIRT_VARS.fd`

2) Run qemu

        qemu-system-riscv64 \
        -accel tcg -m 4096 -smp 2 \
        -serial mon:stdio \
        -device virtio-gpu-pci -full-screen \
        -device qemu-xhci \
        -device usb-kbd \
        -blockdev node-name=pflash0,driver=file,read-only=on,filename=RISCV_VIRT_CODE.fd \
        -blockdev node-name=pflash1,driver=file,filename=RISCV_VIRT_VARS.fd \
        -M virt,pflash0=pflash0,pflash1=pflash1,acpi=off \
        -kernel linux/arch/riscv/boot/Image \
        -initrd buildroot/output/images/rootfs.cpio \
        -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
        -append "root=/dev/ram rw console=ttyS0 earlycon=uart8250,mmio,0x10000000"
