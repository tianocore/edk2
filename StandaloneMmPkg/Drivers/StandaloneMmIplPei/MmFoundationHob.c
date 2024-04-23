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
#include <Guid/SmmBaseHob.h>
#include <Guid/MpInformation2.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/MmCommBuffer.h>
#include <Guid/AcpiS3Context.h>
#include <Guid/UnblockRegion.h>
#include <Guid/SmmProfileDataHob.h>
#include <Guid/MmramMemoryReserve.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmPlatformHobProducerLib.h>

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

  HobLength = (UINT16)((HobLength + 0x7) & (~0x7));
  
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

**/
VOID
MmIplBuildMemoryResourceHob (
  IN UINT8                        *Hob,
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes,
  IN EFI_GUID                     *Owner
  )
{
  ASSERT (Hob != NULL);
  MmIplCreateHob (Hob, EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, sizeof (EFI_HOB_RESOURCE_DESCRIPTOR));

  ((EFI_HOB_RESOURCE_DESCRIPTOR *)Hob)->ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
  ((EFI_HOB_RESOURCE_DESCRIPTOR *)Hob)->ResourceAttribute = ResourceAttribute;
  ((EFI_HOB_RESOURCE_DESCRIPTOR *)Hob)->PhysicalStart     = PhysicalStart;
  ((EFI_HOB_RESOURCE_DESCRIPTOR *)Hob)->ResourceLength    = NumberOfBytes;

  if (Owner != NULL) {
    CopyMem ( &((EFI_HOB_RESOURCE_DESCRIPTOR *)Hob)->Owner, Owner, sizeof (EFI_GUID));
  }

}

/**
  Builds a Firmware Volume HOB.

  This function builds a Firmware Volume HOB.
  It can only be invoked during PEI phase;
  If new HOB buffer is NULL, then ASSERT().

  @param[in]  Hob           The pointer of new HOB buffer.
  @param[in]  BaseAddress   The base address of the Firmware Volume.
  @param[in]  Length        The size of the Firmware Volume in bytes.

**/
VOID
MmIplBuildFvHob (
  IN UINT8                 *HobPtr,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  EFI_HOB_FIRMWARE_VOLUME  *Hob;

  ASSERT (HobPtr != NULL);

  Hob = (EFI_HOB_FIRMWARE_VOLUME *)(UINTN)HobPtr;
  MmIplCreateHob (HobPtr, EFI_HOB_TYPE_FV, sizeof (EFI_HOB_FIRMWARE_VOLUME));

  Hob->BaseAddress = BaseAddress;
  Hob->Length      = Length;
}

/**
  Copies a data buffer to a newly-built HOB for GUID HOB

  This function builds a customized HOB tagged with a GUID for identification, copies the 
  input data to the HOB data field and returns the start address of the GUID HOB data.
  If new HOB buffer is NULL or the GUID HOB could not found, then ASSERT().

  @param[in]       Hob             The pointer of new HOB buffer.
  @param[in, out]  HobSize         The remaining size for building HOB when as input.
                                   The total size of the same GUID HOBs when as output.
  @param[in]       Guid            The GUID of the GUID type HOB.
  @param[in]       MultiInstances  If it is TRUE, there is not only one same GUID HOB.

**/
VOID
MmIplCopyGuidHob (
  IN     UINT8     *Hob,
  IN OUT UINTN     *HobSize,
  IN     EFI_GUID  *Guid,
  IN     BOOLEAN   MultiInstances
  )
{
  EFI_HOB_GENERIC_HEADER  *GuidHob;
  UINTN                   RequiredSize;

  RequiredSize = 0;
  GuidHob      = GetFirstGuidHob (Guid);
  ASSERT (GuidHob != NULL);

  while (GuidHob != NULL) {
    if (*HobSize >= RequiredSize + GuidHob->HobLength) {
      CopyMem (Hob + RequiredSize, GuidHob, GuidHob->HobLength);
    }
    RequiredSize += GuidHob->HobLength;

    if (!MultiInstances) {
      break;
    }
    
    GuidHob = GetNextGuidHob (Guid, GET_NEXT_HOB (GuidHob));
  }
 
  *HobSize = RequiredSize;
}

