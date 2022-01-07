# Introduction

## EDK2 RISC-V Platform Project

### EDK2 RISC-V Design and the Boot Processes
RISC-V edk2 port is designed base on edk2 boot phases and leverage [RISC-V OpenSBI](https://github.com/riscv/opensbi) (which is the implementation of [RISC-V SBI](https://github.com/riscv/riscv-sbi-doc)) as an edk2 library. The design concept is to leverage RISC-V SBI implementation, the basic RISC-V HARTs and the platform initialization. However, it still keeps the edk2 build mechanism and the boot processes. RISC-V OpenSBI is built as
an library and linked with edk2 SEC module. The design diagram and the boot flow is shown in below figure,

#### RISC-V EDK2 Port Design Diagrams
![RISC-V EDK2 Port](https://github.com/tianocore/edk2-platforms/blob/master/Platform/RISC-V/PlatformPkg/Documents/Media/RiscVEdk2BootProcess.svg?raw=true)

#### SEC Phase
As the most of edk2 platforms SEC implementations, RISC-V edk2 port SEC module initiates the fundamental platform
boot environment. RISC-V edk2 SEC module linked with [RiscVOpensbiLib](#riscvopensbilib-library) that pulls in the OpenSBI core source files into the build as a library. SEC module invokes sbi_init() to execute through the OpenSBI
initialization flow. Afterwards, SEC phase hands off to PEI phase via OpenSBI with the ***NextAddress*** and ***NextMode*** are configured.
The entire SEC phase with ***RiscVOpensbiLib*** is executed in the Machine-mode (M-mode) which is the highest
and the mandatory privilege mode of RISC-V HART. The SBI implementation is also executed in the M-mode that
provides the Supervisor Binary Interface for the entities run in the Supervisor-mode (S-mode). The default
privilege mode is configured to S-mode for the next phase after SEC, that says the PEI, DXE and BDS phases are
default executed in S-mode unless the corresponding [PCDs](#risc-v-platform-pcd-settings) are configured
differently from the default settings according to the OEM platform design.

##### RiscVOpensbiLib Library
[Indicated as #1 in the figure](#risc-v-edk2-port-design-diagrams)
> ***RiscVOpensbiLib*** is a edk2 wrapper library of OpenSBI. SEC module is the only consumer of ***RiscVOpensbiLib*** across the entire edk2 boot processes. The sub-module under ***RiscVOpensbiLib*** is updated
to align with OpenSBI project. As mentioned earlier, ***RiscVOpensbiLib*** provides the RISC-V SBI
implementation and initialize the OpenSBI boot flow. SEC module is also linked with below libraries,
- edk2 [OpenSbiPlatformLib](#OpenSbiPlatformLib-library) library that provides the generic RISC-V platform initialization code.
- edk2 [RiscVSpecialPlatformLib](#RiscVSpecialPlatformLib-library) library which is provided by the RISC-V
platform vendor for the platform-specific initialization. The underlying implementation of above two edk2 libraries
are from OpenSBI project. edk2 libraries are introduced as the wrapper libraries that separates and organizes OpenSBI core and platform code based on edk2 framework and the the build mechanism for edk2 RISC-V platforms. ***RiscVOpensbiLib*** library is located under [RISC-V ProcessorPkg](https://github.com/tianocore/edk2-platforms/tree/master/Silicon/RISC-V/ProcessorPkg) while the platform code (e.g. OpenSbiPlatformLib) is located under [RISC-V PlatformPkg](https://github.com/tianocore/edk2-platforms/tree/master/Platform/RISC-V/PlatformPkg).
- edk2 [RiscVSpecialPlatformLib](#riscvspecialplatformlib) library is provided by the platform vendor and located under edk2 RISC-V platform-specific folder.

##### OpenSbiPlatformLib Library
[Indicated as #2 in the figure](#risc-v-edk2-port-design-diagrams)
> ***OpenSbiPlatformLib*** provides the generic RISC-V platform initialization code. Platform vendor can just utilize this library if they don't have additional requirements on the platform initialization.

##### RiscVSpecialPlatformLib Library
[Indicated as #3 in the figure](#risc-v-edk2-port-design-diagrams)
> The major use case of this library is to facilitate the interfaces for platform vendors to provide the special
platform initialization based on the generic platform initialization library.

##### Edk2OpensbiPlatformWrapperLib Library
[Indicated as #4 in the figure](#risc-v-edk2-port-design-diagrams)
> In order to providing the flexibility to edk2 RISC-V firmware solution, ***Edk2OpensbiPlatformWrapperLib*** is the wrapper library of [OpenSbiPlatformLib](#OpenSbiPlatformLib-library) to provide the interfaces for OEM. The ***platform_ops_address***in the generic platform structure is replaced with ***Edk2OpensbiplatformOps*** in SEC
module. The platform function invoked by OpenSBI core is hooked to ***Edk2OpensbiPlatformWrapperLib***. This gives
a change to OEM for implementing platform-specific initialization before and after the generic platform code. OEM
can override this library under their platform folder on demand without touching ***RiscVOpensbiLib*** library
source files and other common source files.

##### Next Phase Address and Privilege Mode
[Indicated as #5 in the figure](#risc-v-edk2-port-design-diagrams)
> Once OpenSBI finishes the boot initialization, it will jump to the next phase with the default privilege set to
S-mode. In order to facilitate the flexibility for a variant of platform demands. EDK2 RISC-V provides the [PCDs](#risc-v-platform-pcd-settings) as the configurable privilege for the next phase. Whether to have PEI or later
phases executed in the default S-mode or to keep the RISC-V edk2 boot phase privilege in M-mode is at platform design discretion. The SEC module sets the next phase address to the PEI Core entry point with a configurable
privilege according to the PCD.

#### PEI Phase
SEC module hands off the boot process to PEI core in the privilege configured by ***PcdPeiCorePrivilegeMode*** PCD *(TODO, currently the privilege is forced to S-mode)*. PEI and later phases are allowed to executed in M-mode
if the platform doesn't require Hypervisor-extended Supervisor mode (HS-mode) for the virtualization. RISC-V edk2 port provides its own instance ***PeiCoreEntryPoint*** library [(indicated as #7 in the figure)](#risc-v-edk2-port-design-diagrams) and linked with [PlatformSecPpiLib](#platformsecppilib-library) in order to support the S-mode PEI phase. PEI core requires [RiscVFirmwareContextLib](#riscvfirmwarecontextlib-library) library to retrieve the information of RISC-V HARTs and platform (e.g. FDT) configurations that built up in SEC phase. ***PeiServicePointer*** is also maintained in the ***RISC-V OpenSBI FirmwareContext*** structure and the pointer is retrieved by [PeiServiceTablePointerOpensbi](#peiservicetablepointeropensbi-library) library.

##### PlatformSecPpiLib Library
[Indicated as #8 in the figure](#risc-v-edk2-port-design-diagrams)

> Some platform has the PEI protocol interface (PPI) prepared in SEC phase and pass the PPI description to PEI phase for the installation. That means the PPI code resides in SEC module and executed in PEI phase. Due to the SEC
(with OpenSBI) is protected by the RISC-V Physical Memory Protection (PMP) through [OpenSBI firmware domain](#edk2-opensbi-firmware-domain), the SEC can be only accessed and executed when RISC-V HART is operated in M-mode. The SEC PPI passed to PEI is not able to be executed by any PEI modules. Thus we have ***PlatformSecPpiLib*** library for the platforms that requires to install the PPI at the early stage of PEI core instead of installing PPI
during PEI dispatcher that maybe too late for some platform use cases. ***PlatformSecPpiLib*** is currently
executed in S-mode because we force to switch RISC-V boot HART to S-mode when SEC hands of boot process to PEI
phase. ***PlatformSecPpiLib*** can also executed in M-mode once we have the full implementation of [***PcdPeiCorePrivilegeMode***.](#risc-v-platform-pcd-settings)

##### RiscVFirmwareContextLib Library
[Indicated as #9 in the figure](#risc-v-edk2-port-design-diagrams)

> The ***OpenSBI FirmwareContext*** is a structure member in sbi_platform, that can carry the firmware
solution-defined information to edk2 boot phases after SEC. edk2 defines its own ***FirmwareContext*** as below in
the current implementation.

    typedef struct {
        UINT64              BootHartId;
        VOID                *PeiServiceTable;      // PEI Service table
        UINT64              FlattenedDeviceTree;   // Pointer to Flattened Device tree
        UINT64              SecPeiHandOffData;     // This is EFI_SEC_PEI_HAND_OFF passed to PEI Core.
        EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC  *HartSpecific[RISC_V_MAX_HART_SUPPORTED];
    } EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT;

> ***RiscVFirmwareContextLib*** library is used by PEI module for obtaining the ***FirmwareContext*** pointer.

##### PeiServiceTablePointerOpensbi Library
[Indicated as #10 in the figure](#risc-v-edk2-port-design-diagrams)

> ***PeiServiceTablePointerOpensbi*** is the library that provides Get/Set PeiServiceTablePointer. ***RiscVFirmwareContextLib*** is the underlying library for the operations on PEI service table pointer.

##### PEI OpenSBI PPI
[Indicated as #11 in the figure](#risc-v-edk2-port-design-diagrams)

> edk2 PEI OpenSBI PPI *(TODO)* provides the unified interface for all PEI drivers to invoke SBI services.

#### DXE Phase
DXE IPL PEI module hands off the boot process to DXE Core in the privilege configured by PcdDxeCorePrivilegeMode PCD *(TODO, currently is not implemented yet)*. edk2 DXE OpenSBI protocol *(TODO, indicated as #12 in the figure)* provides the unified interface for all DXE drivers to invoke SBI services.

#### BDS Phase
The implementation of RISC-V edk2 port in BDS phase is the same as it is in DXE phase which is executed in the
privilege configured by PcdDxeCorePrivilegeMode PCD *(TODO, currently the privilege is forced to S-mode)*. The
OpenSBI is also provided through edk2 DXE OpenSBI Protocol*(TODO, indicated as #12 in the figure)*. However, BDS must transits the privilege mode to S-mode before it handing off the boot process to S-mode OS, OS boot loader or EFI application.

#### EDK2 OpenSBI Firmware Domain

![RISC-V EDK2 FW Domain](https://github.com/tianocore/edk2-platforms/blob/master/Platform/RISC-V/PlatformPkg/Documents/Media/RiscVEdk2FwDomain.svg?raw=true)

OpenSBI implements the firmware domain mechanism to protect the root firmware (which is the OpenSBI itself) as the M-mode only access and execute region. RISC-V edk2 port configures the root firmware domain via [PCDs](#risc-v-platform-pcd-settings) to protect SEC firmware volume, memory and OpenSBI stuff. The firmware region (non-root firmware) that accommodates PEI and DXE phase FV regions, while EFI variable region is reported as a separate firmware region as it shows in above figure.

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

## RISC-V Platform PCD settings
### EDK2 Firmware Volume Settings
EDK2 Firmware volume related PCDs which is declared in platform FDF file.

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
|PcdPlatformFlashNvStorageFtwWorkingBase| The base address of EFI variable fault tolerance workspace (FTW) within firmware device|
|PcdPlatformFlashNvStorageFtwSpareBase| The base address of EFI variable spare FTW within firmware device|

### RISC-V Physical Memory Protection (PMP) Region Settings
Below PCDs could be set in platform FDF file.

| **PCD name** |**Usage**|**Access Permission in M-mode**|**Access Permission in S-mode**|
|--------------|---------|---------|---------|
|PcdRootFirmwareDomainBaseAddress| The starting address of root firmware domain protected by PMP|Full access|No Access|
|PcdRootFirmwareDomainSize| The size of root firmware domain|-|-|
|PcdFirmwareDomainBaseAddress| The starting address of firmware domain that can be accessed and executed in S-mode|Full access|Readable and Executable|
|PcdFirmwareDomainSize| The size of firmware domain|-|-|
|PcdVariableFirmwareRegionBaseAddress| The starting address of EFI variable region that can be accessed in S-mode|Full access|Readable and Writable|
|PcdVariableFirmwareRegionSize| The size of EFI variable firmware region|-|-|

### RISC-V Processor HART Settings

| **PCD name** |**Usage**|
|--------------|---------|
|PcdHartCount| Number of RISC-V HARTs, the value is processor-implementation specific|
|PcdBootHartId| The ID of RISC-V HART to execute main firmware code and boot system to OS|
|PcdBootableHartNumber|The bootable HART number, which is incorporate with RISC-V OpenSBI platform hart_index2id value|
|PcdBootableHartIndexToId| if PcdBootableHartNumber == 0, hart_index2id is built from Device Tree, otherwise this is an array of HART index to HART ID|

### RISC-V OpenSBI Settings

| **PCD name** |**Usage**|
|--------------|---------|
|PcdScratchRamBase| The base address of RISC-V OpenSBI scratch buffer for all RISC-V HARTs|
|PcdScratchRamSize| The total size of RISC-V OpenSBI scratch buffer for all RISC-V HARTs|
|PcdOpenSbiStackSize| The size of initial stack of each RISC-V HART for booting system use RISC-V OpenSBI|
|PcdTemporaryRamBase| The base address of temporary memory for PEI phase|
|PcdTemporaryRamSize| The temporary memory size for PEI phase|
|PcdPeiCorePrivilegeMode|The target RISC-V privilege mode for edk2 PEI phase|
|PcdDxeCorePrivilegeMode (TODO)|The target RISC-V privilege mode for edk2 DXE phase|

## Supported Operating Systems
Currently support boot to EFI Shell and Linux kernel.
Refer to below link for more information,
https://github.com/riscv/riscv-uefi-edk2-docs

## Known Issues and Limitations
Only RISC-V RV64 is verified on edk2.

