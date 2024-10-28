# Dynamic Tables Framework

Dynamic Tables Framework provides mechanisms to reduce the amount
of effort required in porting firmware to new platforms. The aim is
to provide an implementation capable of generating the firmware
tables from an external source.  This is potentially a management
node, either local or remote, or, where suitable, a file that might
be generated from the system construction.  This initial release
does not fully implement that - the configuration is held in local
UEFI modules.

# Feature Summary

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
generators for ARM architecture, that can generate Server Base Boot
Requirement (SBBR) compliant tables. Although, the set of standard
generators implement the functionality required for ARM architecture;
the framework is extensible, and support for other architectures can
be added easily.

The framework currently supports the following table generators for ARM:
* DBG2 - Debug Port Table 2
* DSDT - Differentiated system description table. This is essentially
         a RAW table generator.
* FADT - Fixed ACPI Description Table
* GTDT - Generic Timer Description Table
* IORT - IO Remapping Table
* MADT - Multiple APIC Description Table
* MCFG - PCI Express memory mapped configuration space base address
         Description Table
* PCCT - Platform Communications Channel Table
* PPTT - Processor Properties Topology Table
* SPCR - Serial Port Console Redirection Table
* SRAT - System Resource Affinity Table
* SSDT - Secondary System Description Table. This is essentially
         a RAW table generator.

## Dynamic AML

ACPI Definition block (e.g. DSDT or SSDT) tables are used to describe system
devices along with other control and power management information. These tables
are written using ACPI Source Language (ASL). The ASL code is compiled using an
ASL compiler (e.g. Intel iASL compiler) to generate ACPI Machine Language (AML)
bytecode.

Since, definition blocks are represented using AML grammar, run-time generation
of definition blocks is complex. Dynamic AML is a feature of Dynamic Tables
framework that provides a solution for dynamic generation of ACPI Definition
block tables.

Dynamic AML introduces the following techniques:
* AML Fixup
* AML Codegen
* AML Fixup + Codegen

### AML Fixup
AML fixup is a technique that involves compiling an ASL template file to
generate AML bytecode. This template AML bytecode can be parsed at run-time
and a fixup code can update the required fields in the AML template.

To simplify AML Fixup, the Dynamic Tables Framework provides an *AmlLib*
library with a rich set of APIs that can be used to fixup the AML code.

### AML Codegen
AML Codegen employs generating small segments of AML code. The *AmlLib*
library provides AML Codegen APIs that generate the AML code segments.

    Example: The following table depicts the AML Codegen APIs and the
             corresponding ASL code that would be generated.

    | AML Codegen API                | ASL Code                       |
    |--------------------------------|--------------------------------|
    |  AmlCodeGenDefinitionBlock (   |  DefinitionBlock (             |
    |    ..,                         |    ...                         |
    |    &RootNode);                 |  ) {                           |
    |  AmlCodeGenScope (             |    Scope (_SB) {               |
    |    "\_SB",                     |                                |
    |    RootNode,                   |                                |
    |    &ScopeNode);                |                                |
    |  AmlCodeGenDevice (            |    Device (CPU0) {             |
    |    "CPU0",                     |                                |
    |    ScopeNode,                  |                                |
    |    &CpuNode);                  |                                |
    |  AmlCodeGenNameString (        |      Name (_HID, "ACPI0007")   |
    |    "_HID",                     |                                |
    |    "ACPI0007",                 |                                |
    |    CpuNode,                    |                                |
    |    &HidNode);                  |                                |
    |  AmlCodeGenNameInteger (       |      Name (_UID, Zero)         |
    |    "_UID",                     |                                |
    |    0,                          |                                |
    |    CpuNode,                    |                                |
    |    &UidNode);                  |                                |
    |                                |      } // Device               |
    |                                |    } // Scope                  |
    |                                |  } // DefinitionBlock          |

### AML Fixup + Codegen
A combination of AML Fixup and AML Codegen could be used for generating
Definition Blocks. For example the AML Fixup could be used to fixup certain
parts of the AML template while the AML Codegen APIs could be used to inserted
small fragments of AML code in the AML template.

### AmlLib Library
Since, AML bytecode represents complex AML grammar, an **AmlLib** library is
introduced to assist parsing and traversing of the AML bytecode at run-time.

The AmlLib library parses a definition block and represents it as an AML
tree. This tree representation is based on the AML grammar defined by the
ACPI 6.3 specification, section - 20 'ACPI Machine Language (AML)
Specification'.

