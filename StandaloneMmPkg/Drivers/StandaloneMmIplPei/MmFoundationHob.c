/** @file

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <StandaloneMmIplPei.h>
#include <Guid/MpInformation2.h>
#include <Guid/AcpiS3Context.h>
#include <Guid/UnblockRegion.h>
#include <Guid/SmmProfileData.h>
#include <Guid/AcpiS3Enable.h>
#include <Guid/SmmCpuSyncConfig.h>
#include <Library/MmPlatformHobProducerLib.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/ArchitecturalMsr.h>

//
// Configure the SMM_PROFILE DTS region size
//
#define SMM_PROFILE_DTS_SIZE      (4 * 1024 * 1024)  // 4M
#define CPUID1_EDX_BTS_AVAILABLE  0x200000

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

  @param[in]  HobPtr        The pointer of new HOB buffer.
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
  Builds an ACPI S3 Enable HOB.

  This function builds an ACPI S3 Enable HOB.
  It can only be invoked during PEI phase;
  If new HOB buffer is NULL, then ASSERT().

  @param[in]  HobPtr        The pointer of new HOB buffer.
  @param[in]  HobLength     The size of the HOB.

**/
VOID
MmIplBuildAcpiS3EnableHob (
  IN UINT8   *HobPtr,
  IN UINT16  HobLength
  )
{
  EFI_HOB_GUID_TYPE  *Hob;
  ACPI_S3_ENABLE     *AcpiS3EnableHob;

  ASSERT (HobPtr != NULL);

  Hob = (EFI_HOB_GUID_TYPE *)(UINTN)HobPtr;

  MmIplCreateHob (Hob, EFI_HOB_TYPE_GUID_EXTENSION, HobLength);

  CopyGuid (&Hob->Name, &gEdkiiAcpiS3EnableHobGuid);

  AcpiS3EnableHob               = (ACPI_S3_ENABLE *)(UINTN)(Hob + 1);
  AcpiS3EnableHob->AcpiS3Enable = PcdGetBool (PcdAcpiS3Enable);
}

/**
  Builds smm cpu sync configuration HOB.

  This function builds smm cpu sync configuration HOB.
  It can only be invoked during PEI phase;
  If new HOB buffer is NULL, then ASSERT().

  @param[in]  HobPtr        The pointer of new HOB buffer.
  @param[in]  HobLength     The size of the HOB.

**/
VOID
MmIplBuildSmmCpuSyncConfigHob (
  IN UINT8   *HobPtr,
  IN UINT16  HobLength
  )
{
  EFI_HOB_GUID_TYPE    *Hob;
  SMM_CPU_SYNC_CONFIG  *SmmSyncModeInfoHob;

  ASSERT (HobPtr != NULL);

  Hob = (EFI_HOB_GUID_TYPE *)(UINTN)HobPtr;

  MmIplCreateHob (Hob, EFI_HOB_TYPE_GUID_EXTENSION, HobLength);

  CopyGuid (&Hob->Name, &gEdkiiSmmCpuSyncConfigHobGuid);

  SmmSyncModeInfoHob               = (SMM_CPU_SYNC_CONFIG *)(UINTN)(Hob + 1);
  SmmSyncModeInfoHob->RelaxedMode  = (BOOLEAN)(PcdGet8 (PcdCpuSmmSyncMode) == SmmCpuSyncModeRelaxedAp);
  SmmSyncModeInfoHob->SyncTimeout  = PcdGet64 (PcdCpuSmmApSyncTimeout);
  SmmSyncModeInfoHob->SyncTimeout2 = PcdGet64 (PcdCpuSmmApSyncTimeout2);
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

  @param[in]  Hob                     The pointer of new HOB buffer.
  @param[in]  ModuleName              The GUID File Name of the module.
  @param[in]  Base                    The 64 bit physical address of the module.
  @param[in]  Length                  The length of the module in bytes.
  @param[in]  EntryPoint              The 64 bit physical address of the module entry point.

**/
VOID
MmIplBuildMmCoreModuleHob (
  IN EFI_HOB_MEMORY_ALLOCATION_MODULE  *Hob,
  IN CONST EFI_GUID                    *ModuleName,
  IN EFI_PHYSICAL_ADDRESS              Base,
  IN UINT64                            Length,
  IN EFI_PHYSICAL_ADDRESS              EntryPoint
  )
{
  ASSERT (Hob != NULL);
  ASSERT (ADDRESS_IS_ALIGNED (Base, EFI_PAGE_SIZE));
  ASSERT (IS_ALIGNED (Length, EFI_PAGE_SIZE));
  ASSERT (EntryPoint >= Base && EntryPoint < Base + Length);

  MmIplCreateHob (Hob, EFI_HOB_TYPE_MEMORY_ALLOCATION, sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE));

  CopyGuid (&Hob->MemoryAllocationHeader.Name, &gEfiHobMemoryAllocModuleGuid);
  Hob->MemoryAllocationHeader.MemoryBaseAddress = Base;
  Hob->MemoryAllocationHeader.MemoryLength      = Length;
  Hob->MemoryAllocationHeader.MemoryType        = EfiReservedMemoryType;
  ZeroMem (Hob->MemoryAllocationHeader.Reserved, sizeof (Hob->MemoryAllocationHeader.Reserved));

  CopyGuid (&Hob->ModuleName, ModuleName);
  Hob->EntryPoint = EntryPoint;
}

