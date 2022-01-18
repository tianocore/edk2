# **Platform Runtime Mechanism**

Platform Runtime Mechanism (PRM) introduces the capability of moving platform-specific code out of SMM and into a
code module that executes within the OS context. Moving this firmware to the OS context provides better transparency
and mitigates the negative system impact currently accompanied with SMM solutions. Futhermore, the PRM code is
packaged into modules with well-defined entry points, each representing a specific PRM functionality.

For more details on PRM, refer to the [Platform Runtime Mechanism Specification on uefi.org](https://uefi.org/sites/default/files/resources/Platform%20Runtime%20Mechanism%20-%20with%20legal%20notice.pdf).

The `PrmPkg` maintained in this branch provides a single cohesive set of generic PRM functionality that is intended
to be leveraged by platform firmware with minimal overhead to integrate PRM functionality in the firmware.

> By default, the build makes use of a new ACPI OperationRegion type specifically introduced for PRM called
`PlatformRtMechanism`. Support for this OperationRegion is planned for the next release of the ACPI specification.
However, support for `PlatformRtMechanism` is already included in the iASL Compiler/Disassembler for early prototyping
(i.e. this package). If you would like the default build to work and/or to use PRM handlers that are invoked
through ACPI, iASL compiler [20200528](https://acpica.org/node/181) or greater must be used. If you are only
interested in compiling the code and/or using direct call style PRM handlers, you can simply remove
`PrmSsdtInstallDxe` from `PrmPkg.dsc`.

The changes in the ACPI Specification include two elements:

1. `BIT20` in Platform-Wide _OSC Capabilities DWORD2 will be used by an OS to indicate support for PRM
2. A new Operation Region Address Space Identifier Value is defined as `0xB` for `PlatformRtMechanism`

## How to Build PrmPkg

As noted earlier, resources in `PrmPkg` are intended to be referenced by a platform firmware so it can adopt support
for PRM. In that case, the platform firmware should add the `PrmConfigDxe` and `PrmLoaderDxe` drivers to its DSC and
FDF files so they are built in the platform firmware build and dispatched during its runtime. All that is left is to
add individual PRM modules to the DSC and FDF. These can be built from source or included as binaries into the platform
firmware flash map.

### PrmPkg Standalone Build

To build `PrmPkg` as a standalone package:

1. If new to EDK II, follow the directions in [Getting Started with EDK II](https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II)

2. Clone the *master* branch on the edk2 repository locally \
   ``git clone https://github.com/tianocore/edk2.git``

3. Change to the edk2 workspace directory \
   ``cd edk2``

4. Run *edksetup* to set local environment variables needed for build
   * Windows:
     * ``edksetup.bat``
   * Linux:
     * If you have not already built BaseTools:
       * ``make -C BaseTools``
     * ``. edksetup.sh``

5. Build PrmPkg \
   ``build -p PrmPkg/PrmPkg.dsc -a IA32 -a X64``
   > ***Note***: Due to the way PRM modules are compiled with exports, **only building on Visual Studio compiler tool
   chains has been tested**.

> ***Note***: \
> This package has been used without modification in several environments including client, server,
> and virtual systems.
>
> You can add your own PRM modules into the build and check them with the `PrmInfo` UEFI application described
> later in this document and dump the PRMT table in the OS to check if your PRM module is represented as expected.

### PRM Platform GUID

**IMPORTANT** PRM has a concept of a "Platform GUID" which associates a specific platform with a set of PRM modules
built for that platform. This GUID is used to ensure system compatibility for a given collection of PRM modules.

Therefore, each PRM module must only target a single platform and each platform must have a unique GUID. Even if a
PRM module is unchanged between two different platforms now, there is no guarantee that will remain the case so always
assign a unique Platform GUID for each platform.

The PRM Platform GUID is primarily used during PRM module runtime updates in the OS to ensure that the Platform GUID
in the system's ACPI table (PRMT) matches the Platform GUID of the module requested for update. Even if runtime
updates are not a planned feature for a given platform, still assign a unique Platform GUID for binary module
identification (the Platform GUID is in the module's export descriptor) and to ensure such updates can be seamlessly
supported in the future if needed.

In the `PrmPkg` implementation, the Platform GUID is automatically derived from the PLATFORM_GUID in the DSC file of
the package being built.

### Build Output

Like a typical EDK II package, the PrmPkg binary build output can be found in the Build directory in the edk2
workspace. The organization in that directory follows the same layout as other EDK II packages.

For example, that path to PRM module sample binaries for a DEBUG VS2017 X64 build is: \
``edk2/Build/Prm/DEBUG_VS2017/X64/PrmPkg/Samples``

## Overview

At a high-level, PRM can be viewed from three levels of granularity:

1. `PRM interface` - Encompassing the entirety of firmware functionalities and data provided to OS runtime. Most
   information is provided through ACPI tables to be agnostic to a UEFI implementation.
2. `PRM module` - An independently updatable package of PRM handlers. The PRM interface will be composed of multiple
   PRM modules. This requirement allows for the separation of OEM and IHV PRM code, each of which can be serviced
   independently.
3. `PRM handler` - The implementation/callback of a single PRM functionality as identified by a GUID.

## Firmware Design

The firmware has three key generic drivers to support PRM:

1. A `PRM Loader driver` - Functionality is split across three phases:
   1. Discover - Find all PRM modules in the firmware image made available by the platform firmware author.
      * This phase includes verifying authenticity/integrity of the image, the image executable type, the export
        table is present and the PRM Export Module Descriptor is present and valid.
   2. Process - Convert PRM handler GUID to name mappings in the PRM Module Export Descriptor to PRM handler Name
      to physical address mappings required to construct the PRM ACPI table.
   3. Publish - Publish the PRM ACPI table using the information from the Process phase.

2. A `PRM Configuration driver` - A generic driver responsible for processing PRM module configuration information
   consumed through a `PRM_CONFIG_PROTOCOL` per PRM module instance. Therefore, the `PRM_CONFIG_PROTOCOL` serves
   as the dynamic interface for this driver to process PRM module resources and prepare the module's data to be
   configured properly for OS runtime.

3. A `PRM Module` - Not a single driver but a user written PE/COFF image that follows the PRM module authoring process.
   A PRM module groups together cohesive sets of PRM functionality into functions referred to as "PRM handlers".

## PrmPkg Code Organization

The package follows a standard EDK II style package format. The list below contains some notable areas to
explore in the package:

* [ACPI Table Definitions](PrmPkg/PrmLoaderDxe/PrmAcpiTable.h)
* [Common Interface Definitions](PrmPkg/Include)
* [PRM Config Driver](PrmPkg/PrmConfigDxe)
* [PRM Loader Driver](PrmPkg/PrmLoaderDxe)
* [Sample PRM Modules](PrmPkg/Samples)

While the package does provide sample PRM modules to be used as a reference, actual PRM modules should not be
maintained in PrmPkg. It is intended to only contain PRM infrastructure code and a few samples of how to use
that infrastructure. The PrmPkg is meant to be used as-is by firmware that supports PRM. Any shortcomings that
prevent the package from being used as-is should be addressed directly in PrmPkg.

## PRM Information UEFI Application

A UEFI application is provided in this package called `PrmInfo` that allows a user to display and test PRM
modules on their system.

[Link to application source code](PrmPkg/Application/PrmInfo).

This application is intended to be helpful during PRM enabling by allowing the user to:

  1. Confirm that their firmware port of the PRM infrastructure implemented in this package is functioning correctly.
  2. Quickly get information about what PRM modules and handlers that are present on a given system.
  3. Quickly test PRM handlers without booting into a full operating system.
  4. Develop and exercise PRM handlers prior to the availability of an operating system that is PRM aware.

Execute the application help command for detailed usage instructions and examples of how to use the application: \
  ``PrmInfo -?``

*Example Usage:*

![PrmInfo Usage Example](https://raw.githubusercontent.com/tianocore/edk2-staging/PlatformRuntimeMechanism/PrmPkg/Application/PrmInfo/PrmInfo_Usage_Example.gif)

## PRM Module

> ***Note***: You can find simple examples of PRM modules in the Samples directory of this package.
> [Samples/Readme.md](PrmPkg/Samples/Readme.md) has more information.

By default, the EDK II implementation of UEFI does not allow images with the subsystem type
`IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER` to be built with exports.

```txt
ERROR - Linker #1294 from LINK : fatal exports and import libraries are not supported with /SUBSYSTEM:EFI_RUNTIME_DRIVER
```

This can adjusted in the MSVC linker options.

The subsystem type is changed in the firmware build to allow the export table to be added but the subsystem type in the
final image is still `0xC` (`EFI Runtime Driver`). This is important to allow the DXE dispatcher to use its standard
image verification and loading algorithms to load the image into permanent memory during the DXE execution phase.

All firmware-loaded PRM modules are loaded into a memory buffer of type `EfiRuntimeServicesCode`. This means the
operating system must preserve all PRM handler code and the buffer will be reflected in the UEFI memory map. The
execution for invoking PRM handlers is the same as that required for UEFI Runtime Services, notably 4KiB or more of
available stack space must be provided and the stack must be 16-byte aligned.

***Note:*** Long term it is possible to similarly load the modules into a `EfiRuntimeServicesCode` buffer and perform
relocation fixups with a new EFI module type for PRM if desired. It was simply not done since it is not essential
for this POC.

Where possible, PRM module information is stored and generated using industry compiler tool chains. This is a key
motivation behind using PE/COFF export tables to expose PRM module information and using a single PRM module binary
definition consistent between firmware and OS load.

### PRM Module Exports

A PRM module must contain at least two exports: A PRM Module Export Descriptor and at least one PRM handler. Here's
an example of an export table from a PRM module that has a single PRM handler:

```txt
  0000000000005000: 00 00 00 00 FF FF FF FF 00 00 00 00 3C 50 00 00  ............<P..
  0000000000005010: 01 00 00 00 02 00 00 00 02 00 00 00 28 50 00 00  ............(P..
  0000000000005020: 30 50 00 00 38 50 00 00 78 13 00 00 20 40 00 00  0P..8P..x... @..
  0000000000005030: 5D 50 00 00 7C 50 00 00 00 00 01 00 50 72 6D 53  ]P..|P......PrmS
  0000000000005040: 61 6D 70 6C 65 43 6F 6E 74 65 78 74 42 75 66 66  ampleContextBuff
  0000000000005050: 65 72 4D 6F 64 75 6C 65 2E 64 6C 6C 00 44 75 6D  erModule.dll.Dum
  0000000000005060: 70 53 74 61 74 69 63 44 61 74 61 42 75 66 66 65  pStaticDataBuffe
  0000000000005070: 72 50 72 6D 48 61 6E 64 6C 65 72 00 50 72 6D 4D  rPrmHandler.PrmM
  0000000000005080: 6F 64 75 6C 65 45 78 70 6F 72 74 44 65 73 63 72  oduleExportDescr
  0000000000005090: 69 70 74 6F 72 00                                iptor.

    00000000 characteristics
    FFFFFFFF time date stamp
        0.00 version
           1 ordinal base
           2 number of functions
           2 number of names

    ordinal hint RVA      name

          1    0 00001378 DumpStaticDataBufferPrmHandler
          2    1 00004020 PrmModuleExportDescriptor

```

### PRM Image Format

PRM modules are ultimately PE/COFF images. However, when packaged in firmware the PE/COFF image is placed into a
Firmware File System (FFS) file. This is transparent to the operating system but done to better align with the typical
packaging of PE32(+) images managed in the firmware binary image. In the dump of the PRM FV binary image shown earlier,
the FFS sections placed by EDK II build tools ("DXE dependency", "User interface", "Version") that reside alongside the
PE/COFF binary are shown. A PRM module can be placed into a firmware image as a pre-built PE/COFF binary or built
during the firmware build process. In either case, the PE/COFF section is contained in a FFS file as shown in that
image.

### PRM Module Implementation

To simplify building the PRM Module Export Descriptor, a PRM module implementation can use the following macros to mark
functions as PRM handlers. In this example, a PRM module registers three functions by name as PRM handlers with the
associated GUIDs.

```c
//
// Register the PRM export information for this PRM Module
//
PRM_MODULE_EXPORT (
  PRM_HANDLER_EXPORT_ENTRY (PRM_HANDLER_1_GUID, PrmHandler1),
  PRM_HANDLER_EXPORT_ENTRY (PRM_HANDLER_2_GUID, PrmHandler2),
  PRM_HANDLER_EXPORT_ENTRY (PRM_HANDLER_N_GUID, PrmHandlerN)
  );
```

`PRM_MODULE_EXPORT` take a variable-length argument list of `PRM_HANDLER_EXPORT_ENTRY` entries that each describe an
individual PRM handler being exported for the module. Ultimately, this information is used to define the structure
necessary to statically allocate the PRM Module Export Descriptor Structure (and its PRM Handler Export Descriptor
substructures) in the image.

Another required export for PRM modules is automatically provided in `PrmModule.h`, a header file that pulls together
all the includes needed to author a PRM module. This export is `PRM_MODULE_UPDATE_LOCK_EXPORT`. By including,
`PrmModule.h`, a PRM module has the `PRM_MODULE_UPDATE_LOCK_DESCRIPTOR` automatically exported.

## PRM Handler Constraints

At this time, PRM handlers are restricted to a maximum identifier length of 128 characters. This is checked when using
the `PRM_HANDLER_EXPORT` macro by using a static assert that reports a violation at build-time.

PRM handlers are **not** allowed to use UEFI Runtime Services and should not rely upon any UEFI constructs. For the
purposes of this POC, this is currently not explicitly enforced but should be in the final changes.
