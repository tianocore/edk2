/** @file

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Guid/SmmCpuFeatureInfo.h>
#include <Guid/SmmProfileDataHob.h>
#include <Guid/SmmBaseHob.h>
#include <Guid/MpInformation2.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/MmCommBuffer.h>
#include <Guid/AcpiS3Context.h>
#include <Guid/UnblockRegion.h>

VOID  *mHobList;
extern EFI_PHYSICAL_ADDRESS  mMmramRanges;
extern UINT64                mMmramRangeCount;
extern EFI_PHYSICAL_ADDRESS  mMmCoreImageAddress;
extern UINT64                mMmCoreImageSize;
extern EFI_PHYSICAL_ADDRESS  mMmCoreEntryPoint;
extern EFI_PHYSICAL_ADDRESS  mMmFvBaseAddress;
extern UINT64                mMmFvSize;
extern EFI_GUID              *mMmCoreFileName;

/**
  Returns the pointer to the HOB list.

  This function returns the pointer to first HOB in the list.

  @return The pointer to the HOB list.

**/
VOID *
EFIAPI
MmIplGetHobList (
  VOID
  )
{
  ASSERT (mHobList != NULL);
  return mHobList;
}


/**
  Returns the pointer to the HOB list.

  This function returns the pointer to first HOB in the list.

  @return The pointer to the HOB list.

**/
VOID *
EFIAPI
MmIplCreateFoundationHobs (
  IN VOID   *HobList,
  IN UINTN  *HobSize
  )
{
  ASSERT (mHobList != NULL);
  return mHobList;
}

/**
  MM Ipl builds a Handoff Information Table HOB

  @param MemoryBegin     - Start Memory Address.
  @param MemoryLength    - Length of Memory.

  @return EFI_SUCCESS Always success to initialize HOB.

**/
EFI_STATUS
MmIplHobConstructor (
  IN EFI_PHYSICAL_ADDRESS  MemoryBegin,
  IN UINT64                MemoryLength
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *Hob;
  EFI_HOB_GENERIC_HEADER      *HobEnd;

  Hob                   = (VOID *)(UINTN)MemoryBegin;
  HobEnd                = (EFI_HOB_GENERIC_HEADER *)(Hob+1);
  Hob->Header.HobType   = EFI_HOB_TYPE_HANDOFF;
  Hob->Header.HobLength = (UINT16)sizeof (EFI_HOB_HANDOFF_INFO_TABLE);
  Hob->Header.Reserved  = 0;

  HobEnd->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  HobEnd->HobLength = (UINT16)sizeof (EFI_HOB_GENERIC_HEADER);
  HobEnd->Reserved  = 0;

  Hob->Version  = EFI_HOB_HANDOFF_TABLE_VERSION;

  Hob->EfiMemoryTop        = MemoryBegin + MemoryLength;
  Hob->EfiMemoryBottom     = MemoryBegin;
  Hob->EfiFreeMemoryTop    = MemoryBegin + MemoryLength;
  Hob->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS)(UINTN)(HobEnd + 1);
  Hob->EfiEndOfHobList     = (EFI_PHYSICAL_ADDRESS)(UINTN)HobEnd;

  mHobList = Hob;

  return EFI_SUCCESS;
}

