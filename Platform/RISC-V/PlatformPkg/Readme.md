# Introduction

## EDK2 RISC-V Platform Project

### EDK2 Build Architecture for RISC-V
The edk2 build architecture which is supported and verified on edk2 code base for
RISC-V platforms is `RISCV64`.

### Toolchain for RISC-V
The toolchain is on RISC-V GitHub (https://github.com/riscv/riscv-gnu-toolchain)
for building edk2 RISC-V binary. The corresponding edk2 Toolchain tag for building
RISC-V platform is "GCC5" declared in `tools_def.txt`.

### Packages
There are two packages to support RISC-V edk2 platforms:
- `Silicon/RISC-V/ProcessorPkg/RiscVProcessorPkg.dec`
- `Platform/RISC-V/PlatformPkg/RiscVPlatformPkg.dec`

`RiscVPlatformPkg` currently provides the generic SEC driver for all RISC-V platforms,
and some platform level libraries.
`RiscVProcessorPkg` currently provides RISC-V processor related libraries, PEI modules,
DXE drivers and industrial standard header files.

## EDK2 RISC-V Platform Package
RISC-V platform package provides the common modules for RISC-V platforms. RISC-V
platform vendors could include RiscPlatformPkg.dec to use the common drivers, libraries,
definitions, PCDs and etc. for the RISC-V platforms development.

### Download the Source Code ###
```
git clone https://github.com/tianocore/edk2.git
git clone https://github.com/tianocore/edk2-platforms.git

```

You have to follow the build steps for
EDK2 (https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II)
and additionally set an environment variable to point to your RISC-V toolchain binaries
for building RISC-V platforms,
```
# e.g. If the toolchain binaries are under /riscv-gnu-toolchain-binaries/bin
export GCC5_RISCV64_PREFIX=/riscv-gnu-toolchain-binaries/bin/riscv64-unknown-elf-
```

Then you can build the edk2 firmware image for RISC-V platforms.

```
# e.g. For building SiFive Hifive Unleashed platform:
build -a RISCV64 -t GCC5 -p Platform/SiFive/U5SeriesPkg/FreedomU540HiFiveUnleashedBoard/U540.dsc
```

## RISC-V OpenSBI Library
RISC-V [OpenSBI](https://github.com/riscv/opensbi) is the implementation of
[RISC-V SBI (Supervisor Binary Interface) specification](https://github.com/riscv/riscv-sbi-doc).
For EDK2 UEFI firmware solution, RISC-V OpenSBI is integrated as a library
[(submoudule)](Silicon/RISC-V/ProcessorPkg/Library/RiscVOpensbiLib/opensbi) in EDK2
RISC-V Processor Package. The RISC-V OpenSBI library is built in SEC driver without
any modifications and provides the interfaces for supervisor mode execution environment
to execute privileged operations.

## RISC-V Platform PCD settings
### EDK2 Firmware Volume Settings
EDK2 Firmware volume related PCDs which declared in platform FDF file.

| **PCD name** |**Usage**|
|--------------|---------|
|PcdRiscVSecFvBase| The base address of SEC Firmware Volume|
|PcdRiscVSecFvSize| The size of SEC Firmware Volume|
|PcdRiscVPeiFvBase| The base address of PEI Firmware Volume|
|PcdRiscVPeiFvSize| The size of PEI Firmware Volume|
|PcdRiscVDxeFvBase| The base address of DXE Firmware Volume|
|PcdRiscVDxeFvSize| The size of DXE Firmware Volume|

### EDK2 EFI Variable Region Settings
The PCD settings regard to EFI Variable

| **PCD name** |**Usage**|
|--------------|---------|
|PcdVariableFdBaseAddress| The EFI variable firmware device base address|
|PcdVariableFdSize| The EFI variable firmware device size|
|PcdVariableFdBlockSize| The block size of EFI variable firmware device|
|PcdPlatformFlashNvStorageVariableBase| EFI variable base address within firmware device|
|PcdPlatformFlashNvStorageFtwWorkingBase| The base address of EFI variable fault tolerance worksapce (FTW) within firmware device|
|PcdPlatformFlashNvStorageFtwSpareBase| The base address of EFI variable spare FTW within firmware device|

### RISC-V Physical Memory Protection (PMP) Region Settings
Below PCDs could be set in platform FDF file.

| **PCD name** |**Usage**|
|--------------|---------|
|PcdFwStartAddress| The starting address of firmware region to protected by PMP|
|PcdFwEndAddress| The ending address of firmware region to protected by PMP|

### RISC-V Processor HART Settings

| **PCD name** |**Usage**|
|--------------|---------|
|PcdHartCount| Number of RISC-V HARTs, the value is processor-implementation specific|
|PcdBootHartId| The ID of RISC-V HART to execute main fimrware code and boot system to OS|
|PcdBootableHartNumber|The bootable HART number, which is incorporate with RISC-V OpenSBI platform hart_index2id value|

### RISC-V OpenSBI Settings

| **PCD name** |**Usage**|
|--------------|---------|
|PcdScratchRamBase| The base address of RISC-V OpenSBI scratch buffer for all RISC-V HARTs|
|PcdScratchRamSize| The total size of RISC-V OpenSBI scratch buffer for all RISC-V HARTs|
|PcdOpenSbiStackSize| The size of initial stack of each RISC-V HART for booting system use RISC-V OpenSBI|
|PcdTemporaryRamBase| The base address of temporary memory for PEI phase|
|PcdTemporaryRamSize| The temporary memory size for PEI phase|
|PcdPeiCorePrivilegeMode|The target RISC-V privilege mode for edk2 PEI phase|

## Supported Operating Systems
Currently support boot to EFI Shell and Linux kernel.
Refer to below link for more information,
https://github.com/riscv/riscv-uefi-edk2-docs

## Known Issues and Limitations
Only RISC-V RV64 is verified on edk2.

