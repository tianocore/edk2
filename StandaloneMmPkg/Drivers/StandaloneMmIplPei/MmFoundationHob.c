/** @file

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <StandaloneMmIplPei.h>
#include <Guid/MpInformation2.h>
#include <Guid/AcpiS3Context.h>
#include <Guid/MmAcpiS3Enable.h>
#include <Guid/MmCpuSyncConfig.h>
#include <Guid/MmProfileData.h>
#include <Guid/MmUnblockRegion.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/ArchitecturalMsr.h>

typedef struct {
  EFI_PHYSICAL_ADDRESS    Base;
  UINT64                  Length;
} MM_IPL_MEMORY_REGION;

/**
  Add a new HOB to the HOB List.

  @param[in] Hob          The pointer of new HOB buffer.
  @param[in] HobType      Type of the new HOB.
  @param[in] HobLength    Length of the new HOB to allocate.

  @return    NULL if there is no space to create a hob.
  @return    The address point to the new created hob.

**/
VOID *
MmIplCreateHob (
  IN VOID    *Hob,
  IN UINT16  HobType,
  IN UINT16  HobLength
  )
{
  //
  // Check Length to avoid data overflow.
  //
  ASSERT (HobLength < MAX_UINT16 - 0x7);

  HobLength = (UINT16)ALIGN_VALUE (HobLength, 8);

  ((EFI_HOB_GENERIC_HEADER *)Hob)->HobType   = HobType;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->HobLength = HobLength;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->Reserved  = 0;

  return Hob;
}

/**
  Builds a HOB that describes a chunk of system memory.

  This function builds a HOB that describes a chunk of system memory.
  If there is no additional space for HOB creation, then ASSERT().

  @param[in]  Hob                 The pointer of new HOB buffer.
  @param[in]  ResourceType        The type of resource described by this HOB.
  @param[in]  ResourceAttribute   The resource attributes of the memory described by this HOB.
  @param[in]  PhysicalStart       The 64 bit physical address of memory described by this HOB.
  @param[in]  NumberOfBytes       The length of the memory described by this HOB in bytes.
  @param[in]  Owner               The pointer of GUID.

**/
VOID
MmIplBuildMemoryResourceHob (
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *Hob,
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes,
  IN EFI_GUID                     *Owner
  )
{
  ASSERT (Hob != NULL);
  MmIplCreateHob (Hob, EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, sizeof (EFI_HOB_RESOURCE_DESCRIPTOR));

  Hob->ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
  Hob->ResourceAttribute = ResourceAttribute;
  Hob->PhysicalStart     = PhysicalStart;
  Hob->ResourceLength    = NumberOfBytes;

  if (Owner != NULL) {
    CopyGuid (&Hob->Owner, Owner);
  } else {
    ZeroMem (&Hob->Owner, sizeof (EFI_GUID));
  }
}

/**
  Builds a Firmware Volume HOB.

  This function builds a Firmware Volume HOB.
  It can only be invoked during PEI phase;
  If new HOB buffer is NULL, then ASSERT().

  @param[in]       Hob            The pointer of new HOB buffer.
  @param[in, out]  HobBufferSize  The available size of the HOB buffer when as input.
                                  The used size of when as output.
  @param[in]       BaseAddress    The base address of the Firmware Volume.
  @param[in]       Length         The size of the Firmware Volume in bytes.

**/
VOID
MmIplBuildFvHob (
  IN UINT8                 *Hob,
  IN OUT UINTN             *HobBufferSize,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  EFI_HOB_FIRMWARE_VOLUME  *FvHob;
  UINT16                   HobLength;

  ASSERT (Hob != NULL);

  HobLength = ALIGN_VALUE (sizeof (EFI_HOB_FIRMWARE_VOLUME), 8);
  if (*HobBufferSize >= HobLength) {
    MmIplCreateHob (Hob, EFI_HOB_TYPE_FV, sizeof (EFI_HOB_FIRMWARE_VOLUME));

    FvHob              = (EFI_HOB_FIRMWARE_VOLUME *)Hob;
    FvHob->BaseAddress = BaseAddress;
    FvHob->Length      = Length;
  }

  *HobBufferSize = HobLength;
}