/**
  Builds a HOB for a loaded PE32 module.

  This function builds a HOB for a loaded PE32 module.
  It can only be invoked during PEI phase;
  If physical address of the Module is not 4K aligned, then ASSERT().
  If new HOB buffer is NULL, then ASSERT().

  @param[in]  Hob                     The pointer of new HOB buffer.
  @param[in]  ModuleName              The GUID File Name of the module.
  @param[in]  MemoryAllocationModule  The 64 bit physical address of the module.
  @param[in]  ModuleLength            The length of the module in bytes.
  @param[in]  EntryPoint              The 64 bit physical address of the module entry point.

**/
VOID
MmIplBuildMmCoreModuleHob (
  IN UINT8                *HobPtr,
  IN CONST EFI_GUID       *ModuleName,
  IN EFI_PHYSICAL_ADDRESS  MemoryAllocationModule,
  IN UINT64                ModuleLength,
  IN EFI_PHYSICAL_ADDRESS  EntryPoint
  )
{
  EFI_HOB_MEMORY_ALLOCATION_MODULE  *Hob;

  ASSERT (HobPtr != NULL);
  ASSERT ((MemoryAllocationModule & (EFI_PAGE_SIZE - 1)) == 0);

  Hob = (EFI_HOB_MEMORY_ALLOCATION_MODULE *)(UINTN)HobPtr;

  MmIplCreateHob (HobPtr, EFI_HOB_TYPE_MEMORY_ALLOCATION, sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE));

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
  Build memory allocation and resource HOB for smm profile data

  This function builds HOBs for smm profile data, one is memory
  allocation HOB, another is resource HOB.

  @param[in]       Hob             The pointer of new HOB buffer.
  @param[in, out]  HobSize         The remaining size for building HOB when as input.
                                   The total size of the same GUID HOBs when as output.

  @return          NULL if smm profile data memory allocation HOB not found.
  @return          Pointer of smm profile data memory allocation HOB if found.

**/
EFI_HOB_MEMORY_ALLOCATION *
MmIplBuildSmmProfileHobs (
  IN     UINT8  *Hob,
  IN OUT UINTN  *HobSize
  )
{
  UINTN                      RequiredSize;
  EFI_HOB_MEMORY_ALLOCATION  *GuidHob;

  RequiredSize = 0;
  GuidHob      = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (GuidHob != NULL) {
    //
    // Find gEdkiiSmmProfileDataGuid
    //
    if (CompareGuid (&GuidHob->AllocDescriptor.Name, &gEdkiiSmmProfileDataGuid)) {
      //
      // Build memory allocation HOB
      //
      if (*HobSize >= RequiredSize + GuidHob->Header.HobLength) {
        CopyMem (Hob + RequiredSize, GuidHob, GuidHob->Header.HobLength);
      }
      RequiredSize += GuidHob->Header.HobLength;
      //
      // Build resource HOB
      //
      if (*HobSize >= RequiredSize + sizeof (EFI_HOB_RESOURCE_DESCRIPTOR)) {
        MmIplBuildMemoryResourceHob (
          Hob + RequiredSize,
          GuidHob->AllocDescriptor.MemoryType,
          0,
          GuidHob->AllocDescriptor.MemoryBaseAddress,
          GuidHob->AllocDescriptor.MemoryLength,
          NULL
          );
      }
      RequiredSize += sizeof (EFI_HOB_RESOURCE_DESCRIPTOR); 
      break;
    }

    GuidHob = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (GuidHob));
  }

  if (GuidHob != NULL) {
    *HobSize = RequiredSize;
  }

  return GuidHob;
}