/**
  Build memory allocation and resource HOB for smm profile data

  This function builds HOBs for smm profile data, one is memory
  allocation HOB, another is resource HOB.

  @param[in]       HobBuffer      The pointer of new HOB buffer.
  @param[in, out]  HobBufferSize  The total size of the same GUID HOBs when as input.
                                  The size will be 0 for output when build HOB fail.

  @return          NULL if smm profile data memory allocation HOB not found.
  @return          Pointer of smm profile data memory allocation HOB if found.

**/
EFI_HOB_MEMORY_ALLOCATION *
MmIplBuildSmmProfileHobs (
  IN     UINT8   *HobBuffer,
  IN OUT UINT16  *HobBufferSize
  )
{
  BOOLEAN                        BtsSupported;
  UINT32                         RegEdx;
  MSR_IA32_MISC_ENABLE_REGISTER  MiscEnableMsr;
  UINTN                          TotalSize;
  VOID                           *Alloc;
  EFI_PEI_HOB_POINTERS           Hob;

  //
  // Check if processor support BTS
  //
  BtsSupported = TRUE;
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx);
  if ((RegEdx & CPUID1_EDX_BTS_AVAILABLE) != 0) {
    //
    // Per IA32 manuals:
    // When CPUID.1:EDX[21] is set, the following BTS facilities are available:
    // 1. The BTS_UNAVAILABLE flag in the IA32_MISC_ENABLE MSR indicates the
    //    availability of the BTS facilities, including the ability to set the BTS and
    //    BTINT bits in the MSR_DEBUGCTLA MSR.
    // 2. The IA32_DS_AREA MSR can be programmed to point to the DS save area.
    //
    MiscEnableMsr.Uint64 = AsmReadMsr64 (MSR_IA32_MISC_ENABLE);
    if (MiscEnableMsr.Bits.BTS == 1) {
      //
      // BTS facilities is not supported if MSR_IA32_MISC_ENABLE.BTS bit is set.
      //
      BtsSupported = FALSE;
    }
  }

  if (BtsSupported) {
    TotalSize =  PcdGet32 (PcdCpuSmmProfileSize) + SMM_PROFILE_DTS_SIZE;
  } else {
    TotalSize =  PcdGet32 (PcdCpuSmmProfileSize);
  }

  Alloc = AllocateReservedPages (EFI_SIZE_TO_PAGES (TotalSize));
  if (Alloc == NULL) {
    return NULL;
  }

  ZeroMem ((VOID *)(UINTN)Alloc, TotalSize);

  //
  // Find the HOB just created, change the name to gEdkiiSmmProfileDataGuid,
  // then build memory allocation HOB and resource HOB for smm profile data
  //
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (Hob.Raw != NULL) {
    if (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress == (UINTN)Alloc) {
      CopyGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gEdkiiSmmProfileDataGuid);

      if (*HobBufferSize >= Hob.MemoryAllocation->Header.HobLength + sizeof (EFI_HOB_RESOURCE_DESCRIPTOR)) {
        //
        // Build memory allocation HOB
        //
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
          &gEdkiiSmmProfileDataGuid
          );
      }

      ASSERT (*HobBufferSize == (Hob.MemoryAllocation->Header.HobLength + sizeof (EFI_HOB_RESOURCE_DESCRIPTOR)));
      return Hob.MemoryAllocation;
    }

    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (Hob));
  }

  FreePages (Alloc, EFI_SIZE_TO_PAGES (TotalSize));

  *HobBufferSize = 0;

  return NULL;
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
        UnblockRegion->MemoryDescriptor.PhysicalStart,
        EFI_PAGES_TO_SIZE (UnblockRegion->MemoryDescriptor.NumberOfPages),
        &UnblockRegion->IdentifierGuid
        );
    }

    UsedSize += sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
    GuidHob   = GetNextGuidHob (&gMmUnblockRegionHobGuid, GET_NEXT_HOB (GuidHob));
  }

  *HobBufferSize = UsedSize;
}