/**
  Builds MM ACPI S3 Enable HOB.

  This function builds MM ACPI S3 Enable HOB.
  It can only be invoked during PEI phase;
  If new HOB buffer is NULL, then ASSERT().

  @param[in]       Hob            The pointer of new HOB buffer.
  @param[in, out]  HobBufferSize  The available size of the HOB buffer when as input.
                                  The used size of when as output.

**/
VOID
MmIplBuildMmAcpiS3EnableHob (
  IN UINT8      *Hob,
  IN OUT UINTN  *HobBufferSize
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  MM_ACPI_S3_ENABLE  *MmAcpiS3Enable;
  UINT16             HobLength;

  ASSERT (Hob != NULL);

  HobLength = ALIGN_VALUE (sizeof (EFI_HOB_GUID_TYPE) + sizeof (MM_ACPI_S3_ENABLE), 8);
  if (*HobBufferSize >= HobLength) {
    MmIplCreateHob (Hob, EFI_HOB_TYPE_GUID_EXTENSION, HobLength);

    GuidHob = (EFI_HOB_GUID_TYPE *)Hob;
    CopyGuid (&GuidHob->Name, &gMmAcpiS3EnableHobGuid);

    MmAcpiS3Enable               = (MM_ACPI_S3_ENABLE *)(GuidHob + 1);
    MmAcpiS3Enable->AcpiS3Enable = PcdGetBool (PcdAcpiS3Enable);
  }

  *HobBufferSize = HobLength;
}

/**
  Builds MM cpu sync configuration HOB.

  This function builds smm cpu sync configuration HOB.
  It can only be invoked during PEI phase;
  If new HOB buffer is NULL, then ASSERT().

  @param[in]       Hob            The pointer of new HOB buffer.
  @param[in, out]  HobBufferSize  The available size of the HOB buffer when as input.
                                  The used size of when as output.

**/
VOID
MmIplBuildMmCpuSyncConfigHob (
  IN UINT8      *Hob,
  IN OUT UINTN  *HobBufferSize
  )
{
  EFI_HOB_GUID_TYPE   *GuidHob;
  MM_CPU_SYNC_CONFIG  *MmSyncModeInfoHob;
  UINT16              HobLength;

  ASSERT (Hob != NULL);

  GuidHob = (EFI_HOB_GUID_TYPE *)(UINTN)Hob;

  HobLength = ALIGN_VALUE (sizeof (EFI_HOB_GUID_TYPE) + sizeof (MM_CPU_SYNC_CONFIG), 8);
  if (*HobBufferSize >= HobLength) {
    MmIplCreateHob (GuidHob, EFI_HOB_TYPE_GUID_EXTENSION, HobLength);

    CopyGuid (&GuidHob->Name, &gMmCpuSyncConfigHobGuid);

    MmSyncModeInfoHob                = (MM_CPU_SYNC_CONFIG *)(UINTN)(GuidHob + 1);
    MmSyncModeInfoHob->RelaxedApMode = (BOOLEAN)(PcdGet8 (PcdCpuSmmSyncMode) == MmCpuSyncModeRelaxedAp);
    MmSyncModeInfoHob->Timeout       = PcdGet64 (PcdCpuSmmApSyncTimeout);
    MmSyncModeInfoHob->Timeout2      = PcdGet64 (PcdCpuSmmApSyncTimeout2);
  }

  *HobBufferSize = HobLength;
}

/**
  Copies a data buffer to a newly-built HOB for GUID HOB

  This function builds a customized HOB tagged with a GUID for identification, copies the
  input data to the HOB data field and returns the start address of the GUID HOB data.
  If new HOB buffer is NULL or the GUID HOB could not found, then ASSERT().

  @param[in]       HobBuffer            The pointer of HOB buffer.
  @param[in, out]  HobBufferSize        The available size of the HOB buffer when as input.
                                        The used size of when as output.
  @param[in]       Guid                 The GUID of the GUID type HOB.
  @param[in]       MultiInstances       TRUE indicating copying multiple HOBs with the same Guid.
**/
VOID
MmIplCopyGuidHob (
  IN UINT8      *HobBuffer,
  IN OUT UINTN  *HobBufferSize,
  IN EFI_GUID   *Guid,
  IN BOOLEAN    MultiInstances
  )
{
  EFI_HOB_GENERIC_HEADER  *GuidHob;
  UINTN                   UsedSize;

  UsedSize = 0;
  GuidHob  = GetFirstGuidHob (Guid);
  ASSERT (GuidHob != NULL);

  while (GuidHob != NULL) {
    if (*HobBufferSize >= UsedSize + GuidHob->HobLength) {
      CopyMem (HobBuffer + UsedSize, GuidHob, GuidHob->HobLength);
    }

    UsedSize += GuidHob->HobLength;

    if (!MultiInstances) {
      break;
    }

    GuidHob = GetNextGuidHob (Guid, GET_NEXT_HOB (GuidHob));
  }

  *HobBufferSize = UsedSize;
}

