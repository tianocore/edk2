/** @file

  Copyright (c) 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeCoffLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PrePiHobListPointerLib.h>

#include <Protocol/PeCoffLoader.h>
#include <Guid/ExtractSection.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/MemoryAllocationHob.h>

VOID
BuildMemoryTypeInformationHob (
  VOID
  );

/**
  Returns the pointer to the HOB list.

  This function returns the pointer to first HOB in the list.

  @return The pointer to the HOB list.

**/
VOID *
EFIAPI
GetHobList (
  VOID
  )
{
  return PrePeiGetHobList ();
}

/**
  Updates the pointer to the HOB list.

  @param  HobList       Hob list pointer to store

**/
EFI_STATUS
EFIAPI
SetHobList (
  IN  VOID  *HobList
  )
{
  return PrePeiSetHobList (HobList);
}

/**


**/
EFI_HOB_HANDOFF_INFO_TABLE *
HobConstructor (
  IN VOID   *EfiMemoryBegin,
  IN UINTN  EfiMemoryLength,
  IN VOID   *EfiFreeMemoryBottom,
  IN VOID   *EfiFreeMemoryTop
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *Hob;
  EFI_HOB_GENERIC_HEADER      *HobEnd;

  Hob    = EfiFreeMemoryBottom;
  HobEnd = (EFI_HOB_GENERIC_HEADER *)(Hob+1);

  Hob->Header.HobType   = EFI_HOB_TYPE_HANDOFF;
  Hob->Header.HobLength = sizeof (EFI_HOB_HANDOFF_INFO_TABLE);
  Hob->Header.Reserved  = 0;

  HobEnd->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  HobEnd->HobLength = sizeof (EFI_HOB_GENERIC_HEADER);
  HobEnd->Reserved  = 0;

  Hob->Version  = EFI_HOB_HANDOFF_TABLE_VERSION;
  Hob->BootMode = BOOT_WITH_FULL_CONFIGURATION;

  Hob->EfiMemoryTop        = (UINTN)EfiMemoryBegin + EfiMemoryLength;
  Hob->EfiMemoryBottom     = (UINTN)EfiMemoryBegin;
  Hob->EfiFreeMemoryTop    = (UINTN)EfiFreeMemoryTop;
  Hob->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS)(UINTN)(HobEnd+1);
  Hob->EfiEndOfHobList     = (EFI_PHYSICAL_ADDRESS)(UINTN)HobEnd;

  return Hob;
}

VOID *
CreateHob (
  IN  UINT16  HobType,
  IN  UINT16  HobLength
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *HandOffHob;
  EFI_HOB_GENERIC_HEADER      *HobEnd;
  EFI_PHYSICAL_ADDRESS        FreeMemory;
  VOID                        *Hob;

  HandOffHob = GetHobList ();

  //
  // Check Length to avoid data overflow.
  //
  if (HobLength > MAX_UINT16 - 0x7) {
    return NULL;
  }

  HobLength = (UINT16)((HobLength + 0x7) & (~0x7));

  FreeMemory = HandOffHob->EfiFreeMemoryTop - HandOffHob->EfiFreeMemoryBottom;

  if (FreeMemory < HobLength) {
    return NULL;
  }

  Hob                                        = (VOID *)(UINTN)HandOffHob->EfiEndOfHobList;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->HobType   = HobType;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->HobLength = HobLength;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->Reserved  = 0;

  HobEnd                      = (EFI_HOB_GENERIC_HEADER *)((UINTN)Hob + HobLength);
  HandOffHob->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS)(UINTN)HobEnd;

  HobEnd->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  HobEnd->HobLength = sizeof (EFI_HOB_GENERIC_HEADER);
  HobEnd->Reserved  = 0;
  HobEnd++;
  HandOffHob->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS)(UINTN)HobEnd;

  return Hob;
}