typedef struct {
  EFI_PHYSICAL_ADDRESS    Base;
  UINT64                  Length;
} MM_IPL_MEMORY_REGION;

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
  all Mmram ranges, SmmProfile data and MMIO ranges.

  @param[in]       HobBuffer      The pointer of new HOB buffer.
  @param[in, out]  HobBufferSize  The total size of the same GUID HOBs when as output.
  @param[in]      PlatformHobList    Platform HOB list.
  @param[in]      PlatformHobSize    Platform HOB size.
  @param[in]      Block              Pointer of MMRAM descriptor block.
  @param[in]      SmmProfileDataHob  Pointer to smm profile data HOB.

**/
VOID
MmIplBuildResourceHobForAllSystemMemory (
  IN UINT8                           *HobBuffer,
  IN OUT UINTN                       *HobBufferSize,
  IN VOID                            *PlatformHobList,
  IN UINTN                           PlatformHobSize,
  IN EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *Block,
  IN EFI_HOB_MEMORY_ALLOCATION       *SmmProfileDataHob
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

  MaxAddress = LShiftU64 (1, MmIplCalculateMaximumSupportAddress ());

  //
  // Get the count of platform memory regions
  //
  PlatformRegionCount = 0;
  if ((PlatformHobList != NULL) && (PlatformHobSize != 0)) {
    CollectPlatformMemoryRegions (PlatformHobList, PlatformHobSize, NULL, &PlatformRegionCount);
  }

  //
  // Allocate buffer for platform memory regions, SmmProfile data, MMRam ranges, an extra terminator.
  //
  Count         = PlatformRegionCount + Block->NumberOfMmReservedRegions + ((SmmProfileDataHob != NULL) ? 1 : 0) + 1;
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
  // Collect SMRAM regions
  //
  for (Index = 0; Index < Block->NumberOfMmReservedRegions; Index++) {
    MemoryRegions[PlatformRegionCount + Index].Base   = Block->Descriptor[Index].CpuStart;
    MemoryRegions[PlatformRegionCount + Index].Length = Block->Descriptor[Index].PhysicalSize;
  }

  //
  // Collect SMM profile database region
  //
  if (SmmProfileDataHob != NULL) {
    MemoryRegions[PlatformRegionCount + Block->NumberOfMmReservedRegions].Base   = SmmProfileDataHob->AllocDescriptor.MemoryBaseAddress;
    MemoryRegions[PlatformRegionCount + Block->NumberOfMmReservedRegions].Length = SmmProfileDataHob->AllocDescriptor.MemoryLength;
  }

  //
  // Build system memory resource HOBs excluding platform memory regions, SMRAM regions, SmmProfile database.
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
          PcdGetBool (PcdCpuSmmProfileEnable) ? EDKII_MM_RESOURCE_ATTRIBUTE_LOGGING : 0,
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
  @param[in]      MmCommBuffer        Pointer of MM communicate buffer.
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
RETURN_STATUS
CreateMmFoundationHobList (
  IN UINT8                           *FoundationHobList,
  IN OUT UINTN                       *FoundationHobSize,
  IN UINT8                           *PlatformHobList,
  IN UINTN                           PlatformHobSize,
  IN MM_COMM_BUFFER                  *MmCommBuffer,
  IN EFI_PHYSICAL_ADDRESS            MmFvBase,
  IN UINT64                          MmFvSize,
  IN EFI_GUID                        *MmCoreFileName,
  IN EFI_PHYSICAL_ADDRESS            MmCoreImageAddress,
  IN UINT64                          MmCoreImageSize,
  IN EFI_PHYSICAL_ADDRESS            MmCoreEntryPoint,
  IN EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *Block
  )
{
  UINTN                      RemainingSize;
  UINTN                      UsedSize;
  EFI_HOB_MEMORY_ALLOCATION  *SmmProfileDataHob;
  RETURN_STATUS              Status;
  UINT16                     HobLength;

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
    ASSERT (!FixedPcdGetBool (PcdCpuSmmRestrictedMemoryAccess));
  }

  RemainingSize = 0;
  UsedSize      = 0;
  //
  // Build communication buffer HOB in MM HOB list
  //
  RemainingSize = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplCopyGuidHob (FoundationHobList + UsedSize, &RemainingSize, &gEdkiiCommunicationBufferGuid, FALSE);
  UsedSize += RemainingSize;

  //
  // Build MmCore module HOB in MM HOB list
  //
  if (*FoundationHobSize >= UsedSize + sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE)) {
    MmIplBuildMmCoreModuleHob (
      (EFI_HOB_MEMORY_ALLOCATION_MODULE *)(FoundationHobList + UsedSize),
      MmCoreFileName,
      MmCoreImageAddress,
      MmCoreImageSize,
      MmCoreEntryPoint
      );
  }

  UsedSize += sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE);

  //
  // BFV address for Standalone Core
  //
  if (*FoundationHobSize >= UsedSize + sizeof (EFI_HOB_FIRMWARE_VOLUME)) {
    MmIplBuildFvHob (FoundationHobList + UsedSize, MmFvBase, MmFvSize);
  }

  UsedSize += sizeof (EFI_HOB_FIRMWARE_VOLUME);

  //
  // Build ACPI S3 Enable HOB
  //
  HobLength = (UINT16)((sizeof (EFI_HOB_GUID_TYPE) + sizeof (ACPI_S3_ENABLE) + 0x7) & (~0x7));
  if (*FoundationHobSize >= UsedSize + HobLength) {
    MmIplBuildAcpiS3EnableHob (FoundationHobList + UsedSize, HobLength);
  }

  UsedSize += HobLength;

  //
  // Build SMM CPU sync configuration HOB
  //
  HobLength = (UINT16)((sizeof (EFI_HOB_GUID_TYPE) + sizeof (SMM_CPU_SYNC_CONFIG) + 0x7) & (~0x7));
  if (*FoundationHobSize >= UsedSize + HobLength) {
    MmIplBuildSmmCpuSyncConfigHob (FoundationHobList + UsedSize, HobLength);
  }

  UsedSize += HobLength;

  //
  // Build CPU SMM base HOB in MM HOB list
  //
  RemainingSize = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplCopyGuidHob (FoundationHobList + UsedSize, &RemainingSize, &gSmmBaseHobGuid, TRUE);
  UsedSize += RemainingSize;

  //
  // Build SMRAM memory Hob in MM HOB list
  //
  RemainingSize = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplCopyGuidHob (FoundationHobList + UsedSize, &RemainingSize, &gEfiSmmSmramMemoryGuid, FALSE);
  UsedSize += RemainingSize;

  //
  // Build Mp Information2 Hob in MM HOB list
  //
  RemainingSize = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplCopyGuidHob (FoundationHobList + UsedSize, &RemainingSize, &gMpInformation2HobGuid, TRUE);
  UsedSize += RemainingSize;

  //
  // gEfiAcpiVariableGuid
  //
  RemainingSize = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  MmIplCopyGuidHob (FoundationHobList + UsedSize, &RemainingSize, &gEfiAcpiVariableGuid, FALSE);
  UsedSize += RemainingSize;

  if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
    //
    // Build memory allocation and resource HOB for smm profile data
    //
    HobLength = (UINT16)((sizeof (EFI_HOB_MEMORY_ALLOCATION) + sizeof (EFI_HOB_RESOURCE_DESCRIPTOR) + 0x7) & (~0x7));
    if (*FoundationHobSize >= UsedSize + HobLength) {
      SmmProfileDataHob = MmIplBuildSmmProfileHobs (FoundationHobList + UsedSize, &HobLength);
    }

    UsedSize += HobLength;
  }

  RemainingSize = GetRemainingHobSize (*FoundationHobSize, UsedSize);
  if (FixedPcdGetBool (PcdCpuSmmRestrictedMemoryAccess)) {
    //
    // Only unblocked memory regions are accessible
    //
    MmIplBuildResourceHobForUnblockedRegion (FoundationHobList + UsedSize, &RemainingSize);
  } else {
    //
    // All system memory (DRAM) is accessible.
    // When SMM Profile is enabled:
    //   * Access to regions reported from MmPlatformHobProducerLib do not require logging.
    //   * Access to other system memory requires logging.
    //
    MmIplBuildResourceHobForAllSystemMemory (FoundationHobList + UsedSize, &RemainingSize, PlatformHobList, PlatformHobSize, Block, SmmProfileDataHob);
  }

  UsedSize += RemainingSize;

  if (*FoundationHobSize < UsedSize) {
    Status = RETURN_BUFFER_TOO_SMALL;
  } else {
    Status = RETURN_SUCCESS;
  }

  *FoundationHobSize = UsedSize;
  return Status;
}