/**

  Build reserouce Hobs from unblocked memory regions

  This function builds resource HOBs for all unblocked memory regions.

  @param[in]       Hob             The pointer of new HOB buffer.
  @param[in, out]  HobSize         The remaining size for building HOB when as input.
                                   The total size of the same GUID HOBs when as output.
  @param[in]       Attribute       Attribute of memory region.

**/
VOID
MmIplBuildResourceHobForUnblockedRegion (
  IN     UINT8                        *Hob,
  IN OUT UINTN                        *HobSize,
  IN     EFI_RESOURCE_ATTRIBUTE_TYPE  Attribute
  ) 
{
  VOID                    *HobData;
  EFI_HOB_GENERIC_HEADER  *GuidHob;
  UINTN                   RequiredSize;

  RequiredSize = 0;

  GuidHob = GetFirstGuidHob (&gMmUnblockRegionHobGuid);
  while (GuidHob != NULL) {
    if (*HobSize >= RequiredSize + sizeof (EFI_HOB_RESOURCE_DESCRIPTOR)) {
      HobData = GET_GUID_HOB_DATA (GuidHob);
      MmIplBuildMemoryResourceHob (
        Hob + RequiredSize,
        EFI_RESOURCE_SYSTEM_MEMORY,
        Attribute,
        ((MM_UNBLOCK_REGION *)HobData)->MemoryDescriptor.PhysicalStart,
        EFI_PAGES_TO_SIZE (((MM_UNBLOCK_REGION *)HobData)->MemoryDescriptor.NumberOfPages),
        &((MM_UNBLOCK_REGION *)HobData)->IdentifierGuid
        );

      DEBUG ((
        DEBUG_INFO,
        "BuildResourceDescriptorHob memory is for %x\n",
        ((MM_UNBLOCK_REGION *)HobData)->MemoryDescriptor.PhysicalStart
        ));
    }
    RequiredSize += sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
    GuidHob = GetNextGuidHob (&gMmUnblockRegionHobGuid, GET_NEXT_HOB (GuidHob));
  }

  *HobSize = RequiredSize;
}

/**
  Create MMIO memory map according to platform HOB.

  @param[in]       PlatformHobList    Platform HOB list.
  @param[out]      PlatformHobSize    Platform HOB size.
  @param[in, out]  MemoryMap          MMIO memory map.

  @retval          Size of MMIO resource HOB count
**/
UINTN
CreateMmioMemMap (
  IN      VOID                 *PlatformHobList,
  IN      UINTN                 PlatformHobSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap
)
{
  UINTN                 Count;
  EFI_PEI_HOB_POINTERS  Hob;

  Count = 0;
  //
  // Get the HOB list for processing
  //
  Hob.Raw = PlatformHobList;

  //
  // Collect memory ranges
  //
  while ((UINTN)Hob.Raw < ((UINTN)PlatformHobList + PlatformHobSize) ) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_MAPPED_IO) {
        if (MemoryMap != NULL) {
          MemoryMap[Count].PhysicalStart = Hob.ResourceDescriptor->PhysicalStart;
          MemoryMap[Count].NumberOfPages = EFI_SIZE_TO_PAGES (Hob.ResourceDescriptor->ResourceLength);
        }
        Count ++;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  return Count;
}

