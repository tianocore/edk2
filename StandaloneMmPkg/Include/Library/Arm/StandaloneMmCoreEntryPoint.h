/** @file
  Entry point to the Standalone MM Foundation when initialized during the SEC
  phase on ARM platforms

Copyright (c) 2017 - 2021, Arm Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __STANDALONEMMCORE_ENTRY_POINT_H__
#define __STANDALONEMMCORE_ENTRY_POINT_H__

#include <Library/PeCoffLib.h>
#include <Library/FvLib.h>

#define CPU_INFO_FLAG_PRIMARY_CPU  0x00000001

typedef struct {
  UINT8  Type;       /* type of the structure */
  UINT8  Version;    /* version of this structure */
  UINT16 Size;      /* size of this structure in bytes */
  UINT32 Attr;      /* attributes: unused bits SBZ */
} EFI_PARAM_HEADER;

typedef struct {
  UINT64 Mpidr;
  UINT32 LinearId;
  UINT32 Flags;
} EFI_SECURE_PARTITION_CPU_INFO;

typedef struct {
  EFI_PARAM_HEADER              Header;
  UINT64                        SpMemBase;
  UINT64                        SpMemLimit;
  UINT64                        SpImageBase;
  UINT64                        SpStackBase;
  UINT64                        SpHeapBase;
  UINT64                        SpNsCommBufBase;
  UINT64                        SpSharedBufBase;
  UINT64                        SpImageSize;
  UINT64                        SpPcpuStackSize;
  UINT64                        SpHeapSize;
  UINT64                        SpNsCommBufSize;
  UINT64                        SpPcpuSharedBufSize;
  UINT32                        NumSpMemRegions;
  UINT32                        NumCpus;
  EFI_SECURE_PARTITION_CPU_INFO *CpuInfo;
} EFI_SECURE_PARTITION_BOOT_INFO;

typedef
EFI_STATUS
(*PI_MM_ARM_TF_CPU_DRIVER_ENTRYPOINT) (
  IN UINTN EventId,
  IN UINTN CpuNumber,
  IN UINTN NsCommBufferAddr
  );

typedef struct {
  PI_MM_ARM_TF_CPU_DRIVER_ENTRYPOINT *ArmTfCpuDriverEpPtr;
} ARM_TF_CPU_DRIVER_EP_DESCRIPTOR;

typedef RETURN_STATUS (*REGION_PERMISSION_UPDATE_FUNC) (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
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
  IN  CONST PE_COFF_LOADER_IMAGE_CONTEXT      *ImageContext,
  IN  EFI_PHYSICAL_ADDRESS                    ImageBase,
  IN  UINT32                                  SectionHeaderOffset,
  IN  CONST  UINT16                           NumberOfSections,
  IN  REGION_PERMISSION_UPDATE_FUNC           TextUpdater,
  IN  REGION_PERMISSION_UPDATE_FUNC           ReadOnlyUpdater,
  IN  REGION_PERMISSION_UPDATE_FUNC           ReadWriteUpdater
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
  IN        VOID                            *TeData,
  IN  OUT   PE_COFF_LOADER_IMAGE_CONTEXT    *ImageContext,
      OUT   EFI_PHYSICAL_ADDRESS            *ImageBase,
  IN  OUT   UINT32                          *SectionHeaderOffset,
  IN  OUT   UINT16                          *NumberOfSections
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
  IN        EFI_FIRMWARE_VOLUME_HEADER      *BfvAddress,
  IN  OUT   VOID                            **TeData,
  IN  OUT   UINTN                           *TeDataSize
  );


/**
  Use the boot information passed by privileged firmware to populate a HOB list
  suitable for consumption by the MM Core and drivers.

  @param  [in, out] CpuDriverEntryPoint   Address of MM CPU driver entrypoint
  @param  [in]      PayloadBootInfo       Boot information passed by privileged
                                          firmware

**/
VOID *
EFIAPI
CreateHobListFromBootInfo (
  IN  OUT  PI_MM_ARM_TF_CPU_DRIVER_ENTRYPOINT *CpuDriverEntryPoint,
  IN       EFI_SECURE_PARTITION_BOOT_INFO     *PayloadBootInfo
  );


/**
  The entry point of Standalone MM Foundation.

  @param  [in]  SharedBufAddress  Pointer to the Buffer between SPM and SP.
  @param  [in]  SharedBufSize     Size of the shared buffer.
  @param  [in]  cookie1           Cookie 1
  @param  [in]  cookie2           Cookie 2

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN VOID    *SharedBufAddress,
  IN UINT64  SharedBufSize,
  IN UINT64  cookie1,
  IN UINT64  cookie2
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
  IN EFI_HANDLE             ImageHandle,
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
