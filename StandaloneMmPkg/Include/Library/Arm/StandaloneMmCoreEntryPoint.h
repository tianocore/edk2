/** @file
  Entry point to the Standalone MM Foundation when initialized during the SEC
  phase on ARM platforms

  Copyright (c) 2017 - 2024, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - SPM_MM - An implementation where the Secure Partition Manager resides at EL3
              with management services running from an isolated Secure Partitions
              at S-EL0, and the communication protocol is the Management Mode(MM)
              interface.
    - FF-A - Firmware Framework for Arm A-profile
    - TL   - Transfer List

  @par Reference(s):
    - Transfer List [https://github.com/FirmwareHandoff/firmware_handoff]
    - Secure Partition Manager [https://trustedfirmware-a.readthedocs.io/en/latest/components/secure-partition-manager-mm.html].
    - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/j/?lang=en]

**/

#ifndef __STANDALONEMMCORE_ENTRY_POINT_H__
#define __STANDALONEMMCORE_ENTRY_POINT_H__

#include <Library/PeCoffLib.h>
#include <Library/FvLib.h>

#define CPU_INFO_FLAG_PRIMARY_CPU  0x00000001

/*
 * BOOT protocol used to boot StandaloneMm
 */
typedef enum {
  /// Unknown Boot protocol.
  BootProtocolUnknown,

  /// Boot information delivered via Transfer List
  /// with 32 bits register convention
  BootProtocolTl32,

  /// Boot information delivered via Transfer List
  /// with 64 bits register convention
  BootProtocolTl64,

  BootProtocolMax,
} BOOT_PROTOCOL;

/*
 * Communication ABI protocol to communicate between normal/secure partition.
 */
typedef enum {
  /// Unknown Communication ABI protocol
  AbiProtocolUnknown,

  /// Communicate via SPM_MM ABI protocol
  AbiProtocolSpmMm,

  /// Communicate via FF-A ABI protocol
  AbiProtocolFfa,

  AbiProtocolMax,
} ABI_PROTOCOL;

typedef struct {
  UINT8     Type;    /* type of the structure */
  UINT8     Version; /* version of this structure */
  UINT16    Size;    /* size of this structure in bytes */
  UINT32    Attr;    /* attributes: unused bits SBZ */
} EFI_PARAM_HEADER;

typedef RETURN_STATUS (*REGION_PERMISSION_UPDATE_FUNC) (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Privileged firmware assigns RO & Executable attributes to all memory occupied
  by the Boot Firmware Volume. This function sets the correct permissions of
  sections in the Standalone MM Core module to be able to access RO and RW data
  and make further progress in the boot process.

  @param  [in] ImageContext           Pointer to PE/COFF image context
  @param  [in] ImageBase              Base of image in memory
  @param  [in] SectionHeaderOffset    Offset of PE/COFF image section header
  @param  [in] NumberOfSections       Number of Sections
  @param  [in] TextUpdater            Function to change code permissions
  @param  [in] ReadOnlyUpdater        Function to change RO permissions
  @param  [in] ReadWriteUpdater       Function to change RW permissions

**/
EFI_STATUS
EFIAPI
UpdateMmFoundationPeCoffPermissions (
  IN  CONST PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN  EFI_PHYSICAL_ADDRESS                ImageBase,
  IN  UINT32                              SectionHeaderOffset,
  IN  CONST  UINT16                       NumberOfSections,
  IN  REGION_PERMISSION_UPDATE_FUNC       TextUpdater,
  IN  REGION_PERMISSION_UPDATE_FUNC       ReadOnlyUpdater,
  IN  REGION_PERMISSION_UPDATE_FUNC       ReadWriteUpdater
  );

/**
  Privileged firmware assigns RO & Executable attributes to all memory occupied
  by the Boot Firmware Volume. This function locates the section information of
  the Standalone MM Core module to be able to change permissions of the
  individual sections later in the boot process.

  @param  [in]      TeData                Pointer to PE/COFF image data
  @param  [in, out] ImageContext          Pointer to PE/COFF image context
  @param  [out]     ImageBase             Pointer to ImageBase variable
  @param  [in, out] SectionHeaderOffset   Offset of PE/COFF image section header
  @param  [in, out] NumberOfSections      Number of Sections

**/
EFI_STATUS
EFIAPI
GetStandaloneMmCorePeCoffSections (
  IN        VOID                          *TeData,
  IN  OUT   PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  OUT   EFI_PHYSICAL_ADDRESS              *ImageBase,
  IN  OUT   UINT32                        *SectionHeaderOffset,
  IN  OUT   UINT16                        *NumberOfSections
  );

/**
  Privileged firmware assigns RO & Executable attributes to all memory occupied
  by the Boot Firmware Volume. This function locates the Standalone MM Core
  module PE/COFF image in the BFV and returns this information.

  @param  [in]      BfvAddress         Base Address of Boot Firmware Volume
  @param  [in, out] TeData             Pointer to address for allocating memory
                                       for PE/COFF image data
  @param  [in, out] TeDataSize         Pointer to size of PE/COFF image data

**/
EFI_STATUS
EFIAPI
LocateStandaloneMmCorePeCoffData (
  IN        EFI_FIRMWARE_VOLUME_HEADER  *BfvAddress,
  IN  OUT   VOID                        **TeData,
  IN  OUT   UINTN                       *TeDataSize
  );

/**
  The entry point of Standalone MM Foundation.

  @param  [in]  Arg0        Boot information passed according to boot protocol.
  @param  [in]  Arg1        Boot information passed according to boot protocol.
  @param  [in]  Arg2        Boot information passed according to boot protocol.
  @param  [in]  Arg3        Boot information passed according to boot protocol.

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN UINTN  Arg0,
  IN UINTN  Arg1,
  IN UINTN  Arg2,
  IN UINTN  Arg3
  );

/**
  Auto generated function that calls the library constructors for all of the module's dependent libraries.

  This function must be called by _ModuleEntryPoint().
  This function calls the set of library constructors for the set of library instances
  that a module depends on.  This includes library instances that a module depends on
  directly and library instances that a module depends on indirectly through other
  libraries. This function is auto generated by build tools and those build tools are
  responsible for collecting the set of library instances, determine which ones have
  constructors, and calling the library constructors in the proper order based upon
  each of the library instances own dependencies.

  @param  ImageHandle  The image handle of the DXE Core.
  @param  SystemTable  A pointer to the EFI System Table.

**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  );

/**
  Auto generated function that calls a set of module entry points.

  This function must be called by _ModuleEntryPoint().
  This function calls the set of module entry points.
  This function is auto generated by build tools and those build tools are responsible
  for collecting the module entry points and calling them in a specified order.

  @param  HobStart  Pointer to the beginning of the HOB List passed in from the PEI Phase.

**/
VOID
EFIAPI
ProcessModuleEntryPointList (
  IN VOID  *HobStart
  );

#endif