/**
  Builds a HOB that describes a chunk of system memory.

  This function builds a HOB that describes a chunk of system memory.
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
  EFI_HOB_RESOURCE_DESCRIPTOR  *Hob;

  Hob = CreateHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, sizeof (EFI_HOB_RESOURCE_DESCRIPTOR));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  Hob->ResourceType      = ResourceType;
  Hob->ResourceAttribute = ResourceAttribute;
  Hob->PhysicalStart     = PhysicalStart;
  Hob->ResourceLength    = NumberOfBytes;
}

VOID
EFIAPI
BuildFvHobs (
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  *ResourceAttribute
  )
{
  EFI_RESOURCE_ATTRIBUTE_TYPE  Resource;

  BuildFvHob (PhysicalStart, NumberOfBytes);

  if (ResourceAttribute == NULL) {
    Resource = (EFI_RESOURCE_ATTRIBUTE_PRESENT    |
                EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                EFI_RESOURCE_ATTRIBUTE_TESTED |
                EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE);
  } else {
    Resource = *ResourceAttribute;
  }

  BuildResourceDescriptorHob (EFI_RESOURCE_FIRMWARE_DEVICE, Resource, PhysicalStart, NumberOfBytes);
}

/**
  Returns the next instance of a HOB type from the starting HOB.

  This function searches the first instance of a HOB type from the starting HOB pointer.
  If there does not exist such HOB type from the starting HOB pointer, it will return NULL.
  In contrast with macro GET_NEXT_HOB(), this function does not skip the starting HOB pointer
  unconditionally: it returns HobStart back if HobStart itself meets the requirement;
  caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.
  If HobStart is NULL, then ASSERT().

  @param  Type          The HOB type to return.
  @param  HobStart      The starting HOB pointer to search from.

  @return The next instance of a HOB type from the starting HOB.

**/
VOID *
EFIAPI
GetNextHob (
  IN UINT16      Type,
  IN CONST VOID  *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  ASSERT (HobStart != NULL);

  Hob.Raw = (UINT8 *)HobStart;
  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == Type) {
      return Hob.Raw;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return NULL;
}

/**
  Returns the first instance of a HOB type among the whole HOB list.

  This function searches the first instance of a HOB type among the whole HOB list.
  If there does not exist such HOB type in the HOB list, it will return NULL.

  @param  Type          The HOB type to return.

  @return The next instance of a HOB type from the starting HOB.

**/
VOID *
EFIAPI
GetFirstHob (
  IN UINT16  Type
  )
{
  VOID  *HobList;

  HobList = GetHobList ();
  return GetNextHob (Type, HobList);
}

/**
  This function searches the first instance of a HOB from the starting HOB pointer.
  Such HOB should satisfy two conditions:
  its HOB type is EFI_HOB_TYPE_GUID_EXTENSION and its GUID Name equals to the input Guid.
  If there does not exist such HOB from the starting HOB pointer, it will return NULL.
  Caller is required to apply GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE ()
  to extract the data section and its size info respectively.
  In contrast with macro GET_NEXT_HOB(), this function does not skip the starting HOB pointer
  unconditionally: it returns HobStart back if HobStart itself meets the requirement;
  caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.
  If Guid is NULL, then ASSERT().
  If HobStart is NULL, then ASSERT().

  @param  Guid          The GUID to match with in the HOB list.
  @param  HobStart      A pointer to a Guid.

  @return The next instance of the matched GUID HOB from the starting HOB.

**/
VOID *
EFIAPI
GetNextGuidHob (
  IN CONST EFI_GUID  *Guid,
  IN CONST VOID      *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  GuidHob;

  GuidHob.Raw = (UINT8 *)HobStart;
  while ((GuidHob.Raw = GetNextHob (EFI_HOB_TYPE_GUID_EXTENSION, GuidHob.Raw)) != NULL) {
    if (CompareGuid (Guid, &GuidHob.Guid->Name)) {
      break;
    }

    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
  }

  return GuidHob.Raw;
}

/**
  This function searches the first instance of a HOB among the whole HOB list.
  Such HOB should satisfy two conditions:
  its HOB type is EFI_HOB_TYPE_GUID_EXTENSION and its GUID Name equals to the input Guid.
  If there does not exist such HOB from the starting HOB pointer, it will return NULL.
  Caller is required to apply GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE ()
  to extract the data section and its size info respectively.
  If Guid is NULL, then ASSERT().

  @param  Guid          The GUID to match with in the HOB list.

  @return The first instance of the matched GUID HOB among the whole HOB list.

**/
VOID *
EFIAPI
GetFirstGuidHob (
  IN CONST EFI_GUID  *Guid
  )
{
  VOID  *HobList;

  HobList = GetHobList ();
  return GetNextGuidHob (Guid, HobList);
}