/**
  Builds a HOB for a loaded PE32 module.

  This function builds a HOB for a loaded PE32 module.
  It can only be invoked during PEI phase;
  If physical address of the Module is not 4K aligned, then ASSERT().
  If new HOB buffer is NULL, then ASSERT().

  @param[in]       Hob            The pointer of new HOB buffer.
  @param[in, out]  HobBufferSize  The available size of the HOB buffer when as input.
                                  The used size of when as output.
  @param[in]       ModuleName     The GUID File Name of the module.
  @param[in]       Base           The 64 bit physical address of the module.
  @param[in]       Length         The length of the module in bytes.
  @param[in]       EntryPoint     The 64 bit physical address of the module entry point.

**/
VOID
MmIplBuildMmCoreModuleHob (
  IN UINT8                 *Hob,
  IN OUT UINTN             *HobBufferSize,
  IN CONST EFI_GUID        *ModuleName,
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN UINT64                Length,
  IN EFI_PHYSICAL_ADDRESS  EntryPoint
  )
{
  UINT16                            HobLength;
  EFI_HOB_MEMORY_ALLOCATION_MODULE  *MmCoreModuleHob;

  ASSERT (Hob != NULL);
  ASSERT (ADDRESS_IS_ALIGNED (Base, EFI_PAGE_SIZE));
  ASSERT (IS_ALIGNED (Length, EFI_PAGE_SIZE));
  ASSERT (EntryPoint >= Base && EntryPoint < Base + Length);

  HobLength = ALIGN_VALUE (sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE), 8);
  if (*HobBufferSize >= HobLength) {
    MmIplCreateHob (Hob, EFI_HOB_TYPE_MEMORY_ALLOCATION, sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE));

    MmCoreModuleHob = (EFI_HOB_MEMORY_ALLOCATION_MODULE *)Hob;
    CopyGuid (&MmCoreModuleHob->MemoryAllocationHeader.Name, &gEfiHobMemoryAllocModuleGuid);
    MmCoreModuleHob->MemoryAllocationHeader.MemoryBaseAddress = Base;
    MmCoreModuleHob->MemoryAllocationHeader.MemoryLength      = Length;
    MmCoreModuleHob->MemoryAllocationHeader.MemoryType        = EfiReservedMemoryType;
    ZeroMem (MmCoreModuleHob->MemoryAllocationHeader.Reserved, sizeof (MmCoreModuleHob->MemoryAllocationHeader.Reserved));

    CopyGuid (&MmCoreModuleHob->ModuleName, ModuleName);
    MmCoreModuleHob->EntryPoint = EntryPoint;
  }

  *HobBufferSize = HobLength;
}

/**
  Build memory allocation HOB in PEI HOB list for MM profile data.

  This function is to allocate memory for MM profile data.

  @return          NULL if MM profile data memory allocation HOB build fail.
  @return          Pointer of MM profile data memory allocation HOB if build successfully.

**/
EFI_HOB_MEMORY_ALLOCATION *
BuildMmProfileDataHobInPeiHobList (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 TotalSize;
  VOID                  *Alloc;

  TotalSize = PcdGet32 (PcdCpuSmmProfileSize);
  Alloc     = AllocateReservedPages (EFI_SIZE_TO_PAGES (TotalSize));
  if (Alloc == NULL) {
    return NULL;
  }

  ZeroMem (Alloc, TotalSize);

  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (Hob.Raw != NULL) {
    if (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress == (EFI_PHYSICAL_ADDRESS)(UINTN)Alloc) {
      //
      // Find the HOB just created and change the Name to gMmProfileDataHobGuid in PEI HOB list
      //
      CopyGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gMmProfileDataHobGuid);
      return Hob.MemoryAllocation;
    }

    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (Hob));
  }

  return NULL;
}