AML objects, methods and data are represented as tree nodes. Since the AML
data is represented as tree nodes, it is possible to traverse the tree, locate
a node and modify the node data. The tree can then be serialized to a buffer
(that represents the definition block). This definition block containing
the fixed up AML code can then be installed as an ACPI table (DSDT/SSDT).

AmlLib provides a rich API to operate on AML data. For example it provides
APIs to update a device's name, the value of a "_UID" object, and the memory
and interrupt number stored in a "_CRS" node.

Although the AmlLib performs checks to a reasonable extent while modifying a
definition block, these checks may not cover all aspects due to the complexity
of the ASL/AML language. It is therefore recommended to review any operation
performed, and validate the generated output.

    Example: The serialized AML code could be validated by
     - Saving the generated AML to a file and comparing with
       a reference output.
     or
     - Disassemble the generated AML using the iASL compiler
       and verifying the output.

### Bespoke ACPI tables

The Dynamic Tables framework supports the creation of several tables using
standard generators, see Feature Summary Section for a list of such tables.

The supported platforms already contain several tables.
If a table is not present for the platform, two alternative processes can be followed:

- define the table in using ASL,
- define the table in packed C structures (also known as RAW).

The two approaches are detailed below.

#### Adding an ASL table for which the Dynamic Tables Framework does not provide a standard generator

This method creates the SSDT table from the ASL source, using a standard generator.
Perform the following steps:

1. Create the table source file, placing it within the ConfigurationManager source tree, e.g.:

Create a file Platform/ARM/VExpressPkg/ConfigurationManager/ConfigurationManagerDxe/AslTables/NewTableSource.asl
with the following contents:

```
DefinitionBlock ("", "SSDT", 2, "XXXXXX", "XXXXXXXX", 1) {
  Scope(_SB) {
    Device(FLA0) {
      Name(_HID, "XXXX0000")
      Name(_UID, 0)

      // _DSM - Device Specific Method
      Function(_DSM,{IntObj,BuffObj},{BuffObj, IntObj, IntObj, PkgObj})
      {
          W0 = 0x1
          return (W0)
      }
    }
  }
}
```

2. Reference the table source file in ConfigurationMangerDxe.inf

```
 [Sources]
  AslTables/NewTableSource.asl
```

3. Update the ConfigurationManager.h file
Platform/ARM/VExpressPkg/ConfigurationManager/ConfigurationManagerDxe/ConfigurationManager.h

Add an array to hold the AML code:
```
   extern CHAR8 newtablesource_aml_code[];
```

Note: the array name is composed of the ASL source file name all in lower case, followed by the _aml_code postfix.

4. Increment the macro PLAT_ACPI_TABLE_COUNT

5. Add a new CM_STD_OBJ_ACPI_TABLE_INFO structure entry and initialise.

 - the entry contains:
    - the table signature,
    - the table revision (unused in this case),
    - the ID of the standard generator to be used (the SSDT generator in this case).
    - a pointer to the AML code,

```
     // Table defined in the NewTableSource.asl file
     {
       EFI_ACPI_6_4_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
       0, // Unused
       CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdt),
       (EFI_ACPI_DESCRIPTION_HEADER*)newtablesource_aml_code
     },
```

#### Add a RAW table for which there is no standard generator

An ACPI table can be defined as a packed C struct in the C source code. This is referred to as the "raw" table format.
The steps to create a table in raw format are detailed below:

1. Define the table in a C source file and populate the ACPI table structure field with the required values.

   For example, create the file Platform/ARM/VExpressPkg/ConfigurationManager/ConfigurationManagerDxe/RawTable.c

```
    // Example creating the HMAT in raw format
    EFI_ACPI_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE Hmat = {
     ...
    };
```

2. Reference the table source file in ConfigurationMangerDxe.inf

```
 [Sources]
  RawTable.c
```

2. Increment the macro PLAT_ACPI_TABLE_COUNT

3. Add a new CM_STD_OBJ_ACPI_TABLE_INFO structure entry and initialise.

 - the entry contains:
    - the table signature,
    - the table revision,
    - the RAW generator ID.
    - a pointer to the C packed struct that defines the table,

```
    {
      EFI_ACPI_6_3_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_SIGNATURE,
      EFI_ACPI_6_3_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdRaw),
      (EFI_ACPI_DESCRIPTION_HEADER*)&Hmat
    },
```

# Roadmap

