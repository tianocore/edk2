/** @file
*
* Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
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
#include <Library/UefiCpuLib.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <Guid/SerialPortInfoGuid.h>
#include <Guid/SystemTableInfoGuid.h>
#include <Guid/MemoryMapInfoGuid.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <Guid/GraphicsInfoHob.h>


#define LEGACY_8259_MASK_REGISTER_MASTER  0x21
#define LEGACY_8259_MASK_REGISTER_SLAVE   0xA1
#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  ((ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1)))

/**
  Auto-generated function that calls the library constructors for all of the module's
  dependent libraries.
**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

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
  IN  UINT16    HobType,
  IN  UINT16    HobLength
  );

/**
  Update the Stack Hob if the stack has been moved

  @param  BaseAddress   The 64 bit physical address of the Stack.
  @param  Length        The length of the stack in bytes.

**/
VOID
EFIAPI
UpdateStackHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  );

/**
  Build a Handoff Information Table HOB

  This function initialize a HOB region from EfiMemoryBegin with length
  EfiMemoryLength. And EfiFreeMemoryBottom and EfiFreeMemoryTop should
  be inside the HOB region.

  @param[in] EfiMemoryBegin       Total memory start address
  @param[in] EfiMemoryLength      Total memory length reported in handoff HOB.
  @param[in] EfiFreeMemoryBottom  Free memory start address
  @param[in] EfiFreeMemoryTop     Free memory end address.

  @return   The pointer to the handoff HOB table.

**/
EFI_HOB_HANDOFF_INFO_TABLE*
EFIAPI
HobConstructor (
  IN VOID   *EfiMemoryBegin,
  IN UINTN  EfiMemoryLength,
  IN VOID   *EfiFreeMemoryBottom,
  IN VOID   *EfiFreeMemoryTop
  );

/**
  Find DXE core from FV and build DXE core HOBs.

  @param[out]  DxeCoreEntryPoint     DXE core entry point

  @retval EFI_SUCCESS        If it completed successfully.
  @retval EFI_NOT_FOUND      If it failed to load DXE FV.
**/
EFI_STATUS
LoadDxeCore (
  OUT PHYSICAL_ADDRESS        *DxeCoreEntryPoint
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
  IN EFI_PHYSICAL_ADDRESS   DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS   HobList
  );

#endif
