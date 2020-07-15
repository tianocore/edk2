Dynamic Tables Framework
------------------------

Dynamic Tables Framework provides mechanisms to reduce the amount
of effort required in porting firmware to new platforms. The aim is
to provide an implementation capable of generating the firmware
tables from an external source.  This is potentially a management
node, either local or remote, or, where suitable, a file that might
be generated from the system construction.  This initial release
does not fully implement that - the configuration is held in local
UEFI modules.

Feature Summary
---------------
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
* SPCR - Serial Port Console Redirection Table
* SSDT - Secondary System Description Table. This is essentially
         a RAW table generator.

Roadmap
-------
The current implementation of the Configuration Manager populates the
platform information statically as a C structure. Further enhancements
to introduce runtime loading of platform information from a platform
information file is planned.

Also support for generating SMBIOS tables is planned and will be added
subsequently.

Related Modules
---------------

### ACPICA iASL compiler
The RAW table generator, used to process the DSDT/SSDT files depends on
the iASL compiler to convert the DSDT/SSDT ASL files to a C array containing
the hex AML code. The "-tc" option of the iASL compiler has been enhanced to
support generation of an AML hex file (C header) with a unique symbol name
so that it is suitable for inclusion from a C source file.

Related Links
--------------

<https://github.com/acpica/acpica.git>


Supported Platforms
-------------------
1. Juno
2. FVP Models

Build Instructions
------------------
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

Prerequisites
-------------
ACPICA iASL compiler with the enhanced "-tc" option to support generation of
AML hex (C header) files with unique symbol names.

A patch *'[iASL: Enhance the -tc option (create AML hex file in C)](https://github.com/acpica/acpica/commit/f9a88a4c1cd020b6a5475d63b29626852a0b5f37)'*, dated 16 March 2018 (2018-03-16),
to enable this support has been integrated to the ACPICA source repository.

Ensure that the iASL compiler used for building *Dynamic Tables Framework* has this feature enabled.

This feature was made available in the *ACPICA Compiler update
[Version 20180508](https://www.acpica.org/node/156)*, dated 8 May 2018 (2018-05-08).

Documentation
-------------

Refer to the following presentation from *UEFI Plugfest Seattle 2018*:

[Dynamic Tables Framework: A Step Towards Automatic Generation of Advanced Configuration and Power Interface (ACPI) & System Management BIOS (SMBIOS) Tables – Sami Mujawar (Arm).](http://www.uefi.org/sites/default/files/resources/Arm_Dynamic%20Tables%20Framework%20A%20Step%20Towards%20Automatic%20Generation%20of%20Advanced%20Configuration%20and%20Power%20Interface%20%28ACPI%29%20%26%20System%20Management%20BIOS%20%28SMBIOS%29%20Tables%20_0.pdf)
