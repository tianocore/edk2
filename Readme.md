# Introduction

**DynamicTablesPkg** currently supports Arm architecture, and we welcome the
adoption by other architectures.

This branch will be used to:
 - Reorganise the code to streamline adoption by other architectures.
 - Introduce Dynamic Tables support for RISC-V architecture
 - Integrate Dynamic SMBIOS support
   (<https://edk2.groups.io/g/devel/message/107254>)

## Goals
 - Streamline adoption by other architectures.
 - Minimise the impact of migration for existing platforms
 - Reuse common code
 - Maintain flexibility across architectural components

# Dynamic Tables Framework

The dynamic tables framework is designed to generate standardised
firmware tables that describe the hardware information at
run-time. A goal of standardised firmware is to have a common
firmware for a platform capable of booting both Windows and Linux
operating systems.

Traditionally the firmware tables are handcrafted using ACPI
Source Language (ASL), Table Definition Language (TDL) and
C-code. This approach can be error prone and involves time
consuming debugging. In addition, it may be desirable to configure
platform hardware at runtime such as: configuring the number of
cores available for use by the OS, or turning SoC features ON or
OFF.

The dynamic tables framework simplifies this by providing a set
of standard table generators, that are implemented as libraries.
These generators query a platform specific component, the
'Configuration Manager', to collate the information required
for generating the tables at run-time.

The framework also provides the ability to implement custom/OEM
generators; thereby facilitating support for custom tables. The
custom generators can also utilize the existing standard generators
and override any functionality if needed.

The framework currently implements a set of standard ACPI table
generators for Arm architecture, these include both data tables
and ASL tables. The ASL generation includes support for both
fixup, where a template AML code is patched, and additionally
provides an API to parse, search, generate and serialise the
AML bytecode.

Although, the set of standard generators implement the functionality
required for Arm architecture; the framework is extensible, and
support for other architectures can be added.

## Branch Owners

   - Sami Mujawar <sami.mujawar@arm.com>
   - Pierre Gondois <pierre.gondois@arm.com>

## Feature Summary

### Dynamic Tables framework supports
 - ACPI data tables
 - AML tables
   * AML Template Fixup
   * AML Code Generation

The framework currently supports the following table generators for Arm:
   * DBG2 - Debug Port Table 2
   * DSDT - Differentiated system description table. This is essentially
            a RAW table generator.
   * FADT - Fixed ACPI Description Table
   * GTDT - Generic Timer Description Table
   * IORT - IO Remapping Table
   * MADT - Multiple APIC Description Table
   * MCFG - PCI Express memory mapped configuration space base address
            Description Table
   * SPCR - Serial Port Console Redirection Table
   * SSDT - Secondary System Description Table. This is essentially
            a RAW table generator.
   * PCCT - Platform Communications Channel Table.
   * PPTT - Processor Properties Topology Table.
   * SRAT - System Resource Affinity Table.
   * SSDT-CMN600 - SSDT Table for Arm CoreLink CMN-600 Coherent Mesh Network.
   * SSDT-Cpu-Topology - SSDT Table for describing the CPU hierarchy.
   * SSDT-PCIe - SSDT Table describing the PCIe.
   * SSDT-Serial-Port - SSDT Table describing the Serial ports.

## SMBIOS Support
 - A SMBIOS String table helper library has been provided.
 - Initial patches to add SMBIOS support are available at:
   * SMBIOS Dispatcher (<https://edk2.groups.io/g/devel/message/100834>)
   * SMBIOS Table generation (<https://edk2.groups.io/g/devel/message/107254>).

# Roadmap

1.  See [Related Modules](#related-modules) section below for details of
    staging repositories and branches to be used for prototyping.
2.  The design aspects and changes shall be discussed on the mailing list
    with patches to support the details.
3.  A new section in DynamicTablesPkg\Readme.md shall be added to reflect
    the design updates, e.g. changes to CM Objects, Namespace definitions, etc.
4.  The design changes should typically be supported by patches for the
    DynamicTables core framework and demonstrate the impact on the platform
    code by typically providing patches for at least one existing
    platform (possibly edk2-platforms/Platform/ARM/[Juno | FVP]).
5.  The design changes should be small and typically be reflected in separate
    patch series.
6.  The first phase would be to partition the codebase into common code vs
    architectural specific code. This would involve moving files and
    reflecting the associated changes such that the build does not break.
7.  Define a new namespace *ArchCommon* for the common architectural components.
8.  Identify the CM_ARM_OBJECTs that can be moved to the *ArchCommon* namespace.
    As part of this identify if any object needs to be dropped,
    e.g. EArmObjReserved29
9.  Identify overlap of SMBIOS objects with existing CM Objects.
10. Submit patches to move CM objects from Arm Namespace to *ArchCommon*
    Namespace. Ideally one object (and any dependencies) should be moved
    at a time.
11. Submit patches to migrate upstream platforms that use DynamicTablesPkg
12. Define a new namespace for RISC-V specific objects
13. Submit patches for enabling RISC-V
14. In the next phase support for Dynamic SMBIOS can be enabled.

## Note:
- Periodically rebase with edk2 & edk2-platforms master branch to sync
   with latest changes.
- Merge *reorg* updates after point 11 above to edk2 & edk2-platforms master
  branch.
- Similarly, the RISC-V support can be merged after point 13.

# Related Modules

## edk2-staging
The *dynamictables-reorg* branch in the **edk2-staging** repository
contains the updates to streamline the adoption of Dynamic Tables
Framework by other architectures.

## edk2-platforms
The *devel-dynamictables-reorg* branch in the **edk2-platforms** repository
contains the platform specific changes.

# Related Links

Source Code Repositories for staging:<BR>

### 1. edk2 codebase <BR>
   Repo: <https://github.com/tianocore/edk2-staging.git> <BR>
   Branch: *dynamictables-reorg*

### 2. edk2-platforms codebase <BR>
  Repo: <https://github.com/tianocore/edk2-platforms.git> <BR>
  Branch: *devel-dynamictables-reorg*

# Impacted Platforms

| Platform            | Location                                      | Description                                    | Migration Status | Known Issues |
| :----               | :-----                                        | :----                                          | :---             | :---         |
| Arm Virt Kvmtool    | edk2/ArmVirtPkg/KvmtoolCfgMgrDxe              | Arm Kvmtool Guest firmware                     |                  |              |
| FVP                 | edk2-platforms/Platform/ARM/VExpressPkg       | Arm Fixed Virtual Platform                     |                  |              |
| Juno                | edk2-platforms/Platform/ARM/JunoPkg           | Arm Juno Software Development Platform         |                  |              |
| N1SDP               | edk2-platforms/Platform/ARM/N1Sdp             | Arm Neoverse N1 Software Development Platform  |                  |              |
| Morello FVP         | edk2-platforms/Platform/ARM/Morello           | Arm Morello Fixed Virtual Platform             |                  |              |
| Morello             | edk2-platforms/Platform/ARM/Morello           | Arm Morello Software Development Platform      |                  |              |
| LX2160A             | edk2-platforms/Silicon/NXP/LX2160A            | NXP LX2160A                                    |                  |              |


# Prerequisites

Ensure that the latest ACPICA iASL compiler is used for building *Dynamic Tables Framework*. <BR>
*Dynamic Tables Framework* has been tested using the following iASL compiler version: <BR>
ACPICA iASL compiler [Version 20230628](https://www.acpica.org/node/183), dated 28 June, 2023.

# Build Instructions

1. Set path for the iASL compiler.

2. Set PACKAGES_PATH to point to the locations of the following repositories:

Example:

> set PACKAGES_PATH=%CD%\edk2;%CD%\edk2-platforms;%CD%\edk2-non-osi

  or

> export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-platforms:$PWD/edk2-non-osi

3. To enable Dynamic tables framework the *'DYNAMIC_TABLES_FRAMEWORK'*
option must be defined for some platforms that support both traditional
ACPI tables as well as Dynamic Table generation. This can be passed as
a command line parameter to the edk2 build system.

Example:

Juno supports both traditional and dynamic ACPI tables.
>build -a AARCH64 -p Platform\ARM\JunoPkg\ArmJuno.dsc
   -t GCC5 **-D DYNAMIC_TABLES_FRAMEWORK**

or
FVP only supports dynamic ACPI table generation, so the preprocessor
flag is not required for the build.
>build -a AARCH64 -p Platform\ARM\VExpressPkg\ArmVExpress-FVP-AArch64.dsc
   -t GCC5

# Documentation

The documentation for the Dynamic Tables Framework is available at
DynamicTablesPkg\Readme.md. Additionally, Doxygen style documentation
is used in the code.

# Guidelines for submitting patches

1. Follow the standard edk2 coding guidelines for preparing patches. <BR>
   The edk2-staging guidelines can be found at
   <https://github.com/tianocore/edk2-staging>

2. To submit a patch for edk2-staging repo include the branch name in
   the subject line of the commit message. <BR>
   e.g. **[staging/dynamictables-reorg PATCH v<*n*> <x/y>]: Package/Module: Subject**

3. To submit a patch for edk2-platforms staging repo include the branch
   name in the subject line of the commit message. <BR>
   e.g. **[platforms/devel-dynamictables-reorg PATCH v<*n*> <x/y>]: Package/Module: Subject**


# Stakeholders/Distribution List

  Please send a patch if you wished to be added/removed from the distribution
  list below.

 - Sami Mujawar <sami.mujawar@arm.com>
 - Pierre Gondois <pierre.gondois@arm.com>
 - Yeo Reum Yun <YeoReum.Yun@arm.com>

# Miscellaneous