/**
  Build memory allocation and resource HOB for MM profile data

  This function builds HOBs for MM profile data, one is memory
  allocation HOB, another is resource HOB.

  @param[in]       HobBuffer      The pointer of new HOB buffer.
  @param[in, out]  HobBufferSize  The total size of the same GUID HOBs when as input.
                                  The size will be 0 for output when build HOB fail.

**/
VOID
MmIplBuildMmProfileHobs (
  IN UINT8      *HobBuffer,
  IN OUT UINTN  *HobBufferSize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 HobLength;

  Hob.MemoryAllocation = NULL;
  HobLength            = ALIGN_VALUE (sizeof (EFI_HOB_MEMORY_ALLOCATION), 8) + ALIGN_VALUE (sizeof (EFI_HOB_RESOURCE_DESCRIPTOR), 8);

  if (*HobBufferSize >= HobLength) {
    Hob.Raw = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
    while (Hob.Raw != NULL) {
      if (CompareGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gMmProfileDataHobGuid)) {
        break;
      }

      Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (Hob));
    }

    ASSERT (Hob.MemoryAllocation != NULL);

    //
    // Build memory allocation HOB
    //
    ASSERT (Hob.MemoryAllocation->Header.HobLength == ALIGN_VALUE (sizeof (EFI_HOB_MEMORY_ALLOCATION), 8));
    CopyMem (HobBuffer, Hob.Raw, Hob.MemoryAllocation->Header.HobLength);

    //
    // Build resource HOB
    //
    MmIplBuildMemoryResourceHob (
      (EFI_HOB_RESOURCE_DESCRIPTOR *)(HobBuffer + Hob.MemoryAllocation->Header.HobLength),
      EFI_RESOURCE_SYSTEM_MEMORY,
      0,
      Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
      Hob.MemoryAllocation->AllocDescriptor.MemoryLength,
      &gMmProfileDataHobGuid
      );
  }

  *HobBufferSize = HobLength;
}

/**

  Build resource Hobs from unblocked memory regions

  This function builds resource HOBs for all unblocked memory regions.

  @param[in]       HobBuffer      The pointer of new HOB buffer.
  @param[in, out]  HobBufferSize  The total size of the same GUID HOBs when as output.

**/
VOID
MmIplBuildResourceHobForUnblockedRegion (
  IN UINT8      *HobBuffer,
  IN OUT UINTN  *HobBufferSize
  )
{
  MM_UNBLOCK_REGION       *UnblockRegion;
  EFI_HOB_GENERIC_HEADER  *GuidHob;
  UINTN                   UsedSize;

  UsedSize = 0;

  GuidHob = GetFirstGuidHob (&gMmUnblockRegionHobGuid);
  while (GuidHob != NULL) {
    if (*HobBufferSize >= UsedSize + sizeof (EFI_HOB_RESOURCE_DESCRIPTOR)) {
      UnblockRegion = GET_GUID_HOB_DATA (GuidHob);
      MmIplBuildMemoryResourceHob (
        (EFI_HOB_RESOURCE_DESCRIPTOR *)(HobBuffer + UsedSize),
        EFI_RESOURCE_SYSTEM_MEMORY,
        0,
        UnblockRegion->PhysicalStart,
        EFI_PAGES_TO_SIZE (UnblockRegion->NumberOfPages),
        &UnblockRegion->IdentifierGuid
        );
    }

    UsedSize += sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
    GuidHob   = GetNextGuidHob (&gMmUnblockRegionHobGuid, GET_NEXT_HOB (GuidHob));
  }

  *HobBufferSize = UsedSize;
}

