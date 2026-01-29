# LoongArch QEMU virt platform

## Overview

  LoongArch QEMU virt is a generic platform that does not require any actual hardware.
  The minimum required QEMU version is [8.1](https://gitlab.com/qemu-project/qemu/-/tags), the minimum required GCC version is [GCC13](https://gcc.gnu.org/gcc-13/), the minimum required Binutils version is [2.40](https://ftp.gnu.org/gnu/binutils/).

## Prepare (X86 Linux Environment)

### Fedora39 and higher
Install LoongArch64 cross compiler, LoongArch system QEMU.

    yum install gcc-loongarch64-linux-gnu
    yum install qemu-system-loongarch64

### Others X86 OS ENV
#### Configure cross-tools

**Download:**

    wget https://github.com/loongson/build-tools/releases/download/2023.08.08/x86_64-cross-tools-loongarch64-binutils_2.41-gcc_13.2.0.tar.xz

**Configure the cross-tools environment:**

    mkdir /opt/loongarch64_cross-toolchain/
    tar -vxf x86_64-cross-tools-loongarch64-binutils_2.41-gcc_13.2.0.tar.xz -C /opt/loongarch64_cross-toolchain/
    export PATH=/opt/loongarch64_cross-toolchain/cross-tools/bin:$PATH

Note: Please obtain [the latest cross-compilation](https://github.com/loongson/build-tools) toolchains.

#### Build QEMU

    git clone https://gitlab.com/qemu-project/qemu.git

Note: Please refer to QEMU compilation rules, located in qemu/doc/system/loongarch/virt.rst.


## Build LoongArch QEMU virtual machine firmware
#### Get edk2 resouces

    git clone --recurse-submodule https://github.com/tianocore/edk2.git

#### Building LoongArch QEMU virt FW with GCC

    export WORKSPACE=`pwd`
    export GCC5_LOONGARCH64_PREFIX=loongarch64-unknown-linux-gnu-
    export PACKAGES_PATH=$WORKSPACE/edk2
    export EDK_TOOLS_PATH=$WORKSPACE/edk2/BaseTools
    source edk2/edksetup.sh --reconfig
    make -C edk2/BaseTools
    source edk2/edksetup.sh BaseTools
    build -b RELEASE -t GCC5 -a LOONGARCH64 -p OvmfPkg/LoongArchVirt/LoongArchVirtQemu.dsc

## Test LoongArch QEMU virtual machine firmware
    qemu-system-loongarch64 \
    -m 4G \
    -M virt \
    -smp 2 \
    -cpu la464 \
    -bios Build/LoongArchVirtQemu/RELEASE_GCC5/FV/QEMU_EFI.fd \
    -serial stdio

## Test LoongArch QEMU virtual machine OS

* Download ArchLinux QCOW [images](https://mirrors.pku.edu.cn/loongarch/archlinux/images) for LoongArch.

* [Running LoongArch ArchLinux on virtual machine](https://mirrors.pku.edu.cn/loongarch/archlinux/images/README.html).

* Download openEuler 22.03 LTS QCOW [images](https://mirrors.nju.edu.cn/openeuler/openEuler-22.03-LTS/virtual_machine_img/loongarch64/openEuler-22.03-LTS-LoongArch-loongarch64.qcow2.xz) for LoongArch.