/**
  Build resource HOB to cover [0, PhysicalAddressBits length] by excluding
  all Mmram ranges, SmmProfile data and MMIO ranges.

  @param[in]      Create             If TRUE, need to create resource HOBs.
  @param[in]      Hob                The pointer of new HOB buffer.
  @param[in,out]  RequiredSize       Required size for all HOBs.
  @param[in]      Attribute          Resource HOB attribute.
  @param[in]      PlatformHobList    Platform HOB list.
  @param[in]      PlatformHobSize    Platform HOB size.
  @param[in]      Block              Pointer of MMRAM descriptor block.
  @param[in]      SmmProfileDataHob  Pointer to smm profile data HOB.

**/
VOID
MmIplBuildResourceHobForAllSystemMemory (
  IN     UINT8                           *Hob,
  IN OUT UINTN                           *HobSize,
  IN     EFI_RESOURCE_ATTRIBUTE_TYPE     *Attribute,
  IN     VOID                            *PlatformHobList,
  IN     UINTN                           PlatformHobSize,
  IN     EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *Block,
  IN     EFI_HOB_MEMORY_ALLOCATION       *SmmProfileDataHob
  )
{
  UINTN                  Index;
  UINTN                  MmRamIndex;
  UINTN                  Count;
  UINTN                  MmioCount;
  UINTN                  RequiredSize;
  UINT64                 PreviousAddress;
  UINT64                 Base;
  UINT64                 Length;
  UINT64                 MaxLength;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  EFI_MEMORY_DESCRIPTOR  SortBuffer;

  MmioCount = 0;
  if ((PlatformHobList != NULL) && (PlatformHobSize != 0)) {
    MmioCount = CreateMmioMemMap (PlatformHobList, PlatformHobSize, NULL);
  }

  //
  // Allocate memory map buffer for MMIO HOBs, SmmProfile data, MMRam ranges
  //
  Count     = MmioCount + Block->NumberOfMmReservedRegions + 1;
  MemoryMap = AllocatePages (EFI_SIZE_TO_PAGES (Count * sizeof (EFI_MEMORY_DESCRIPTOR)));
  ASSERT (MemoryMap != NULL);

  //
  // Set MMIO base and length to memory map buffer
  //
  if ((PlatformHobList != NULL) && (PlatformHobSize != 0) && (MemoryMap != NULL)) {
    MmioCount = CreateMmioMemMap (PlatformHobList, PlatformHobSize, MemoryMap);
  }

  //
  // Set Mmram base and length to memory map buffer
  //
  
  for (Index = MmioCount, MmRamIndex = 0; MmRamIndex < Block->NumberOfMmReservedRegions; Index++, MmRamIndex++) {
    MemoryMap[Index].PhysicalStart = Block->Descriptor[MmRamIndex].CpuStart;
    MemoryMap[Index].NumberOfPages = EFI_SIZE_TO_PAGES (Block->Descriptor[MmRamIndex].PhysicalSize);
  }

  //
  // Set SmmProfile data base and length to memory map buffer
  //
  if (SmmProfileDataHob != NULL) {
    MemoryMap[Index].PhysicalStart = SmmProfileDataHob->AllocDescriptor.MemoryBaseAddress;
    MemoryMap[Index].NumberOfPages = EFI_SIZE_TO_PAGES (SmmProfileDataHob->AllocDescriptor.MemoryLength);
    Index++;
  }

  //
  // Perform QuickSort for all EFI_RESOURCE_SYSTEM_MEMORY range to calculating the MMIO
  //
  QuickSort (MemoryMap, Count, sizeof (EFI_MEMORY_DESCRIPTOR), (BASE_SORT_COMPARE)MemoryDescriptorCompare, &SortBuffer);

  RequiredSize     = 0;
  PreviousAddress  = 0;
  MaxLength        = LShiftU64 (1, CalculateMaximumSupportAddress ());
  //
  // Build system memory resource HOBs except MMIO ranges, SmmProfile data and Mmram ranges
  //
  for (Index = 0; Index < Count; Index++) {
    Base   = MemoryMap[Index].PhysicalStart;
    Length = EFI_PAGES_TO_SIZE(MemoryMap[Index].NumberOfPages);
    ASSERT (MaxLength >= (Base +  Length));

    if (Base > PreviousAddress) {
      if (*HobSize >= RequiredSize + sizeof (EFI_HOB_RESOURCE_DESCRIPTOR)) {
        MmIplBuildMemoryResourceHob (
          Hob + RequiredSize,
          EFI_RESOURCE_SYSTEM_MEMORY,
          *Attribute,
          PreviousAddress,
          Base - PreviousAddress,
          NULL
        );
      }
      RequiredSize += sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
    }

    PreviousAddress = Base + Length;
  }

  //
  // Set the last remaining range
  //
  if (PreviousAddress < MaxLength) {
    if (*HobSize >= RequiredSize + sizeof (EFI_HOB_RESOURCE_DESCRIPTOR)) {
      MmIplBuildMemoryResourceHob (
        Hob + RequiredSize,
        EFI_RESOURCE_SYSTEM_MEMORY,
        *Attribute,
        PreviousAddress,
        MaxLength - PreviousAddress,
        NULL
      );
    }
    RequiredSize += sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
  }

  *HobSize = RequiredSize;
  FreePages (MemoryMap, EFI_SIZE_TO_PAGES (Count * sizeof (EFI_MEMORY_DESCRIPTOR)));
}

