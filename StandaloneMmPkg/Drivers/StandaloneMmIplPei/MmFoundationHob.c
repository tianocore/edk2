/** @file

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <StandaloneMmIplPei.h>
#include <Guid/MpInformation2.h>
#include <Guid/AcpiS3Context.h>
#include <Guid/MmAcpiS3Enable.h>
#include <Guid/MmCpuSyncConfig.h>

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

  if (*FoundationHobSize < UsedSize) {
    Status = RETURN_BUFFER_TOO_SMALL;
  } else {
    Status = RETURN_SUCCESS;
  }

  *FoundationHobSize = UsedSize;
  return Status;
}
