/** @file
  HOB Library implementation for Standalone MM Core.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017 - 2018, ARM Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include <Guid/MemoryAllocationHob.h>

/**
  Builds a HOB for a loaded PE32 module.

  This function builds a HOB for a loaded PE32 module.
  It can only be invoked during PEI phase;
  for MM phase, it will ASSERT() because PEI HOB is read-only for MM phase.

  If ModuleName is NULL, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().

  @param  ModuleName              The GUID File Name of the module.
  @param  MemoryAllocationModule  The 64 bit physical address of the module.
  @param  ModuleLength            The length of the module in bytes.
  @param  EntryPoint              The 64 bit physical address of the module entry point.

**/
VOID
EFIAPI
BuildModuleHob (
  IN CONST EFI_GUID        *ModuleName,
  IN EFI_PHYSICAL_ADDRESS  MemoryAllocationModule,
  IN UINT64                ModuleLength,
  IN EFI_PHYSICAL_ADDRESS  EntryPoint
  )
{
  //
  // PEI HOB is read only for MM phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB that describes a chunk of system memory.

  This function builds a HOB that describes a chunk of system memory.
  It can only be invoked during PEI phase;
  for MM phase, it will ASSERT() because PEI HOB is read-only for MM phase.

  If there is no additional space for HOB creation, then ASSERT().

  @param  ResourceType        The type of resource described by this HOB.
  @param  ResourceAttribute   The resource attributes of the memory described by this HOB.
  @param  PhysicalStart       The 64 bit physical address of memory described by this HOB.
  @param  NumberOfBytes       The length of the memory described by this HOB in bytes.

**/
VOID
EFIAPI
BuildResourceDescriptorHob (
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes
  )
{
  //
  // PEI HOB is read only for MM phase
  //
  ASSERT (FALSE);
}

/**
  Builds a customized HOB tagged with a GUID for identification and returns
  the start address of GUID HOB data.

  This function builds a customized HOB tagged with a GUID for identification
  and returns the start address of GUID HOB data so that caller can fill the customized data.
  The HOB Header and Name field is already stripped.
  It can only be invoked during PEI phase.
  For MM phase, it will ASSERT() because PEI HOB is read-only for MM phase.

  If Guid is NULL, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().
  If DataLength > (0xFFF8 - sizeof (EFI_HOB_GUID_TYPE)), then ASSERT().
  HobLength is UINT16 and multiples of 8 bytes, so the max HobLength is 0xFFF8.

  @param  Guid          The GUID to tag the customized HOB.
  @param  DataLength    The size of the data payload for the GUID HOB.

  @retval  NULL         The GUID HOB could not be allocated.
  @retval  others       The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidHob (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           DataLength
  )
{
  //
  // PEI HOB is read only for MM phase
  //
  ASSERT (FALSE);
  return NULL;
}

/**
  Builds a customized HOB tagged with a GUID for identification, copies the input data to the HOB
  data field, and returns the start address of the GUID HOB data.

  This function builds a customized HOB tagged with a GUID for identification and copies the input
  data to the HOB data field and returns the start address of the GUID HOB data.  It can only be
  invoked during PEI phase; for MM phase, it will ASSERT() because PEI HOB is read-only for MM phase.
  The HOB Header and Name field is already stripped.
  It can only be invoked during PEI phase.
  For MM phase, it will ASSERT() because PEI HOB is read-only for MM phase.

  If Guid is NULL, then ASSERT().
  If Data is NULL and DataLength > 0, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().
  If DataLength > (0xFFF8 - sizeof (EFI_HOB_GUID_TYPE)), then ASSERT().
  HobLength is UINT16 and multiples of 8 bytes, so the max HobLength is 0xFFF8.

  @param  Guid          The GUID to tag the customized HOB.
  @param  Data          The data to be copied into the data field of the GUID HOB.
  @param  DataLength    The size of the data payload for the GUID HOB.

  @retval  NULL         The GUID HOB could not be allocated.
  @retval  others       The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidDataHob (
  IN CONST EFI_GUID  *Guid,
  IN VOID            *Data,
  IN UINTN           DataLength
  )
{
  //
  // PEI HOB is read only for MM phase
  //
  ASSERT (FALSE);
  return NULL;
}

/**
  Builds a Firmware Volume HOB.

  This function builds a Firmware Volume HOB.
  It can only be invoked during PEI phase;
  for MM phase, it will ASSERT() because PEI HOB is read-only for MM phase.

  If there is no additional space for HOB creation, then ASSERT().
  If the FvImage buffer is not at its required alignment, then ASSERT().

  @param  BaseAddress   The base address of the Firmware Volume.
  @param  Length        The size of the Firmware Volume in bytes.

**/
VOID
EFIAPI
BuildFvHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  //
  // PEI HOB is read only for MM phase
  //
  ASSERT (FALSE);
}