/**
  Collect unblock memory regions.

  @param[in, out]  MemoryRegion       Pointer to unblock memory regions.
  @param[in, out]  MemoryRegionCount  Count of unblock memory regions.
**/
VOID
CollectUnblockMemoryRegions (
  IN OUT MM_IPL_MEMORY_REGION  *MemoryRegion,
  IN OUT UINTN                 *MemoryRegionCount
  )
{
  UINTN                   Index;
  EFI_HOB_GENERIC_HEADER  *GuidHob;
  MM_UNBLOCK_REGION       *UnblockRegion;

  ASSERT (MemoryRegionCount != NULL);
  ASSERT (*MemoryRegionCount == 0 || MemoryRegion != NULL);

  Index = 0;
  //
  // Collect unblock memory ranges
  //
  GuidHob = GetFirstGuidHob (&gMmUnblockRegionHobGuid);
  while (GuidHob != NULL) {
    if (Index < *MemoryRegionCount) {
      UnblockRegion              = GET_GUID_HOB_DATA (GuidHob);
      MemoryRegion[Index].Base   = UnblockRegion->PhysicalStart;
      MemoryRegion[Index].Length = EFI_PAGES_TO_SIZE (UnblockRegion->NumberOfPages);
    }

    Index++;
    GuidHob = GetNextGuidHob (&gMmUnblockRegionHobGuid, GET_NEXT_HOB (GuidHob));
  }

  *MemoryRegionCount = Index;
}

/**
  Create MMIO memory map according to platform HOB.

  @param[in]       PlatformHobList    Platform HOB list.
  @param[in]       PlatformHobSize    Platform HOB size.
  @param[in, out]  MemoryRegion       Memory regions.
  @param[in, out]  MemoryRegionCount  Count of MMIO regions
**/
VOID
CollectPlatformMemoryRegions (
  IN UINT8                     *PlatformHobList,
  IN UINTN                     PlatformHobSize,
  IN OUT MM_IPL_MEMORY_REGION  *MemoryRegion,
  IN OUT UINTN                 *MemoryRegionCount
  )
{
  UINTN                 Index;
  EFI_PEI_HOB_POINTERS  Hob;

  ASSERT (MemoryRegionCount != NULL);
  ASSERT (*MemoryRegionCount == 0 || MemoryRegion != NULL);

  Index = 0;
  //
  // Get the HOB list for processing
  //
  Hob.Raw = PlatformHobList;

  //
  // Collect memory ranges
  //
  while (Hob.Raw < PlatformHobList + PlatformHobSize) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (  (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_MAPPED_IO)
         || (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY)
         || (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_FIRMWARE_DEVICE)
         || (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED))
      {
        if (Index < *MemoryRegionCount) {
          MemoryRegion[Index].Base   = Hob.ResourceDescriptor->PhysicalStart;
          MemoryRegion[Index].Length = Hob.ResourceDescriptor->ResourceLength;
        }

        Index++;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  *MemoryRegionCount = Index;
}

/**
  Function to compare 2 MM_IPL_MEMORY_REGION pointer based on Base.

  @param[in] Buffer1            pointer to MM_IPL_MEMORY_REGION pointer to compare
  @param[in] Buffer2            pointer to second MM_IPL_MEMORY_REGION pointer to compare

  @retval 0                     Buffer1 equal to Buffer2
  @retval <0                    Buffer1 is less than Buffer2
  @retval >0                    Buffer1 is greater than Buffer2
**/
INTN
EFIAPI
MemoryRegionBaseAddressCompare (
  IN CONST VOID  *Buffer1,
  IN CONST VOID  *Buffer2
  )
{
  if (((MM_IPL_MEMORY_REGION *)Buffer1)->Base > ((MM_IPL_MEMORY_REGION *)Buffer2)->Base) {
    return 1;
  } else if (((MM_IPL_MEMORY_REGION *)Buffer1)->Base < ((MM_IPL_MEMORY_REGION *)Buffer2)->Base) {
    return -1;
  }

  return 0;
}

/**
  Calculate the maximum support address.

  @return the maximum support address.
**/
UINT8
MmIplCalculateMaximumSupportAddress (
  VOID
  )
{
  UINT32  RegEax;
  UINT8   PhysicalAddressBits;
  VOID    *Hob;

  //
  // Get physical address bits supported.
  //
  Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
  if (Hob != NULL) {
    PhysicalAddressBits = ((EFI_HOB_CPU *)Hob)->SizeOfMemorySpace;
  } else {
    AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
    if (RegEax >= CPUID_VIR_PHY_ADDRESS_SIZE) {
      AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &RegEax, NULL, NULL, NULL);
      PhysicalAddressBits = (UINT8)RegEax;
    } else {
      PhysicalAddressBits = 36;
    }
  }

  return PhysicalAddressBits;
}