/**
  This service enables PEIMs to ascertain the present value of the boot mode.


  @retval BootMode

**/
EFI_BOOT_MODE
EFIAPI
GetBootMode (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = GetHobList ();
  return Hob.HandoffInformationTable->BootMode;
}

/**
  This service enables PEIMs to update the boot mode variable.

  @param  BootMode              The value of the boot mode to set.

  @retval EFI_SUCCESS           The value was successfully updated

**/
EFI_STATUS
EFIAPI
SetBootMode (
  IN  EFI_BOOT_MODE  BootMode
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw                               = GetHobList ();
  Hob.HandoffInformationTable->BootMode = BootMode;
  return EFI_SUCCESS;
}

/**
  Builds a HOB for a loaded PE32 module.

  This function builds a HOB for a loaded PE32 module.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
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
  EFI_HOB_MEMORY_ALLOCATION_MODULE  *Hob;

  ASSERT (
    ((MemoryAllocationModule & (EFI_PAGE_SIZE - 1)) == 0) &&
    ((ModuleLength & (EFI_PAGE_SIZE - 1)) == 0)
    );

  Hob = CreateHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  CopyGuid (&(Hob->MemoryAllocationHeader.Name), &gEfiHobMemoryAllocModuleGuid);
  Hob->MemoryAllocationHeader.MemoryBaseAddress = MemoryAllocationModule;
  Hob->MemoryAllocationHeader.MemoryLength      = ModuleLength;
  Hob->MemoryAllocationHeader.MemoryType        = EfiBootServicesCode;

  //
  // Zero the reserved space to match HOB spec
  //
  ZeroMem (Hob->MemoryAllocationHeader.Reserved, sizeof (Hob->MemoryAllocationHeader.Reserved));

  CopyGuid (&Hob->ModuleName, ModuleName);
  Hob->EntryPoint = EntryPoint;
}

/**
  Builds a GUID HOB with a certain data length.

  This function builds a customized HOB tagged with a GUID for identification
  and returns the start address of GUID HOB data so that caller can fill the customized data.
  The HOB Header and Name field is already stripped.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If Guid is NULL, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().
  If DataLength >= (0x10000 - sizeof (EFI_HOB_GUID_TYPE)), then ASSERT().

  @param  Guid          The GUID to tag the customized HOB.
  @param  DataLength    The size of the data payload for the GUID HOB.

  @return The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidHob (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           DataLength
  )
{
  EFI_HOB_GUID_TYPE  *Hob;

  //
  // Make sure that data length is not too long.
  //
  ASSERT (DataLength <= (0xffff - sizeof (EFI_HOB_GUID_TYPE)));

  Hob = CreateHob (EFI_HOB_TYPE_GUID_EXTENSION, (UINT16)(sizeof (EFI_HOB_GUID_TYPE) + DataLength));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return NULL;
  }

  CopyGuid (&Hob->Name, Guid);
  return Hob + 1;
}

/**
  Copies a data buffer to a newly-built HOB.

  This function builds a customized HOB tagged with a GUID for identification,
  copies the input data to the HOB data field and returns the start address of the GUID HOB data.
  The HOB Header and Name field is already stripped.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If Guid is NULL, then ASSERT().
  If Data is NULL and DataLength > 0, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().
  If DataLength >= (0x10000 - sizeof (EFI_HOB_GUID_TYPE)), then ASSERT().

  @param  Guid          The GUID to tag the customized HOB.
  @param  Data          The data to be copied into the data field of the GUID HOB.
  @param  DataLength    The size of the data payload for the GUID HOB.

  @return The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidDataHob (
  IN CONST EFI_GUID  *Guid,
  IN VOID            *Data,
  IN UINTN           DataLength
  )
{
  VOID  *HobData;

  ASSERT (Data != NULL || DataLength == 0);

  HobData = BuildGuidHob (Guid, DataLength);

  return CopyMem (HobData, Data, DataLength);
}

/**
  Builds a Firmware Volume HOB.

  This function builds a Firmware Volume HOB.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

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
  EFI_HOB_FIRMWARE_VOLUME  *Hob;

  Hob = CreateHob (EFI_HOB_TYPE_FV, sizeof (EFI_HOB_FIRMWARE_VOLUME));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  Hob->BaseAddress = BaseAddress;
  Hob->Length      = Length;
}

/**
  Builds a EFI_HOB_TYPE_FV2 HOB.

  This function builds a EFI_HOB_TYPE_FV2 HOB.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Firmware Volume.
  @param  Length        The size of the Firmware Volume in bytes.
  @param  FvName       The name of the Firmware Volume.
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
  EFI_HOB_FIRMWARE_VOLUME2  *Hob;

  Hob = CreateHob (EFI_HOB_TYPE_FV2, sizeof (EFI_HOB_FIRMWARE_VOLUME2));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  Hob->BaseAddress = BaseAddress;
  Hob->Length      = Length;
  CopyGuid (&Hob->FvName, FvName);
  CopyGuid (&Hob->FileName, FileName);
}

/**
  Builds a EFI_HOB_TYPE_FV3 HOB.

  This function builds a EFI_HOB_TYPE_FV3 HOB.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.

  If there is no additional space for HOB creation, then ASSERT().

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
  EFI_HOB_FIRMWARE_VOLUME3  *Hob;

  Hob = CreateHob (EFI_HOB_TYPE_FV3, sizeof (EFI_HOB_FIRMWARE_VOLUME3));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  Hob->BaseAddress          = BaseAddress;
  Hob->Length               = Length;
  Hob->AuthenticationStatus = AuthenticationStatus;
  Hob->ExtractedFv          = ExtractedFv;
  if (ExtractedFv) {
    CopyGuid (&Hob->FvName, FvName);
    CopyGuid (&Hob->FileName, FileName);
  }
}

/**
  Builds a Capsule Volume HOB.

  This function builds a Capsule Volume HOB.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Capsule Volume.
  @param  Length        The size of the Capsule Volume in bytes.

**/
VOID
EFIAPI
BuildCvHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  ASSERT (FALSE);
}