/**
  Builds a EFI_HOB_TYPE_FV2 HOB.

  This function builds a EFI_HOB_TYPE_FV2 HOB.
  It can only be invoked during PEI phase;
  for MM phase, it will ASSERT() because PEI HOB is read-only for MM phase.

  If there is no additional space for HOB creation, then ASSERT().
  If the FvImage buffer is not at its required alignment, then ASSERT().

  @param  BaseAddress   The base address of the Firmware Volume.
  @param  Length        The size of the Firmware Volume in bytes.
  @param  FvName        The name of the Firmware Volume.
  @param  FileName      The name of the file.

**/
VOID
EFIAPI
BuildFv2Hob (
  IN          EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN          UINT64                Length,
  IN CONST    EFI_GUID              *FvName,
  IN CONST    EFI_GUID              *FileName
  )
{
  ASSERT (FALSE);
}

/**
  Builds a EFI_HOB_TYPE_FV3 HOB.

  This function builds a EFI_HOB_TYPE_FV3 HOB.
  It can only be invoked during PEI phase;
  for MM phase, it will ASSERT() since PEI HOB is read-only for MM phase.

  If there is no additional space for HOB creation, then ASSERT().
  If the FvImage buffer is not at its required alignment, then ASSERT().

  @param BaseAddress            The base address of the Firmware Volume.
  @param Length                 The size of the Firmware Volume in bytes.
  @param AuthenticationStatus   The authentication status.
  @param ExtractedFv            TRUE if the FV was extracted as a file within
                                another firmware volume. FALSE otherwise.
  @param FvName                 The name of the Firmware Volume.
                                Valid only if IsExtractedFv is TRUE.
  @param FileName               The name of the file.
                                Valid only if IsExtractedFv is TRUE.

**/
VOID
EFIAPI
BuildFv3Hob (
  IN          EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN          UINT64                Length,
  IN          UINT32                AuthenticationStatus,
  IN          BOOLEAN               ExtractedFv,
  IN CONST    EFI_GUID              *FvName  OPTIONAL,
  IN CONST    EFI_GUID              *FileName OPTIONAL
  )
{
  ASSERT (FALSE);
}

/**
  Builds a HOB for the CPU.

  This function builds a HOB for the CPU.
  It can only be invoked during PEI phase;
  for MM phase, it will ASSERT() because PEI HOB is read-only for MM phase.

  If there is no additional space for HOB creation, then ASSERT().

  @param  SizeOfMemorySpace   The maximum physical memory addressability of the processor.
  @param  SizeOfIoSpace       The maximum physical I/O addressability of the processor.

**/
VOID
EFIAPI
BuildCpuHob (
  IN UINT8  SizeOfMemorySpace,
  IN UINT8  SizeOfIoSpace
  )
{
  //
  // PEI HOB is read only for MM phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the memory allocation.

  This function builds a HOB for the memory allocation.
  It can only be invoked during PEI phase;
  for MM phase, it will ASSERT() because PEI HOB is read-only for MM phase.

  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the memory.
  @param  Length        The length of the memory allocation in bytes.
  @param  MemoryType    Type of memory allocated by this HOB.

**/
VOID
EFIAPI
BuildMemoryAllocationHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN EFI_MEMORY_TYPE       MemoryType
  )
{
  //
  // PEI HOB is read only for MM phase
  //
  ASSERT (FALSE);
}