/**
  Add a new HOB to the HOB List.

  @param HobType            Type of the new HOB.
  @param HobLength          Length of the new HOB to allocate.

  @return  NULL if there is no space to create a hob.
  @return  The address point to the new created hob.

**/
VOID *
EFIAPI
MmIplCreateHob (
  IN  UINT16  HobType,
  IN  UINT16  HobLength
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *HandOffHob;
  EFI_HOB_GENERIC_HEADER      *HobEnd;
  EFI_PHYSICAL_ADDRESS        FreeMemory;
  VOID                        *Hob;

  HandOffHob = MmIplGetHobList ();

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
MmIplBuildResourceDescriptorHob (
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes
  )
{
  EFI_HOB_RESOURCE_DESCRIPTOR  *Hob;

  Hob = MmIplCreateHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, sizeof (EFI_HOB_RESOURCE_DESCRIPTOR));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  Hob->ResourceType      = ResourceType;
  Hob->ResourceAttribute = ResourceAttribute;
  Hob->PhysicalStart     = PhysicalStart;
  Hob->ResourceLength    = NumberOfBytes;
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
MmIplGetNextHob (
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
MmIplGetFirstHob (
  IN UINT16  Type
  )
{
  VOID  *HobList;

  HobList = MmIplGetHobList ();
  return MmIplGetNextHob (Type, HobList);
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
MmIplGetNextGuidHob (
  IN CONST EFI_GUID  *Guid,
  IN CONST VOID      *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  GuidHob;

  GuidHob.Raw = (UINT8 *)HobStart;
  while ((GuidHob.Raw = MmIplGetNextHob (EFI_HOB_TYPE_GUID_EXTENSION, GuidHob.Raw)) != NULL) {
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
MmIplGetFirstGuidHob (
  IN CONST EFI_GUID  *Guid
  )
{
  VOID  *HobList;

  HobList = MmIplGetHobList ();
  return MmIplGetNextGuidHob (Guid, HobList);
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
MmIplBuildModuleHob (
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

  Hob = MmIplCreateHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE));
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
MmIplBuildGuidHob (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           DataLength
  )
{
  EFI_HOB_GUID_TYPE  *Hob;

  //
  // Make sure that data length is not too long.
  //
  ASSERT (DataLength <= (0xffff - sizeof (EFI_HOB_GUID_TYPE)));

  Hob = MmIplCreateHob (EFI_HOB_TYPE_GUID_EXTENSION, (UINT16)(sizeof (EFI_HOB_GUID_TYPE) + DataLength));
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
MmIplBuildGuidDataHob (
  IN CONST EFI_GUID  *Guid,
  IN VOID            *Data,
  IN UINTN           DataLength
  )
{
  VOID  *HobData;

  ASSERT (Data != NULL || DataLength == 0);

  HobData = MmIplBuildGuidHob (Guid, DataLength);

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
MmIplBuildFvHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  EFI_HOB_FIRMWARE_VOLUME  *Hob;

  Hob = MmIplCreateHob (EFI_HOB_TYPE_FV, sizeof (EFI_HOB_FIRMWARE_VOLUME));
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
MmIplBuildFv2Hob (
  IN          EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN          UINT64                Length,
  IN CONST    EFI_GUID              *FvName,
  IN CONST    EFI_GUID              *FileName
  )
{
  EFI_HOB_FIRMWARE_VOLUME2  *Hob;

  Hob = MmIplCreateHob (EFI_HOB_TYPE_FV2, sizeof (EFI_HOB_FIRMWARE_VOLUME2));
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
MmIplBuildFv3Hob (
  IN          EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN          UINT64                Length,
  IN          UINT32                AuthenticationStatus,
  IN          BOOLEAN               ExtractedFv,
  IN CONST    EFI_GUID              *FvName  OPTIONAL,
  IN CONST    EFI_GUID              *FileName OPTIONAL
  )
{
  EFI_HOB_FIRMWARE_VOLUME3  *Hob;

  Hob = MmIplCreateHob (EFI_HOB_TYPE_FV3, sizeof (EFI_HOB_FIRMWARE_VOLUME3));
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
MmIplBuildCpuHob (
  IN UINT8  SizeOfMemorySpace,
  IN UINT8  SizeOfIoSpace
  )
{
  EFI_HOB_CPU  *Hob;

  Hob = MmIplCreateHob (EFI_HOB_TYPE_CPU, sizeof (EFI_HOB_CPU));
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
MmIplBuildStackHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  EFI_HOB_MEMORY_ALLOCATION_STACK  *Hob;

  ASSERT (
    ((BaseAddress & (EFI_PAGE_SIZE - 1)) == 0) &&
    ((Length & (EFI_PAGE_SIZE - 1)) == 0)
    );

  Hob = MmIplCreateHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, sizeof (EFI_HOB_MEMORY_ALLOCATION_STACK));
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
  Builds a HOB for the memory allocation.

  This function builds a HOB for the memory allocation.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  Name          The name guid of the memory.
  @param  BaseAddress   The 64 bit physical address of the memory.
  @param  Length        The length of the memory allocation in bytes.
  @param  MemoryType    Type of memory allocated by this HOB.

**/
VOID
EFIAPI
MmIplBuildMemoryAllocationHob (
  IN CONST EFI_GUID        *Name,
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

  Hob = MmIplCreateHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, sizeof (EFI_HOB_MEMORY_ALLOCATION));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  ZeroMem (&(Hob->AllocDescriptor.Name), sizeof (EFI_GUID));
  if (Name != NULL) {
    CopyGuid (&Hob->AllocDescriptor.Name, Name);
  }
  Hob->AllocDescriptor.MemoryBaseAddress = BaseAddress;
  Hob->AllocDescriptor.MemoryLength      = Length;
  Hob->AllocDescriptor.MemoryType        = MemoryType;
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
EFIAPI
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
      MmIplBuildMemoryAllocationHob (
        NULL,
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
MmIplBuildMmCoreModuleHob (
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

  Hob = MmIplCreateHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  CopyGuid (&(Hob->MemoryAllocationHeader.Name), &gEfiHobMemoryAllocModuleGuid);
  Hob->MemoryAllocationHeader.MemoryBaseAddress = MemoryAllocationModule;
  Hob->MemoryAllocationHeader.MemoryLength      = ModuleLength;
  Hob->MemoryAllocationHeader.MemoryType        = EfiReservedMemoryType;

  //
  // Zero the reserved space to match HOB spec
  //
  ZeroMem (Hob->MemoryAllocationHeader.Reserved, sizeof (Hob->MemoryAllocationHeader.Reserved));

  CopyGuid (&Hob->ModuleName, ModuleName);
  Hob->EntryPoint = EntryPoint;
}

/**
  Create the MM foundation specific HOB list which StandaloneMm Core needed.

  This function build the MM foundation specific HOB list needed by StandaloneMm Core
  based on the PEI HOB list.

  @param[in]      Buffer            The free buffer to be used for HOB creation.
  @param[in, out] BufferSize        The buffer size.
                                    On return, the expected/used size.

  @retval RETURN_INVALID_PARAMETER  BufferSize is NULL.
  @retval RETURN_BUFFER_TOO_SMALL   The buffer is too small for HOB creation.
                                    BufferSize is updated to indicate the expected buffer size.
                                    When the input BufferSize is bigger than the expected buffer size,
                                    the BufferSize value will be changed the used buffer size.
  @retval RETURN_SUCCESS            HOB List is created/updated successfully or the input Length is 0.

**/
EFI_STATUS
EFIAPI
CreateMmFoundationHobList (
  IN VOID       *Buffer,
  IN OUT UINTN  *BufferSize
  )
{
  VOID                   *GuidHob;
  VOID                   *HobData;
  UINTN                  RequiredSize;
  EFI_PEI_HOB_POINTERS   SmmProfileDataHob;

  if (BufferSize == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((*BufferSize != 0) && (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  RequiredSize = 0;

  //
  // For communication buffer
  //
  GuidHob = GetFirstGuidHob (&gEdkiiCommunicationBufferGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    if ((*BufferSize == 0) && (Buffer == NULL)) {
      RequiredSize += sizeof (EFI_HOB_GUID_TYPE) + sizeof (MM_COMM_BUFFER_DATA);
    } else {
      HobData = GET_GUID_HOB_DATA (GuidHob);
      MmIplBuildGuidDataHob (&gEdkiiCommunicationBufferGuid, HobData, sizeof (MM_COMM_BUFFER_DATA));
    }
  }

  //
  // For MmCore ImageAddress, ImageSize, EntryPoint
  //
  if ((*BufferSize == 0) && (Buffer == NULL)) {
    RequiredSize += sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE);
  } else {
    MmIplBuildMmCoreModuleHob (mMmCoreFileName, mMmCoreImageAddress, mMmCoreImageSize, mMmCoreEntryPoint);
  }

  //
  // BFV address for Standalone Core
  //
  if ((*BufferSize == 0) && (Buffer == NULL)) {
    RequiredSize += sizeof (EFI_HOB_FIRMWARE_VOLUME);
  } else {
    MmIplBuildFvHob (mMmFvBaseAddress, mMmFvSize);
  }

  //
  // CPU feature info, open source did not have gSmmCpuFeatureInfoGuid -> gEdkiiSmmCpuFeatureInfoHobGuid
  //
  GuidHob = GetFirstGuidHob (&gEdkiiSmmCpuFeatureInfoHobGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    if ((*BufferSize == 0) && (Buffer == NULL)) {
      RequiredSize += sizeof (EFI_HOB_GUID_TYPE) + sizeof (SMM_CPU_FEATURE_INFO_HOB);
    } else {
      HobData = GET_GUID_HOB_DATA (GuidHob);
      MmIplBuildGuidDataHob (&gEdkiiSmmCpuFeatureInfoHobGuid, HobData, sizeof (SMM_CPU_FEATURE_INFO_HOB));
    }
  }

  //
  // gSmmBaseHobGuid（multiple instance） under UefiCpuPkg
  //
  GuidHob = GetFirstGuidHob (&gSmmBaseHobGuid);
  ASSERT (GuidHob != NULL);
  while (GuidHob != NULL) {
    HobData = GET_GUID_HOB_DATA (GuidHob);
    if ((*BufferSize == 0) && (Buffer == NULL)) {
      RequiredSize += sizeof (EFI_HOB_GUID_TYPE) + sizeof (SMM_BASE_HOB_DATA) + sizeof (UINT64) * ((SMM_BASE_HOB_DATA *)HobData)->NumberOfProcessors;
    } else {
      MmIplBuildGuidDataHob (&gSmmBaseHobGuid, HobData, sizeof (SMM_BASE_HOB_DATA) + sizeof (UINT64) * ((SMM_BASE_HOB_DATA *)HobData)->NumberOfProcessors);
    }
    GuidHob        = GetNextGuidHob (&gSmmBaseHobGuid, GET_NEXT_HOB (GuidHob));
  }

  //
  // gEfiSmmSmramMemoryGuid 
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmSmramMemoryGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    if ((*BufferSize == 0) && (Buffer == NULL)) {
      RequiredSize += sizeof (EFI_HOB_GUID_TYPE) + sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK) + (mMmramRangeCount * sizeof (EFI_SMRAM_DESCRIPTOR));
    } else {
      HobData = GET_GUID_HOB_DATA (GuidHob);
      MmIplBuildGuidDataHob (&gEfiSmmSmramMemoryGuid, HobData, (sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK) + (mMmramRangeCount * sizeof (EFI_SMRAM_DESCRIPTOR))));
    }
  }

  //
  // gMpInformation2HobGuid(multiple instance) 
  //
  GuidHob = GetFirstGuidHob (&gMpInformation2HobGuid);
  ASSERT (GuidHob != NULL);
  while (GuidHob != NULL) {
    HobData = GET_GUID_HOB_DATA (GuidHob);
    if ((*BufferSize == 0) && (Buffer == NULL)) {
      RequiredSize += sizeof (EFI_HOB_GUID_TYPE) + sizeof (MP_INFORMATION2_HOB_DATA) + sizeof (MP_INFORMATION2_ENTRY) * ((MP_INFORMATION2_HOB_DATA *)HobData)->NumberOfProcessors;
    } else {
      MmIplBuildGuidDataHob (&gMpInformation2HobGuid, HobData, sizeof (MP_INFORMATION2_HOB_DATA) + sizeof (MP_INFORMATION2_ENTRY) * ((MP_INFORMATION2_HOB_DATA *)HobData)->NumberOfProcessors);
    }
    GuidHob        = GetNextGuidHob (&gMpInformation2HobGuid, GET_NEXT_HOB (GuidHob));
  }

  //
  // gEdkiiSmmProfileDataGuid
  //
  if ((*BufferSize == 0) && (Buffer == NULL)) {
    RequiredSize += sizeof (EFI_HOB_MEMORY_ALLOCATION);
  } else {
    //
    // Searching for gEdkiiSmmProfileDataGuid
    //
    SmmProfileDataHob.Raw = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
    while (SmmProfileDataHob.Raw != NULL) {
      //
      // Find gEdkiiSmmProfileDataGuid
      //
      if (CompareGuid (&SmmProfileDataHob.MemoryAllocation->AllocDescriptor.Name, &gEdkiiSmmProfileDataGuid)) {
        break;
      }

      SmmProfileDataHob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (SmmProfileDataHob));
    }

    ASSERT (SmmProfileDataHob.Raw != NULL);

    MmIplBuildMemoryAllocationHob (
      &SmmProfileDataHob.MemoryAllocation->AllocDescriptor.Name,
      SmmProfileDataHob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
      SmmProfileDataHob.MemoryAllocation->AllocDescriptor.MemoryLength,
      SmmProfileDataHob.MemoryAllocation->AllocDescriptor.MemoryType
      );
  }

  //
  // gEfiAcpiVariableGuid
  //
  GuidHob = GetFirstGuidHob (&gEfiAcpiVariableGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    if ((*BufferSize == 0) && (Buffer == NULL)) {
      RequiredSize += sizeof (EFI_HOB_GUID_TYPE) + sizeof(EFI_SMRAM_DESCRIPTOR);
    } else {
      HobData = GET_GUID_HOB_DATA (GuidHob);
      MmIplBuildGuidDataHob (&gEfiAcpiVariableGuid, HobData, sizeof (EFI_SMRAM_DESCRIPTOR));
    }
  }

  //
  // Reserouce Hobs for unblockoed memory regions (Multiple instance)
  //
  GuidHob = GetFirstGuidHob (&gMmUnblockRegionHobGuid);
  ASSERT (GuidHob != NULL);
  while (GuidHob != NULL) {
    if ((*BufferSize == 0) && (Buffer == NULL)) {
      RequiredSize += sizeof (EFI_HOB_GUID_TYPE) + sizeof (MM_UNBLOCK_REGION);
    } else {
      HobData = GET_GUID_HOB_DATA (GuidHob);
      MmIplBuildGuidDataHob (&gMmUnblockRegionHobGuid, HobData, sizeof (MM_UNBLOCK_REGION));
    }
    GuidHob        = GetNextGuidHob (&gMmUnblockRegionHobGuid, GET_NEXT_HOB (GuidHob));
  }


  if (*BufferSize < RequiredSize) {
    *BufferSize = RequiredSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  return EFI_SUCCESS;
}

/**
  Builds a end of list HOB.

  This function builds a HOB for end of the HOB list.

  @param  EndOfHobList            End of HOB list address.

**/
VOID
EFIAPI
CreateEndOfList (
  IN EFI_PHYSICAL_ADDRESS  EndOfHobList
  )
 {
  EFI_HOB_GENERIC_HEADER      *HobEnd;

  HobEnd            = (EFI_HOB_GENERIC_HEADER *)(UINTN)EndOfHobList;
  HobEnd->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  HobEnd->HobLength = (UINT16)sizeof (EFI_HOB_GENERIC_HEADER);
  HobEnd->Reserved  = 0;
 }