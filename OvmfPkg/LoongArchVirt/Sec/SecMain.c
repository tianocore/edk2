/** @file
  Main SEC phase code.  Transitions to PEI.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/PeiServicesLib.h>

#include <Ppi/TemporaryRamSupport.h>

/**
  temporary memory to permanent memory and do stack switching.

  @param[in]  PeiServices     Pointer to the PEI Services Table.
  @param[in]  TemporaryMemoryBase    Temporary Memory Base address.
  @param[in]  PermanentMemoryBase   Permanent Memory Base address.
  @param[in]  CopySize   The size of memory that needs to be migrated.

  @retval   EFI_SUCCESS  Migration successful.
**/
STATIC
EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS    PermanentMemoryBase,
  IN UINTN                   CopySize
  );

STATIC EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI  mTemporaryRamSupportPpi = {
  TemporaryRamMigration
};

STATIC EFI_PEI_PPI_DESCRIPTOR  mPrivateDispatchTable[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiTemporaryRamSupportPpiGuid,
    &mTemporaryRamSupportPpi
  },
};

/**
  Locates a section within a series of sections
  with the specified section type.

  The Instance parameter indicates which instance of the section
  type to return. (0 is first instance, 1 is second...)

  @param[in]   Sections        The sections to search
  @param[in]   SizeOfSections  Total size of all sections
  @param[in]   SectionType     The section type to locate
  @param[in]   Instance        The section instance number
  @param[out]  FoundSection    The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted
**/
STATIC
EFI_STATUS
FindFfsSectionInstance (
  IN  VOID                       *Sections,
  IN  UINTN                      SizeOfSections,
  IN  EFI_SECTION_TYPE           SectionType,
  IN  UINTN                      Instance,
  OUT EFI_COMMON_SECTION_HEADER  **FoundSection
  )
{
  EFI_PHYSICAL_ADDRESS       CurrentAddress;
  UINT32                     Size;
  EFI_PHYSICAL_ADDRESS       EndOfSections;
  EFI_COMMON_SECTION_HEADER  *Section;
  EFI_PHYSICAL_ADDRESS       EndOfSection;

  //
  // Loop through the FFS file sections within the PEI Core FFS file
  //
  EndOfSection  = (EFI_PHYSICAL_ADDRESS)(UINTN)Sections;
  EndOfSections = EndOfSection + SizeOfSections;
  for ( ; ; ) {
    if (EndOfSection == EndOfSections) {
      break;
    }

    CurrentAddress = (EndOfSection + 3) & ~(3ULL);
    if (CurrentAddress >= EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    Section = (EFI_COMMON_SECTION_HEADER *)(UINTN)CurrentAddress;

    Size = SECTION_SIZE (Section);
    if (Size < sizeof (*Section)) {
      return EFI_VOLUME_CORRUPTED;
    }

    EndOfSection = CurrentAddress + Size;
    if (EndOfSection > EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the requested section type
    //
    if (Section->Type == SectionType) {
      if (Instance == 0) {
        *FoundSection = Section;
        return EFI_SUCCESS;
      } else {
        Instance--;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Locates a section within a series of sections
  with the specified section type.

  @param[in]   Sections        The sections to search
  @param[in]   SizeOfSections  Total size of all sections
  @param[in]   SectionType     The section type to locate
  @param[out]  FoundSection    The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted
**/
STATIC
EFI_STATUS
FindFfsSectionInSections (
  IN  VOID                       *Sections,
  IN  UINTN                      SizeOfSections,
  IN  EFI_SECTION_TYPE           SectionType,
  OUT EFI_COMMON_SECTION_HEADER  **FoundSection
  )
{
  return FindFfsSectionInstance (
           Sections,
           SizeOfSections,
           SectionType,
           0,
           FoundSection
           );
}

/**
  Locates a FFS file with the specified file type and a section
  within that file with the specified section type.

  @param[in]   Fv            The firmware volume to search
  @param[in]   FileType      The file type to locate
  @param[in]   SectionType   The section type to locate
  @param[out]  FoundSection  The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted
**/
STATIC
EFI_STATUS
FindFfsFileAndSection (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *Fv,
  IN  EFI_FV_FILETYPE             FileType,
  IN  EFI_SECTION_TYPE            SectionType,
  OUT EFI_COMMON_SECTION_HEADER   **FoundSection
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  CurrentAddress;
  EFI_PHYSICAL_ADDRESS  EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER   *File;
  UINT32                Size;
  EFI_PHYSICAL_ADDRESS  EndOfFile;

  if (Fv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "FV at %p does not have FV header signature\n", Fv));
    return EFI_VOLUME_CORRUPTED;
  }

  CurrentAddress      = (EFI_PHYSICAL_ADDRESS)(UINTN)Fv;
  EndOfFirmwareVolume = CurrentAddress + Fv->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + Fv->HeaderLength; ; ) {
    CurrentAddress = (EndOfFile + 7) & ~(7ULL);
    if (CurrentAddress > EndOfFirmwareVolume) {
      return EFI_VOLUME_CORRUPTED;
    }

    File = (EFI_FFS_FILE_HEADER *)(UINTN)CurrentAddress;
    Size = *(UINT32 *)File->Size & 0xffffff;
    if (Size < (sizeof (*File) + sizeof (EFI_COMMON_SECTION_HEADER))) {
      return EFI_VOLUME_CORRUPTED;
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the request file type
    //
    if (File->Type != FileType) {
      continue;
    }

    Status = FindFfsSectionInSections (
               (VOID *)(File + 1),
               (UINTN)EndOfFile - (UINTN)(File + 1),
               SectionType,
               FoundSection
               );
    if (!EFI_ERROR (Status) ||
        (Status == EFI_VOLUME_CORRUPTED))
    {
      return Status;
    }
  }
}

/**
  Locates the PEI Core entry point address

  @param[in]  Fv                 The firmware volume to search
  @param[out] PeiCoreEntryPoint  The entry point of the PEI Core image

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted
**/
STATIC
EFI_STATUS
FindPeiCoreImageBaseInFv (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *Fv,
  OUT  EFI_PHYSICAL_ADDRESS       *PeiCoreImageBase
  )
{
  EFI_STATUS                 Status;
  EFI_COMMON_SECTION_HEADER  *Section;

  Status = FindFfsFileAndSection (
             Fv,
             EFI_FV_FILETYPE_PEI_CORE,
             EFI_SECTION_PE32,
             &Section
             );
  if (EFI_ERROR (Status)) {
    Status = FindFfsFileAndSection (
               Fv,
               EFI_FV_FILETYPE_PEI_CORE,
               EFI_SECTION_TE,
               &Section
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Unable to find PEI Core image\n"));
      return Status;
    }
  }

  *PeiCoreImageBase = (EFI_PHYSICAL_ADDRESS)(UINTN)(Section + 1);
  return EFI_SUCCESS;
}

/**
  Find and return Pei Core entry point.

  It also find SEC and PEI Core file debug information. It will report them if
  remote debug is enabled.
**/
STATIC
VOID
FindAndReportEntryPoints (
  IN  EFI_FIRMWARE_VOLUME_HEADER  **BootFirmwareVolumePtr,
  OUT EFI_PEI_CORE_ENTRY_POINT    *PeiCoreEntryPoint
  )
{
  EFI_STATUS                    Status;
  EFI_PHYSICAL_ADDRESS          PeiCoreImageBase = 0;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;

  Status = FindPeiCoreImageBaseInFv (*BootFirmwareVolumePtr, &PeiCoreImageBase);
  ASSERT (Status == EFI_SUCCESS);

  ZeroMem ((VOID *)&ImageContext, sizeof (PE_COFF_LOADER_IMAGE_CONTEXT));

  //
  // Report PEI Core debug information when remote debug is enabled
  //
  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)PeiCoreImageBase;
  ImageContext.PdbPointer   = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageContext.ImageAddress);
  PeCoffLoaderRelocateImageExtraAction (&ImageContext);

  //
  // Find PEI Core entry point
  //
  Status = PeCoffLoaderGetEntryPoint ((VOID *)(UINTN)PeiCoreImageBase, (VOID **)PeiCoreEntryPoint);
  if (EFI_ERROR (Status)) {
    *PeiCoreEntryPoint = 0;
  }

  return;
}

/**
  Find the peicore entry point and jump to the entry point to execute.

  @param[in] Context    The first input parameter of InitializeDebugAgent().
**/
STATIC
VOID
EFIAPI
SecStartupPhase2 (
  IN VOID  *Context
  )
{
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  EFI_FIRMWARE_VOLUME_HEADER  *BootFv;
  EFI_PEI_CORE_ENTRY_POINT    PeiCoreEntryPoint;

  SecCoreData = (EFI_SEC_PEI_HAND_OFF *)Context;

  //
  // Find PEI Core entry point. It will report SEC and Pei Core debug information if remote debug
  // is enabled.
  //
  BootFv = (EFI_FIRMWARE_VOLUME_HEADER *)SecCoreData->BootFirmwareVolumeBase;
  FindAndReportEntryPoints (&BootFv, &PeiCoreEntryPoint);
  SecCoreData->BootFirmwareVolumeBase = BootFv;
  SecCoreData->BootFirmwareVolumeSize = (UINTN)BootFv->FvLength;

  DEBUG ((DEBUG_INFO, "Find Pei EntryPoint=%p\n", PeiCoreEntryPoint));

  //
  // Transfer the control to the PEI core
  //
  DEBUG ((DEBUG_INFO, "SecStartupPhase2 %p\n", PeiCoreEntryPoint));

  (*PeiCoreEntryPoint)(SecCoreData, (EFI_PEI_PPI_DESCRIPTOR *)&mPrivateDispatchTable);

  //
  // If we get here then the PEI Core returned, which is not recoverable.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
}

/**
  Entry point to the C language phase of SEC. initialize some temporary memory and set up the stack,
  the control is transferred to this function.

  @param[in]  BootFv   The pointer to the PEI FV in memory.
  @param[in]  TopOfCurrentStack  Top of Current Stack.
**/
VOID
EFIAPI
SecCoreStartupWithStack (
  IN EFI_FIRMWARE_VOLUME_HEADER  *BootFv,
  IN VOID                        *TopOfCurrentStack
  )
{
  EFI_SEC_PEI_HAND_OFF        SecCoreData;
  EFI_FIRMWARE_VOLUME_HEADER  *BootPeiFv = (EFI_FIRMWARE_VOLUME_HEADER *)BootFv;

  DEBUG ((DEBUG_INFO, "Entering C environment\n"));

  ProcessLibraryConstructorList ();

  DEBUG ((
    DEBUG_INFO,
    "SecCoreStartupWithStack (0x%lx, 0x%lx)\n",
    (UINTN)BootFv,
    (UINTN)TopOfCurrentStack
    ));
  DEBUG ((
    DEBUG_INFO,
    "(0x%lx, 0x%lx)\n",
    (UINTN)(FixedPcdGet64 (PcdOvmfSecPeiTempRamBase)),
    (UINTN)(FixedPcdGet32 (PcdOvmfSecPeiTempRamSize))
    ));

  // |-------------|       <-- TopOfCurrentStack
  // |  BSP Stack  | 32k
  // |-------------|
  // |  BSP Heap   | 32k
  // |-------------|       <-- SecCoreData.TemporaryRamBase
  // |  Ap Stack   | 384k
  // |-------------|
  // |  Exception  | 64k
  // |-------------|       <-- PcdOvmfSecPeiTempRamBase

  ASSERT (
    (UINTN)(FixedPcdGet64 (PcdOvmfSecPeiTempRamBase) +
            FixedPcdGet32 (PcdOvmfSecPeiTempRamSize)) ==
    (UINTN)TopOfCurrentStack
    );

  //
  // Initialize SEC hand-off state
  //
  SecCoreData.DataSize = sizeof (EFI_SEC_PEI_HAND_OFF);

  SecCoreData.TemporaryRamSize = (UINTN)SIZE_64KB;
  SecCoreData.TemporaryRamBase = (VOID *)(FixedPcdGet64 (PcdOvmfSecPeiTempRamBase) + FixedPcdGet32 (PcdOvmfSecPeiTempRamSize) - SecCoreData.TemporaryRamSize);

  SecCoreData.PeiTemporaryRamBase = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize = SecCoreData.TemporaryRamSize >> 1;

  SecCoreData.StackBase = (UINT8 *)SecCoreData.TemporaryRamBase + SecCoreData.PeiTemporaryRamSize;
  SecCoreData.StackSize = SecCoreData.TemporaryRamSize >> 1;

  SecCoreData.BootFirmwareVolumeBase = BootPeiFv;
  SecCoreData.BootFirmwareVolumeSize = (UINTN)BootPeiFv->FvLength;

  DEBUG ((
    DEBUG_INFO,
    "&SecCoreData.BootFirmwareVolumeBase=%lx SecCoreData.BootFirmwareVolumeBase=%lx\n",
    (UINT64)&(SecCoreData.BootFirmwareVolumeBase),
    (UINT64)(SecCoreData.BootFirmwareVolumeBase)
    ));
  DEBUG ((
    DEBUG_INFO,
    "&SecCoreData.BootFirmwareVolumeSize=%lx SecCoreData.BootFirmwareVolumeSize=%lx\n",
    (UINT64)&(SecCoreData.BootFirmwareVolumeSize),
    (UINT64)(SecCoreData.BootFirmwareVolumeSize)
    ));

  //
  // Initialize Debug Agent to support source level debug in SEC/PEI phases before memory ready.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_PREMEM_SEC, NULL, NULL);
  SecStartupPhase2 (&SecCoreData);
}

/**
  temporary memory to permanent memory and do stack switching.

  @param[in]  PeiServices     Pointer to the PEI Services Table.
  @param[in]  TemporaryMemoryBase    Temporary Memory Base address.
  @param[in]  PermanentMemoryBase   Permanent Memory Base address.
  @param[in]  CopySize   The size of memory that needs to be migrated.

  @retval   EFI_SUCCESS  Migration successful.
**/
STATIC
EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS    PermanentMemoryBase,
  IN UINTN                   CopySize
  )
{
  VOID                      *OldHeap;
  VOID                      *NewHeap;
  VOID                      *OldStack;
  VOID                      *NewStack;
  BASE_LIBRARY_JUMP_BUFFER  JumpBuffer;

  DEBUG ((
    DEBUG_INFO,
    "TemporaryRamMigration (0x%Lx, 0x%Lx, 0x%Lx)\n",
    TemporaryMemoryBase,
    PermanentMemoryBase,
    (UINT64)CopySize
    ));

  OldHeap = (VOID *)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID *)((UINTN)PermanentMemoryBase + (CopySize >> 1));

  OldStack = (VOID *)((UINTN)TemporaryMemoryBase + (CopySize >> 1));
  NewStack = (VOID *)(UINTN)PermanentMemoryBase;

  //
  // Migrate Heap
  //
  CopyMem (NewHeap, OldHeap, CopySize >> 1);

  //
  // Migrate Stack
  //
  CopyMem (NewStack, OldStack, CopySize >> 1);

  // Use SetJump ()/LongJump () to switch to a new stack.
  //
  if (SetJump (&JumpBuffer) == 0) {
    JumpBuffer.SP = JumpBuffer.SP - (UINTN)OldStack + (UINTN)NewStack;
    LongJump (&JumpBuffer, (UINTN)-1);
  }

  return EFI_SUCCESS;
}