/**
  Get remaining size for building HOBs.

  @param[in] FoundationHobSize    Total size of foundation HOBs.
  @param[in] RequiredSize         Required HOBs' size.

  @retval    MAX remaining size for building HOBs
**/
UINTN
GetRemainingHobSize (
  IN UINTN  FoundationHobSize,
  IN UINTN  RequiredSize
)
{
  if (FoundationHobSize > RequiredSize) {
    return FoundationHobSize - RequiredSize;
  } else {
    return 0;
  }
}

/**
  Create the MM foundation specific HOB list which StandaloneMm Core needed.

  This function build the MM foundation specific HOB list needed by StandaloneMm Core
  based on the PEI HOB list.

  @param[in]      FoundationHobList  The foundation HOB list to be used for HOB creation.
  @param[in, out] FoundationHobSize  The foundation HOB size.
                                     On return, the expected/used size.
  @param[in]      PlatformHobList    Platform HOB list.
  @param[in]      PlatformHobSize    Platform HOB size.
  @param[in]      MmCommBufferData    Platform HOB list.
  @param[in]      MmFvBase            Base of firmare volume which included MM core dirver.
  @param[in]      MmFvSize            Size of firmare volume which included MM core dirver.
  @param[in]      MmCoreFileName      File name of MM core dirver.
  @param[in]      MmCoreImageAddress  Image address of MM core dirver.
  @param[in]      MmCoreImageSize     Image size of MM core dirver.
  @param[in]      MmCoreEntryPoint    Entry pinter of MM core dirver.
  @param[in]      Block               Pointer of MMRAM descriptor block.

  @retval RETURN_BUFFER_TOO_SMALL     The buffer is too small for HOB creation.
                                      BufferSize is updated to indicate the expected buffer size.
                                      When the input BufferSize is bigger than the expected buffer size,
                                      the BufferSize value will be changed the used buffer size.
  @retval RETURN_SUCCESS              HOB List is created/updated successfully or the input Length is 0.

**/
EFI_STATUS
CreateMmFoundationHobList (
  IN     VOID                            *FoundationHobList,
  IN OUT UINTN                           *FoundationHobSize,
  IN     VOID                            *PlatformHobList,
  IN     UINTN                           PlatformHobSize,
  IN     EFI_HOB_GUID_TYPE               *MmCommBuffer,
  IN     EFI_PHYSICAL_ADDRESS            MmFvBase,
  IN     UINT64                          MmFvSize,
  IN     EFI_GUID                        *MmCoreFileName,
  IN     EFI_PHYSICAL_ADDRESS            MmCoreImageAddress,
  IN     UINT64                          MmCoreImageSize,
  IN     EFI_PHYSICAL_ADDRESS            MmCoreEntryPoint,
  IN     EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *Block
  )
{
  UINT8                        *HobPtr;
  UINTN                        HobSize;
  UINTN                        RequiredSize;
  EFI_HOB_MEMORY_ALLOCATION    *SmmProfileDataHob;
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribute;

  ASSERT (FoundationHobSize != NULL);

  ASSERT (((*FoundationHobSize != 0) && (FoundationHobList != NULL)) ||
          ((*FoundationHobSize == 0) && (FoundationHobList == NULL)));

  Attribute = 0;

  if (FeaturePcdGet (PcdCpuSmmProfileEnable) == TRUE) {
    Attribute |= EDKII_MM_RESOURCE_ATTRIBUTE_LOGGING;
  }

  HobSize      = 0;
  RequiredSize = 0;
  HobPtr       = FoundationHobList;
  //
  // Build communication buffer HOB in MM HOB list
  //
  HobSize = GetRemainingHobSize (*FoundationHobSize, RequiredSize);
  MmIplCopyGuidHob (HobPtr + RequiredSize, &HobSize, &gEdkiiCommunicationBufferGuid, FALSE);
  RequiredSize += HobSize;

  //
  // Build MmCore module HOB in MM HOB list
  //
  if (*FoundationHobSize >= RequiredSize + sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE)) {
    MmIplBuildMmCoreModuleHob (HobPtr + RequiredSize, MmCoreFileName, MmCoreImageAddress, MmCoreImageSize, MmCoreEntryPoint);
  }
  RequiredSize += sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE);

  //
  // BFV address for Standalone Core
  //
  if (*FoundationHobSize >= RequiredSize + sizeof (EFI_HOB_FIRMWARE_VOLUME)) {
    MmIplBuildFvHob (HobPtr + RequiredSize, MmFvBase, MmFvSize);
  }
  RequiredSize += sizeof (EFI_HOB_FIRMWARE_VOLUME);

  //
  // Build SMM CPU feature info HOB in MM HOB list
  //
  HobSize = GetRemainingHobSize (*FoundationHobSize, RequiredSize);
  MmIplCopyGuidHob (HobPtr + RequiredSize, &HobSize, &gEdkiiSmmCpuFeatureInfoHobGuid, FALSE);
  RequiredSize += HobSize;

  //
  // Build CPU SMM base HOB in MM HOB list
  //
  HobSize = GetRemainingHobSize (*FoundationHobSize, RequiredSize);
  MmIplCopyGuidHob (HobPtr + RequiredSize, &HobSize, &gSmmBaseHobGuid, TRUE);
  RequiredSize += HobSize;

  //
  // Build SMRAM memory Hob in MM HOB list
  //
  HobSize = GetRemainingHobSize (*FoundationHobSize, RequiredSize);
  MmIplCopyGuidHob (HobPtr + RequiredSize, &HobSize, &gEfiSmmSmramMemoryGuid, FALSE);
  RequiredSize += HobSize;

  //
  // Build Mp Information2 Hob in MM HOB list
  //
  HobSize = GetRemainingHobSize (*FoundationHobSize, RequiredSize);
  MmIplCopyGuidHob (HobPtr + RequiredSize, &HobSize, &gMpInformation2HobGuid, TRUE);
  RequiredSize += HobSize;

  //
  // gEfiAcpiVariableGuid
  //
  HobSize = GetRemainingHobSize (*FoundationHobSize, RequiredSize);
  MmIplCopyGuidHob (HobPtr + RequiredSize, &HobSize, &gEfiAcpiVariableGuid, FALSE);
  RequiredSize += HobSize;

  //
  // Build memory allocation and resource HOB for smm profile data
  //
  HobSize = GetRemainingHobSize (*FoundationHobSize, RequiredSize);
  SmmProfileDataHob = MmIplBuildSmmProfileHobs (HobPtr + RequiredSize, &HobSize);
  RequiredSize += HobSize;

  if (FixedPcdGetBool (PcdCpuSmmRestrictedMemoryAccess) == TRUE) {
    //
    // Build reserouce Hobs from unblocked memory regions
    //
    HobSize = GetRemainingHobSize (*FoundationHobSize, RequiredSize);
    MmIplBuildResourceHobForUnblockedRegion (HobPtr + RequiredSize, &HobSize, Attribute);
    RequiredSize += HobSize;

  } else {
    HobSize = GetRemainingHobSize (*FoundationHobSize, RequiredSize);
    MmIplBuildResourceHobForAllSystemMemory (HobPtr + RequiredSize, &HobSize, &Attribute, PlatformHobList, PlatformHobSize, Block, SmmProfileDataHob);
    RequiredSize += HobSize;
  }

  if (*FoundationHobSize < RequiredSize) {
    *FoundationHobSize = RequiredSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  return EFI_SUCCESS;
}
