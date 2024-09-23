/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UEFI_PAYLOAD_ENTRY_H__
#define __UEFI_PAYLOAD_ENTRY_H__

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PeCoffLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Guid/MemoryAllocationHob.h>
#include <Library/IoLib.h>
#include <Library/PeCoffLib.h>
#include <Library/BlParseLib.h>
#include <Library/PlatformSupportLib.h>
#include <Library/CpuLib.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <Guid/SerialPortInfoGuid.h>
#include <Guid/MemoryMapInfoGuid.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <Guid/GraphicsInfoHob.h>
#include <UniversalPayload/SmbiosTable.h>
#include <UniversalPayload/AcpiTable.h>
#include <UniversalPayload/UniversalPayload.h>
#include <UniversalPayload/ExtraData.h>
#include <UniversalPayload/SerialPortInfo.h>
#include <UniversalPayload/DeviceTree.h>
#include <Guid/PcdDataBaseSignatureGuid.h>

#define LEGACY_8259_MASK_REGISTER_MASTER  0x21
#define LEGACY_8259_MASK_REGISTER_SLAVE   0xA1
#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  ((ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1)))

#define E820_RAM        1
#define E820_RESERVED   2
#define E820_ACPI       3
#define E820_NVS        4
#define E820_UNUSABLE   5
#define E820_DISABLED   6
#define E820_PMEM       7
#define E820_UNDEFINED  8

/**
  Add a new HOB to the HOB List.

  @param HobType            Type of the new HOB.
  @param HobLength          Length of the new HOB to allocate.

  @return  NULL if there is no space to create a hob.
  @return  The address point to the new created hob.

**/
VOID *
EFIAPI
CreateHob (
  IN  UINT16  HobType,
  IN  UINT16  HobLength
  );

/**
  Update the Stack Hob if the stack has been moved

  @param  BaseAddress   The 64 bit physical address of the Stack.
  @param  Length        The length of the stack in bytes.

**/
VOID
EFIAPI
UpdateStackHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  );

/**
  Build a Handoff Information Table HOB

  This function initialize a HOB region from EfiMemoryBegin to
  EfiMemoryTop. And EfiFreeMemoryBottom and EfiFreeMemoryTop should
  be inside the HOB region.

  @param[in] EfiMemoryBottom       Total memory start address
  @param[in] EfiMemoryTop          Total memory end address.
  @param[in] EfiFreeMemoryBottom   Free memory start address
  @param[in] EfiFreeMemoryTop      Free memory end address.

  @return   The pointer to the handoff HOB table.

**/
EFI_HOB_HANDOFF_INFO_TABLE *
EFIAPI
HobConstructor (
  IN VOID  *EfiMemoryBottom,
  IN VOID  *EfiMemoryTop,
  IN VOID  *EfiFreeMemoryBottom,
  IN VOID  *EfiFreeMemoryTop
  );

/**
  Find DXE core from FV and build DXE core HOBs.

  @param[out]  DxeCoreEntryPoint     DXE core entry point

  @retval EFI_SUCCESS        If it completed successfully.
  @retval EFI_NOT_FOUND      If it failed to load DXE FV.
**/
EFI_STATUS
LoadDxeCore (
  OUT PHYSICAL_ADDRESS  *DxeCoreEntryPoint
  );

/**
  Find DXE core from FV and build DXE core HOBs.

  @param[in]   DxeFv                 The FV where to find the DXE core.
  @param[out]  DxeCoreEntryPoint     DXE core entry point

  @retval EFI_SUCCESS        If it completed successfully.
  @retval EFI_NOT_FOUND      If it failed to load DXE FV.
**/
EFI_STATUS
UniversalLoadDxeCore (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *DxeFv,
  OUT PHYSICAL_ADDRESS            *DxeCoreEntryPoint
  );

/**
  It will Parse FDT -node based on information.
  @param[in]  FdtBase   The starting memory address of FdtBase
  @retval HobList   The base address of Hoblist.

**/
UINT64
EFIAPI
FdtNodeParser (
  IN VOID  *FdtBase
  );

/**
  It will Parse FDT -custom node based on information.
  @param[in]  FdtBase The starting memory address of FdtBase
  @param[in]  HostList The starting memory address of New Hob list.

**/
UINTN
EFIAPI
CustomFdtNodeParser (
  IN VOID  *FdtBase,
  IN VOID  *HostList
  );

/**
   Transfers control to DxeCore.

   This function performs a CPU architecture specific operations to execute
   the entry point of DxeCore with the parameters of HobList.

   @param DxeCoreEntryPoint         The entry point of DxeCore.
   @param HobList                   The start of HobList passed to DxeCore.
**/
VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS  DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS  HobList
  );

EFI_STATUS
FixUpPcdDatabase (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *DxeFv
  );

/**
  This function searchs a given section type within a valid FFS file.

  @param  FileHeader            A pointer to the file header that contains the set of sections to
                                be searched.
  @param  SearchType            The value of the section type to search.
  @param  SectionData           A pointer to the discovered section, if successful.

  @retval EFI_SUCCESS           The section was found.
  @retval EFI_NOT_FOUND         The section was not found.

**/
EFI_STATUS
FileFindSection (
  IN EFI_FFS_FILE_HEADER  *FileHeader,
  IN EFI_SECTION_TYPE     SectionType,
  OUT VOID                **SectionData
  );

/**
  This function searchs a given file type with a given Guid within a valid FV.
  If input Guid is NULL, will locate the first section having the given file type

  @param FvHeader        A pointer to firmware volume header that contains the set of files
                         to be searched.
  @param FileType        File type to be searched.
  @param Guid            Will ignore if it is NULL.
  @param FileHeader      A pointer to the discovered file, if successful.

  @retval EFI_SUCCESS    Successfully found FileType
  @retval EFI_NOT_FOUND  File type can't be found.
**/
EFI_STATUS
FvFindFileByTypeGuid (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader,
  IN  EFI_FV_FILETYPE             FileType,
  IN  EFI_GUID                    *Guid           OPTIONAL,
  OUT EFI_FFS_FILE_HEADER         **FileHeader
  );

/**
  Build ACPI board info HOB using infomation from ACPI table

  @param  AcpiTableBase      ACPI table start address in memory

  @retval  A pointer to ACPI board HOB ACPI_BOARD_INFO. Null if build HOB failure.
**/
ACPI_BOARD_INFO *
BuildHobFromAcpi (
  IN   UINT64  AcpiTableBase
  );

/**
  Allocates one or more pages .

  Allocates the number of pages of MemoryType and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.
  If Pages is 0, then NULL is returned.
  If there is not enough memory availble to satisfy the request, then NULL
  is returned.

  @param   Pages                 The number of 4 KB pages to allocate.
  @param   MemoryType            The Memorytype
  @return  A pointer to the allocated buffer or NULL if allocation fails.
**/
VOID *
EFIAPI
PayloadAllocatePages (
  IN UINTN            Pages,
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  Entry point to the C language phase of UEFI payload.
  @param[in]   FdtPrt  The starting address of FDT .
  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
FitUplEntryPoint (
  IN UINTN  BootloaderParameter
  );

/**
  Entry point to the C language phase of UEFI payload.
  @param[in]   BootloaderParameter    The starting address of bootloader parameter block.
  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
UplEntryPoint (
  IN UINTN  BootloaderParameter
  );

#endif