/**
  Builds a HOB for the CPU.

  This function builds a HOB for the CPU.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
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
  EFI_HOB_CPU  *Hob;

  Hob = CreateHob (EFI_HOB_TYPE_CPU, sizeof (EFI_HOB_CPU));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  Hob->SizeOfMemorySpace = SizeOfMemorySpace;
  Hob->SizeOfIoSpace     = SizeOfIoSpace;

  //
  // Zero the reserved space to match HOB spec
  //
  ZeroMem (Hob->Reserved, sizeof (Hob->Reserved));
}

/**
  Builds a HOB for the Stack.

  This function builds a HOB for the stack.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the Stack.
  @param  Length        The length of the stack in bytes.

**/
VOID
EFIAPI
BuildStackHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  EFI_HOB_MEMORY_ALLOCATION_STACK  *Hob;

  ASSERT (
    ((BaseAddress & (EFI_PAGE_SIZE - 1)) == 0) &&
    ((Length & (EFI_PAGE_SIZE - 1)) == 0)
    );

  Hob = CreateHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, sizeof (EFI_HOB_MEMORY_ALLOCATION_STACK));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  CopyGuid (&(Hob->AllocDescriptor.Name), &gEfiHobMemoryAllocStackGuid);
  Hob->AllocDescriptor.MemoryBaseAddress = BaseAddress;
  Hob->AllocDescriptor.MemoryLength      = Length;
  Hob->AllocDescriptor.MemoryType        = EfiBootServicesData;

  //
  // Zero the reserved space to match HOB spec
  //
  ZeroMem (Hob->AllocDescriptor.Reserved, sizeof (Hob->AllocDescriptor.Reserved));
}