/**
  Build resource HOB to cover [0, PhysicalAddressBits length] by excluding
  all Mmram ranges, MM Profile data, Unblock memory ranges and MMIO ranges.

  @param[in]       HobBuffer           The pointer of new HOB buffer.
  @param[in, out]  HobBufferSize       The available size of the HOB buffer when as input.
                                       The used size of when as output.
  @param[in]       PlatformHobList     Platform HOB list.
  @param[in]       PlatformHobSize     Platform HOB size.
  @param[in]       Block               Pointer of MMRAM descriptor block.
  @param[in]       MmProfileDataHob    Pointer to MM profile data HOB.
**/
VOID
MmIplBuildResourceHobForAllSystemMemory (
  IN UINT8                           *HobBuffer,
  IN OUT UINTN                       *HobBufferSize,
  IN VOID                            *PlatformHobList,
  IN UINTN                           PlatformHobSize,
  IN EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *Block,
  IN EFI_HOB_MEMORY_ALLOCATION       *MmProfileDataHob
  )
{
  UINTN                 Index;
  UINTN                 Count;
  UINTN                 PlatformRegionCount;
  UINTN                 UsedSize;
  UINT64                PreviousAddress;
  UINT64                MaxAddress;
  MM_IPL_MEMORY_REGION  *MemoryRegions;
  MM_IPL_MEMORY_REGION  SortBuffer;
  UINTN                 UnblockRegionCount;

  MaxAddress = LShiftU64 (1, MmIplCalculateMaximumSupportAddress ());

  //
  // Get the count of platform memory regions
  //
  PlatformRegionCount = 0;
  if ((PlatformHobList != NULL) && (PlatformHobSize != 0)) {
    CollectPlatformMemoryRegions (PlatformHobList, PlatformHobSize, NULL, &PlatformRegionCount);
  }

  //
  // Get the count of platform memory regions
  //
  UnblockRegionCount = 0;
  CollectUnblockMemoryRegions (NULL, &UnblockRegionCount);

  //
  // Allocate buffer for platform memory regions, unblock memory regions,
  // MM Profile data, MMRam ranges, an extra terminator.
  //
  Count         = PlatformRegionCount + UnblockRegionCount + Block->NumberOfMmReservedRegions + ((MmProfileDataHob != NULL) ? 1 : 0) + 1;
  MemoryRegions = AllocatePages (EFI_SIZE_TO_PAGES (Count * sizeof (*MemoryRegions)));
  ASSERT (MemoryRegions != NULL);
  if (MemoryRegions == NULL) {
    DEBUG ((DEBUG_ERROR, "%a:%d - No enough memory\n", __func__, __LINE__));
    CpuDeadLoop ();
    return;
  }

  //
  // The very last region is the terminator
  //
  MemoryRegions[Count - 1].Base   = MaxAddress;
  MemoryRegions[Count - 1].Length = 0;

  //
  // Collect platform memory regions
  //
  if (PlatformRegionCount != 0) {
    CollectPlatformMemoryRegions (PlatformHobList, PlatformHobSize, MemoryRegions, &PlatformRegionCount);
  }

  //
  // Collect unblock memory regions
  //
  if (UnblockRegionCount != 0) {
    CollectUnblockMemoryRegions (&MemoryRegions[PlatformRegionCount], &UnblockRegionCount);
  }

  //
  // Collect SMRAM regions
  //
  for (Index = 0; Index < Block->NumberOfMmReservedRegions; Index++) {
    MemoryRegions[PlatformRegionCount + UnblockRegionCount + Index].Base   = Block->Descriptor[Index].CpuStart;
    MemoryRegions[PlatformRegionCount + UnblockRegionCount + Index].Length = Block->Descriptor[Index].PhysicalSize;
  }

  //
  // Collect MM profile database region
  //
  if (MmProfileDataHob != NULL) {
    MemoryRegions[PlatformRegionCount + UnblockRegionCount + Block->NumberOfMmReservedRegions].Base   = MmProfileDataHob->AllocDescriptor.MemoryBaseAddress;
    MemoryRegions[PlatformRegionCount + UnblockRegionCount + Block->NumberOfMmReservedRegions].Length = MmProfileDataHob->AllocDescriptor.MemoryLength;
  }

  //
  // Build system memory resource HOBs excluding platform memory regions, SMRAM regions, MmProfile database, Unblocked memory regions.
  //
  QuickSort (MemoryRegions, Count, sizeof (*MemoryRegions), MemoryRegionBaseAddressCompare, &SortBuffer);
  UsedSize        = 0;
  PreviousAddress = 0;
  for (Index = 0; Index < Count; Index++) {
    ASSERT (MaxAddress >= MemoryRegions[Index].Base + MemoryRegions[Index].Length);

    if (MemoryRegions[Index].Base > PreviousAddress) {
      if (*HobBufferSize >= UsedSize + sizeof (EFI_HOB_RESOURCE_DESCRIPTOR)) {
        MmIplBuildMemoryResourceHob (
          (EFI_HOB_RESOURCE_DESCRIPTOR *)(HobBuffer + UsedSize),
          EFI_RESOURCE_SYSTEM_MEMORY,
          FeaturePcdGet (PcdCpuSmmProfileEnable) ? MM_RESOURCE_ATTRIBUTE_LOGGING : 0,
          PreviousAddress,
          MemoryRegions[Index].Base - PreviousAddress,
          &gEfiCallerIdGuid
          );
      }

      UsedSize += sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
    }

    PreviousAddress = MemoryRegions[Index].Base + MemoryRegions[Index].Length;
  }

  *HobBufferSize = UsedSize;
  FreePages (MemoryRegions, EFI_SIZE_TO_PAGES (Count * sizeof (EFI_MEMORY_DESCRIPTOR)));
}