The current implementation of the Configuration Manager populates the
platform information statically as a C structure. Further enhancements
to introduce runtime loading of platform information from a platform
information file is planned.

Also support for generating SMBIOS tables is planned and will be added
subsequently.

# Supported Platforms

1. Juno
2. FVP Models

# Build Instructions

1. Set path for the iASL compiler with support for generating a C header
   file as output.

2. Set PACKAGES_PATH to point to the locations of the following repositories:

Example:

> set PACKAGES_PATH=%CD%\edk2;%CD%\edk2-platforms;

  or

> export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-platforms

3. To enable Dynamic tables framework the *'DYNAMIC_TABLES_FRAMEWORK'*
option must be defined. This can be passed as a command line
parameter to the edk2 build system.

Example:

>build -a AARCH64 -p Platform\ARM\JunoPkg\ArmJuno.dsc
   -t GCC5 **-D DYNAMIC_TABLES_FRAMEWORK**

or

>build -a AARCH64 -p Platform\ARM\VExpressPkg\ArmVExpress-FVP-AArch64.dsc
   -t GCC5 **-D DYNAMIC_TABLES_FRAMEWORK**

# Prerequisites

Ensure that the latest ACPICA iASL compiler is used for building *Dynamic Tables Framework*.
*Dynamic Tables Framework* has been tested using the following iASL compiler version:
[Version 20200717](https://www.acpica.org/node/183), dated 17 July, 2020.


#Running CI builds locally

The TianoCore EDKII project has introduced Core CI infrastructure using TianoCore EDKII Tools PIP modules:

   -  *[edk2-pytool-library](https://pypi.org/project/edk2-pytool-library)*

   - *[edk2-pytool-extensions](https://pypi.org/project/edk2-pytool-extensions)*


The instructions to setup the CI environment are in *'edk2\\.pytool\\Readme.md'*

## Building DynamicTablesPkg with Pytools

1. [Optional] Create a Python Virtual Environment - generally once per workspace

    ```
        python -m venv <name of virtual environment>

        e.g. python -m venv edk2-ci
    ```

2. [Optional] Activate Virtual Environment - each time new shell/command window is opened

    ```
        <name of virtual environment>/Scripts/activate

        e.g. On a windows host PC run:
             edk2-ci\Scripts\activate.bat
    ```
3. Install Pytools - generally once per virtual env or whenever pip-requirements.txt changes

    ```
        pip install --upgrade -r pip-requirements.txt
    ```

4. Initialize & Update Submodules - only when submodules updated

    ```
        stuart_setup -c .pytool/CISettings.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>

        e.g. stuart_setup -c .pytool/CISettings.py TOOL_CHAIN_TAG=GCC5
    ```

5. Initialize & Update Dependencies - only as needed when ext_deps change

    ```
        stuart_update -c .pytool/CISettings.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>

        e.g. stuart_update -c .pytool/CISettings.py TOOL_CHAIN_TAG=GCC5
    ```

6. Compile the basetools if necessary - only when basetools C source files change

    ```
        python BaseTools/Edk2ToolsBuild.py -t <ToolChainTag>
    ```

7. Compile DynamicTablesPkg

    ```
        stuart_build-c .pytool/CISettings.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>

        e.g. stuart_ci_build -c .pytool/CISettings.py TOOL_CHAIN_TAG=GCC5 -p DynamicTablesPkg -a AARCH64 --verbose
    ```

    - use `stuart_build -c .pytool/CISettings.py -h` option to see help on additional options.


# Documentation

Refer to the following presentation from *UEFI Plugfest Seattle 2018*:

[Dynamic Tables Framework: A Step Towards Automatic Generation of Advanced Configuration and Power Interface (ACPI) & System Management BIOS (SMBIOS) Tables](http://www.uefi.org/sites/default/files/resources/Arm_Dynamic%20Tables%20Framework%20A%20Step%20Towards%20Automatic%20Generation%20of%20Advanced%20Configuration%20and%20Power%20Interface%20%28ACPI%29%20%26%20System%20Management%20BIOS%20%28SMBIOS%29%20Tables%20_0.pdf)

## Configuration Manager Objects

The CM_OBJECT_ID type is used to identify the Configuration Manager
    objects.

## Description of Configuration Manager Object ID

| 31     -     28 | 27 - 8 | 7    -    0 |
| :-------------: | :----: | :---------: |
| `Name Space ID` |    0   | `Object ID` |
------------------------------------------

### Name Space ID: Bits [31:28]

|  ID   |  Description                      | Comments |
| ---:  | :--------------------------       | :---     |
| 0000b | Standard                          | |
| 0001b | Arch Common                       | |
| 0010b | ARM                               | |
| 0011b | X64                               | |
| 1111b | Custom/OEM                        | |
| `*`   | All other values are reserved.    | |

### Bits: [27:8] - Reserved, must be zero.

### Bits: [7:0] - Object ID

#### Object ID's in the Standard Namespace:

|  ID   |  Description                      | Comments |
| ---:  | :--------------------------       | :---     |
|   0   | Configuration Manager Revision    | |
|   1   | ACPI Table List                   | |
|   2   | SMBIOS Table List                 | |

#### Object ID's in the ARM Namespace:

|  ID   |  Description                              | Comments |
| ---:  | :--------------------------               | :---     |
|   0   | Reserved                                  | |
|   1   | Boot Architecture Info                    | |
|   2   | GICC Info                                 | |
|   3   | GICD Info                                 | |
|   4   | GIC MSI Frame Info                        | |
|   5   | GIC Redistributor Info                    | |
|   6   | GIC ITS Info                              | |
|   7   | Generic Timer Info                        | |
|   8   | Platform GT Block Info                    | |
|   9   | Generic Timer Block Frame Info            | |
|  10   | Platform Generic Watchdog                 | |
|  11   | ITS Group                                 | |
|  12   | Named Component                           | |
|  13   | Root Complex                              | |
|  14   | SMMUv1 or SMMUv2                          | |
|  15   | SMMUv3                                    | |
|  16   | PMCG                                      | |
|  17   | GIC ITS Identifier Array                  | |
|  18   | ID Mapping Array                          | |
|  19   | SMMU Interrupt Array                      | |
|  20   | CMN 600 Info                              | |
|  21   | Reserved Memory Range Node                | |
|  22   | Memory Range Descriptor                   | |
|  23   | Embedded Trace Extension/Module Info      | |
|  `*`  | All other values are reserved.            | |

#### Object ID's in the Arch Common Namespace:

|  ID   |  Description                              | Comments |
| ---:  | :--------------------------               | :---     |
|   0   |  Reserved                                 | |
|   1   | Power Management Profile Info             | |
|   2   | Serial Port Info                          | |
|   3   | Serial Console Port Info                  | |
|   4   | Serial Debug Port Info                    | |
|   5   | Hypervisor Vendor Id                      | |
|   6   | Fixed feature flags for FADT              | |
|   7   | CM Object Reference                       | |
|   8   | PCI Configuration Space Info              | |
|   9   | PCI Address Map Info                      | |
|  10   | PCI Interrupt Map Info                    | |
|  11   | Memory Affinity Info                      | |
|  12   | Device Handle Acpi                        | |
|  13   | Device Handle PCI                         | |
|  14   | Generic Initiator Affinity Info           | |
|  15   | Low Power Idle State Info                 | |
|  16   | Processor Hierarchy Info                  | |
|  17   | Cache Info                                | |
|  18   | Continuous Performance Control Info       | |
|  19   | Pcc Subspace Type 0 Info                  | |
|  20   | Pcc Subspace Type 1 Info                  | |
|  21   | Pcc Subspace Type 2 Info                  | |
|  22   | Pcc Subspace Type 3 Info                  | |
|  23   | Pcc Subspace Type 4 Info                  | |
|  24   | Pcc Subspace Type 5 Info                  | |
|  25   | P-State Dependency (PSD) Info             | |
|  26   | TPM Interface Info                        | |
|  27   | SPMI Interface Info                       | |
|  28   | SPMI Interrupt and Device/Uid Info        | |
|  `*`  | All other values are reserved.            | |

#### Object ID's in the X64 Namespace:

|  ID   |  Description                              | Comments |
| ---:  | :--------------------------               | :---     |
|   0   | Reserved                                  | |
|   1   | SCI Interrupt Info                        | |
|   2   | SCI Command Info                          | |
|   3   | Legacy Power Management Block Info        | |
|   4   | Legacy GPE Block Info                     | |
|   5   | Power Management Block Info               | |
|   6   | GPE Block Info                            | |
|   7   | Sleep Block Info                          | |
|   8   | Reset Block Info                          | |
|   9   | Miscellaneous Block Info                  | |
|  10   | Windows protection flag Info              | |
|  11   | HPET device Info                          | |
|  `*`  | All other values are reserved.            | |