/**
  Update the Stack Hob if the stack has been moved

  @param  BaseAddress   The 64 bit physical address of the Stack.
  @param  Length        The length of the stack in bytes.

**/
VOID
UpdateStackHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = GetHobList ();
  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
    if (CompareGuid (&gEfiHobMemoryAllocStackGuid, &(Hob.MemoryAllocationStack->AllocDescriptor.Name))) {
      //
      // Build a new memory allocation HOB with old stack info with EfiConventionalMemory type
      // to be reclaimed by DXE core.
      //
      BuildMemoryAllocationHob (
        Hob.MemoryAllocationStack->AllocDescriptor.MemoryBaseAddress,
        Hob.MemoryAllocationStack->AllocDescriptor.MemoryLength,
        EfiConventionalMemory
        );
      //
      // Update the BSP Stack Hob to reflect the new stack info.
      //
      Hob.MemoryAllocationStack->AllocDescriptor.MemoryBaseAddress = BaseAddress;
      Hob.MemoryAllocationStack->AllocDescriptor.MemoryLength      = Length;
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }
}

/**
  Builds a HOB for the memory allocation.

  This function builds a HOB for the memory allocation.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
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
  EFI_HOB_MEMORY_ALLOCATION  *Hob;

  ASSERT (
    ((BaseAddress & (EFI_PAGE_SIZE - 1)) == 0) &&
    ((Length & (EFI_PAGE_SIZE - 1)) == 0)
    );

  Hob = CreateHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, sizeof (EFI_HOB_MEMORY_ALLOCATION));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  ZeroMem (&(Hob->AllocDescriptor.Name), sizeof (EFI_GUID));
  Hob->AllocDescriptor.MemoryBaseAddress = BaseAddress;
  Hob->AllocDescriptor.MemoryLength      = Length;
  Hob->AllocDescriptor.MemoryType        = MemoryType;
  //
  // Zero the reserved space to match HOB spec
  //
  ZeroMem (Hob->AllocDescriptor.Reserved, sizeof (Hob->AllocDescriptor.Reserved));
}

VOID
EFIAPI
BuildExtractSectionHob (
  IN  EFI_GUID                                 *Guid,
  IN  EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER  SectionGetInfo,
  IN  EXTRACT_GUIDED_SECTION_DECODE_HANDLER    SectionExtraction
  )
{
  EXTRACT_SECTION_DATA  Data;

  Data.SectionGetInfo    = SectionGetInfo;
  Data.SectionExtraction = SectionExtraction;
  BuildGuidDataHob (Guid, &Data, sizeof (Data));
}

PE_COFF_LOADER_PROTOCOL  gPeCoffProtocol = {
  PeCoffLoaderGetImageInfo,
  PeCoffLoaderLoadImage,
  PeCoffLoaderRelocateImage,
  PeCoffLoaderImageReadFromMemory,
  PeCoffLoaderRelocateImageForRuntime,
  PeCoffLoaderUnloadImage
};

VOID
EFIAPI
BuildPeCoffLoaderHob (
  VOID
  )
{
  VOID  *Ptr;

  Ptr = &gPeCoffProtocol;
  BuildGuidDataHob (&gPeCoffLoaderProtocolGuid, &Ptr, sizeof (VOID *));
}

// May want to put this into a library so you only need the PCD settings if you are using the feature?
VOID
BuildMemoryTypeInformationHob (
  VOID
  )
{
  EFI_MEMORY_TYPE_INFORMATION  Info[10];

  Info[0].Type          = EfiACPIReclaimMemory;
  Info[0].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIReclaimMemory);
  Info[1].Type          = EfiACPIMemoryNVS;
  Info[1].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIMemoryNVS);
  Info[2].Type          = EfiReservedMemoryType;
  Info[2].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiReservedMemoryType);
  Info[3].Type          = EfiRuntimeServicesData;
  Info[3].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesData);
  Info[4].Type          = EfiRuntimeServicesCode;
  Info[4].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesCode);
  Info[5].Type          = EfiBootServicesCode;
  Info[5].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiBootServicesCode);
  Info[6].Type          = EfiBootServicesData;
  Info[6].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiBootServicesData);
  Info[7].Type          = EfiLoaderCode;
  Info[7].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiLoaderCode);
  Info[8].Type          = EfiLoaderData;
  Info[8].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiLoaderData);

  // Terminator for the list
  Info[9].Type          = EfiMaxMemoryType;
  Info[9].NumberOfPages = 0;

  BuildGuidDataHob (&gEfiMemoryTypeInformationGuid, &Info, sizeof (Info));
}