/**
  Get remaining size for building HOBs.

  @param[in] TotalHobSize    Total size of foundation HOBs.
  @param[in] UsedSize         Required HOBs' size.

  @retval    MAX remaining size for building HOBs
**/
UINTN
GetRemainingHobSize (
  IN UINTN  TotalHobSize,
  IN UINTN  UsedSize
  )
{
  if (TotalHobSize > UsedSize) {
    return TotalHobSize - UsedSize;
  } else {
    return 0;
  }
}

/**
  Create the MM foundation specific HOB list which StandaloneMm Core needed.

  This function build the MM foundation specific HOB list needed by StandaloneMm Core
  based on the PEI HOB list.

  @param[in]      FoundationHobList   The foundation HOB list to be used for HOB creation.
  @param[in, out] FoundationHobSize   The foundation HOB size.
                                      On return, the expected/used size.
  @param[in]      PlatformHobList     Platform HOB list.
  @param[in]      PlatformHobSize     Platform HOB size.
  @param[in]      MmFvBase            Base of firmare volume which included MM core dirver.
  @param[in]      MmFvSize            Size of firmare volume which included MM core dirver.
  @param[in]      MmCoreFileName      File name of MM core dirver.
  @param[in]      MmCoreImageAddress  Image address of MM core dirver.
  @param[in]      MmCoreImageSize     Image size of MM core dirver.
  @param[in]      MmCoreEntryPoint    Entry pinter of MM core dirver.
  @param[in]      MmProfileDataHob    Pointer to Mm profile data HOB.
  @param[in]      Block               Pointer of MMRAM descriptor block.

  @retval RETURN_BUFFER_TOO_SMALL     The buffer is too small for HOB creation.
                                      BufferSize is updated to indicate the expected buffer size.
                                      When the input BufferSize is bigger than the expected buffer size,
                                      the BufferSize value will be changed the used buffer size.
  @retval RETURN_SUCCESS              HOB List is created/updated successfully or the input Length is 0.

**/
RETURN_STATUS
CreateMmFoundationHobList (
  IN UINT8                           *FoundationHobList,
  IN OUT UINTN                       *FoundationHobSize,
  IN UINT8                           *PlatformHobList,
  IN UINTN                           PlatformHobSize,
  IN EFI_PHYSICAL_ADDRESS            MmFvBase,
  IN UINT64                          MmFvSize,
  IN EFI_GUID                        *MmCoreFileName,
  IN EFI_PHYSICAL_ADDRESS            MmCoreImageAddress,
  IN UINT64                          MmCoreImageSize,
  IN EFI_PHYSICAL_ADDRESS            MmCoreEntryPoint,
  IN EFI_HOB_MEMORY_ALLOCATION       *MmProfileDataHob,
  IN EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *Block
  )
{
  UINTN          UsedSize;
  RETURN_STATUS  Status;
  UINTN          HobLength;

  ASSERT (FoundationHobSize != NULL);

  ASSERT (
    ((*FoundationHobSize != 0) && (FoundationHobList != NULL)) ||
    ((*FoundationHobSize == 0) && (FoundationHobList == NULL))
    );

  if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
    //
    // When SmmProfile is enabled, all DRAM is accessible from SMM drivers' perspective.
    // However, underline Cpu SMM driver does not map the DRAM so that every access to it triggers #PF.
    // #PF handler records the access then sets up the mapping in the page table to allow the temporary access by current instruction.
    // The mapping is revoked before next instruction runs.
    //
    ASSERT (!PcdGetBool (PcdCpuSmmRestrictedMemoryAccess));
  }

  UsedSize = 0;

  //
  // Build communication buffer HOB in MM HOB list
  //
  HobLength = *FoundationHobSize;
  MmIplCopyGuidHob (FoundationHobList + UsedSize, &HobLength, &gMmCommBufferHobGuid, FALSE);
  UsedSize += HobLength;

  //
  // Build MmCore module HOB in MM HOB list
  //
  HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplBuildMmCoreModuleHob (
    FoundationHobList + UsedSize,
    &HobLength,
    MmCoreFileName,
    MmCoreImageAddress,
    MmCoreImageSize,
    MmCoreEntryPoint
    );

  UsedSize += HobLength;

  //
  // BFV address for StandaloneMm Core
  //
  HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplBuildFvHob (FoundationHobList + UsedSize, &HobLength, MmFvBase, MmFvSize);
  UsedSize += HobLength;

  //
  // Build MM ACPI S3 Enable HOB
  //
  HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplBuildMmAcpiS3EnableHob (FoundationHobList + UsedSize, &HobLength);
  UsedSize += HobLength;

  //
  // Build MM CPU sync configuration HOB
  //
  HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplBuildMmCpuSyncConfigHob (FoundationHobList + UsedSize, &HobLength);
  UsedSize += HobLength;

  //
  // Build CPU SMM base HOB in MM HOB list
  //
  HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplCopyGuidHob (FoundationHobList + UsedSize, &HobLength, &gSmmBaseHobGuid, TRUE);
  UsedSize += HobLength;

  //
  // Build SMRAM memory Hob in MM HOB list
  //
  HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplCopyGuidHob (FoundationHobList + UsedSize, &HobLength, &gEfiSmmSmramMemoryGuid, FALSE);
  UsedSize += HobLength;

  //
  // Build Mp Information2 Hob in MM HOB list
  //
  HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplCopyGuidHob (FoundationHobList + UsedSize, &HobLength, &gMpInformation2HobGuid, TRUE);
  UsedSize += HobLength;

  //
  // Build ACPI variable HOB
  //
  HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplCopyGuidHob (FoundationHobList + UsedSize, &HobLength, &gEfiAcpiVariableGuid, FALSE);
  UsedSize += HobLength;

  if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
    //
    // Build memory allocation and resource HOB for MM profile data
    //
    HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
    MmIplBuildMmProfileHobs (FoundationHobList + UsedSize, &HobLength);
    UsedSize += HobLength;
  }

  //
  // Build resource HOB for unblocked region
  //
  HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplBuildResourceHobForUnblockedRegion (FoundationHobList + UsedSize, &HobLength);
  UsedSize += HobLength;

  if (!PcdGetBool (PcdCpuSmmRestrictedMemoryAccess)) {
    //
    // All system memory (DRAM) is accessible.
    // When SMM Profile is enabled:
    //   * Access to regions included all Mmram ranges, MM Profile data, Unblock memory ranges and MMIO ranges do not require logging.
    //   * Access to other system memory requires logging.
    //
    HobLength = GetRemainingHobSize (*FoundationHobSize, UsedSize);
    MmIplBuildResourceHobForAllSystemMemory (
      FoundationHobList + UsedSize,
      &HobLength,
      PlatformHobList,
      PlatformHobSize,
      Block,
      MmProfileDataHob
      );
    UsedSize += HobLength;
  }

  if (*FoundationHobSize < UsedSize) {
    Status = RETURN_BUFFER_TOO_SMALL;
  } else {
    Status = RETURN_SUCCESS;
  }

  *FoundationHobSize = UsedSize;
  return Status;
}
