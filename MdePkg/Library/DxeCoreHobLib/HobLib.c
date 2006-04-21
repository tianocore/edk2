/** @file
  HOB Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  HobLib.c

**/



extern VOID *gHobList;

/**
  Returns the pointer to the HOB list.

  @return The pointer to the HOB list.

**/
VOID *
EFIAPI
GetHobList (
  VOID
  )
{
  return gHobList;
}

/**
  This function searches the first instance of a HOB type from the starting HOB pointer. 
  If there does not exist such HOB type from the starting HOB pointer, it will return NULL. 

  @param  Type The HOB type to return.
  @param  HobStart The starting HOB pointer to search from.

  @return The next instance of a HOB type from the starting HOB.

**/
VOID *
EFIAPI
GetNextHob (
  IN UINT16                 Type,
  IN CONST VOID             *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  ASSERT (HobStart != NULL);
   
  Hob.Raw = (UINT8 *) HobStart;
  //
  // Parse the HOB list, stop if end of list or matching type found.
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
  This function searches the first instance of a HOB type among the whole HOB list. 
  If there does not exist such HOB type in the HOB list, it will return NULL. 

  @param  Type The HOB type to return.

  @return The next instance of a HOB type from the starting HOB.

**/
VOID *
EFIAPI
GetFirstHob (
  IN UINT16                 Type
  )
{
  VOID      *HobList;

  HobList = GetHobList ();
  return GetNextHob (Type, HobList);
}

/**
  This function searches the first instance of a HOB from the starting HOB pointer. 
  Such HOB should satisfy two conditions: 
  its HOB type is EFI_HOB_TYPE_GUID_EXTENSION and its GUID Name equals to the input Guid. 
  If there does not exist such HOB from the starting HOB pointer, it will return NULL. 

  @param  Guid The GUID to match with in the HOB list.
  @param  HobStart A pointer to a Guid.

  @return The next instance of the matched GUID HOB from the starting HOB.

**/
VOID *
EFIAPI
GetNextGuidHob (
  IN CONST EFI_GUID         *Guid,
  IN CONST VOID             *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  GuidHob;

  GuidHob.Raw = (UINT8 *) HobStart;
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

  @param  Guid The GUID to match with in the HOB list.

  @return The first instance of the matched GUID HOB among the whole HOB list.

**/
VOID *
EFIAPI
GetFirstGuidHob (
  IN CONST EFI_GUID         *Guid
  )
{
  VOID      *HobList;

  HobList = GetHobList ();
  return GetNextGuidHob (Guid, HobList);
}

/**
  This function builds a HOB for a loaded PE32 module.

  @param  ModuleName The GUID File Name of the module.
  @param  MemoryAllocationModule The 64 bit physical address of the module.
  @param  ModuleLength The length of the module in bytes.
  @param  EntryPoint The 64 bit physical address of the module’s entry point.

**/
VOID
EFIAPI
BuildModuleHob (
  IN CONST EFI_GUID         *ModuleName,
  IN EFI_PHYSICAL_ADDRESS   MemoryAllocationModule,
  IN UINT64                 ModuleLength,
  IN EFI_PHYSICAL_ADDRESS   EntryPoint
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB that describes a chunk of system memory.

  @param  ResourceType The type of resource described by this HOB.
  @param  ResourceAttribute The resource attributes of the memory described by this HOB.
  @param  PhysicalStart The 64 bit physical address of memory described by this HOB.
  @param  NumberOfBytes The length of the memory described by this HOB in bytes.

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
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  This function builds a customized HOB tagged with a GUID for identification 
  and returns the start address of GUID HOB data so that caller can fill the customized data. 

  @param  Guid The GUID to tag the customized HOB.
  @param  DataLength The size of the data payload for the GUID HOB.

  @return The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidHob (
  IN CONST EFI_GUID              *Guid,
  IN UINTN                       DataLength
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
  return NULL;
}

/**
  This function builds a customized HOB tagged with a GUID for identification, 
  copies the input data to the HOB data field, and returns the start address of GUID HOB data.

  @param  Guid The GUID to tag the customized HOB.
  @param  Data The data to be copied into the data field of the GUID HOB.
  @param  DataLength The size of the data payload for the GUID HOB.

  @return The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidDataHob (
  IN CONST EFI_GUID              *Guid,
  IN VOID                        *Data,
  IN UINTN                       DataLength
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
  return NULL;
}

/**
  Builds a Firmware Volume HOB.

  @param  BaseAddress The base address of the Firmware Volume.
  @param  Length The size of the Firmware Volume in bytes.

**/
VOID
EFIAPI
BuildFvHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a Capsule Volume HOB.

  @param  BaseAddress The base address of the Capsule Volume.
  @param  Length The size of the Capsule Volume in bytes.

**/
VOID
EFIAPI
BuildCvHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the CPU.

  @param  SizeOfMemorySpace The maximum physical memory addressability of the processor.
  @param  SizeOfIoSpace The maximum physical I/O addressability of the processor.

**/
VOID
EFIAPI
BuildCpuHob (
  IN UINT8                       SizeOfMemorySpace,
  IN UINT8                       SizeOfIoSpace
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the Stack.

  @param  BaseAddress The 64 bit physical address of the Stack.
  @param  Length The length of the stack in bytes.

**/
VOID
EFIAPI
BuildStackHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the BSP store.

  @param  BaseAddress The 64 bit physical address of the BSP.
  @param  Length The length of the BSP store in bytes.
  @param  MemoryType Type of memory allocated by this HOB.

**/
VOID
EFIAPI
BuildBspStoreHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_MEMORY_TYPE             MemoryType
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the memory allocation.

  @param  BaseAddress The 64 bit physical address of the memory.
  @param  Length The length of the memory allocation in bytes.
  @param  MemoryType Type of memory allocated by this HOB.

**/
VOID
EFIAPI
BuildMemoryAllocationHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_MEMORY_TYPE             MemoryType
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}
