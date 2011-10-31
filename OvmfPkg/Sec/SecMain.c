/** @file
  Main SEC phase code.  Transitions to PEI.

  Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/PeimEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/IoLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/ExtractGuidedSectionLib.h>

#include <Ppi/TemporaryRamSupport.h>

#define SEC_IDT_ENTRY_COUNT  34

typedef struct _SEC_IDT_TABLE {
  EFI_PEI_SERVICES          *PeiService;
  IA32_IDT_GATE_DESCRIPTOR  IdtTable[SEC_IDT_ENTRY_COUNT];
} SEC_IDT_TABLE;

VOID
EFIAPI
SecStartupPhase2 (
  IN VOID                     *Context
  );

EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );

//
//
//  
EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI mTemporaryRamSupportPpi = {
  TemporaryRamMigration
};

EFI_PEI_PPI_DESCRIPTOR mPrivateDispatchTable[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiTemporaryRamSupportPpiGuid,
    &mTemporaryRamSupportPpi
  },
};

//
// Template of an IDT entry pointing to 10:FFFFFFE4h.
//
IA32_IDT_GATE_DESCRIPTOR  mIdtEntryTemplate = {
  {                                      // Bits
    0xffe4,                              // OffsetLow
    0x10,                                // Selector
    0x0,                                 // Reserved_0
    IA32_IDT_GATE_TYPE_INTERRUPT_32,     // GateType
    0xffff                               // OffsetHigh
  }    
};

/**
  Locates the main boot firmware volume.

  @param[in,out]  BootFv  On input, the base of the BootFv
                          On output, the decompressed main firmware volume

  @retval EFI_SUCCESS    The main firmware volume was located and decompressed
  @retval EFI_NOT_FOUND  The main firmware volume was not found

**/
EFI_STATUS
FindMainFv (
  IN OUT  EFI_FIRMWARE_VOLUME_HEADER   **BootFv
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *Fv;
  UINTN                       Distance;

  ASSERT (((UINTN) *BootFv & EFI_PAGE_MASK) == 0);

  Fv = *BootFv;
  Distance = (UINTN) (*BootFv)->FvLength;
  do {
    Fv = (EFI_FIRMWARE_VOLUME_HEADER*) ((UINT8*) Fv - EFI_PAGE_SIZE);
    Distance += EFI_PAGE_SIZE;
    if (Distance > SIZE_32MB) {
      return EFI_NOT_FOUND;
    }

    if (Fv->Signature != EFI_FVH_SIGNATURE) {
      continue;
    }

    if ((UINTN) Fv->FvLength > Distance) {
      continue;
    }

    *BootFv = Fv;
    return EFI_SUCCESS;

  } while (TRUE);
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
EFI_STATUS
FindFfsSectionInSections (
  IN  VOID                             *Sections,
  IN  UINTN                            SizeOfSections,
  IN  EFI_SECTION_TYPE                 SectionType,
  OUT EFI_COMMON_SECTION_HEADER        **FoundSection
  )
{
  EFI_PHYSICAL_ADDRESS        CurrentAddress;
  UINT32                      Size;
  EFI_PHYSICAL_ADDRESS        EndOfSections;
  EFI_COMMON_SECTION_HEADER   *Section;
  EFI_PHYSICAL_ADDRESS        EndOfSection;

  //
  // Loop through the FFS file sections within the PEI Core FFS file
  //
  EndOfSection = (EFI_PHYSICAL_ADDRESS)(UINTN) Sections;
  EndOfSections = EndOfSection + SizeOfSections;
  for (;;) {
    if (EndOfSection == EndOfSections) {
      break;
    }
    CurrentAddress = (EndOfSection + 3) & ~(3ULL);
    if (CurrentAddress >= EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    Section = (EFI_COMMON_SECTION_HEADER*)(UINTN) CurrentAddress;
    DEBUG ((EFI_D_INFO, "Section->Type: 0x%x\n", Section->Type));

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
      *FoundSection = Section;
      return EFI_SUCCESS;
    }
    DEBUG ((EFI_D_INFO, "Section->Type (0x%x) != SectionType (0x%x)\n", Section->Type, SectionType));
  }

  return EFI_NOT_FOUND;
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
EFI_STATUS
EFIAPI
FindFfsFileAndSection (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *Fv,
  IN  EFI_FV_FILETYPE                  FileType,
  IN  EFI_SECTION_TYPE                 SectionType,
  OUT EFI_COMMON_SECTION_HEADER        **FoundSection
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        CurrentAddress;
  EFI_PHYSICAL_ADDRESS        EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER         *File;
  UINT32                      Size;
  EFI_PHYSICAL_ADDRESS        EndOfFile;

  if (Fv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((EFI_D_INFO, "FV at %p does not have FV header signature\n", Fv));
    return EFI_VOLUME_CORRUPTED;
  }

  CurrentAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) Fv;
  EndOfFirmwareVolume = CurrentAddress + Fv->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + Fv->HeaderLength; ; ) {

    CurrentAddress = (EndOfFile + 7) & ~(7ULL);
    if (CurrentAddress > EndOfFirmwareVolume) {
      return EFI_VOLUME_CORRUPTED;
    }

    File = (EFI_FFS_FILE_HEADER*)(UINTN) CurrentAddress;
    Size = *(UINT32*) File->Size & 0xffffff;
    if (Size < (sizeof (*File) + sizeof (EFI_COMMON_SECTION_HEADER))) {
      return EFI_VOLUME_CORRUPTED;
    }
    DEBUG ((EFI_D_INFO, "File->Type: 0x%x\n", File->Type));

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the request file type
    //
    if (File->Type != FileType) {
      DEBUG ((EFI_D_INFO, "File->Type (0x%x) != FileType (0x%x)\n", File->Type, FileType));
      continue;
    }

    Status = FindFfsSectionInSections (
               (VOID*) (File + 1),
               (UINTN) EndOfFile - (UINTN) (File + 1),
               SectionType,
               FoundSection
               );
    if (!EFI_ERROR (Status) || (Status == EFI_VOLUME_CORRUPTED)) {
      return Status;
    }
  }
}

/**
  Locates the compressed main firmware volume and decompresses it.

  @param[in,out]  Fv            On input, the firmware volume to search
                                On output, the decompressed main FV

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
EFIAPI
DecompressGuidedFv (
  IN OUT EFI_FIRMWARE_VOLUME_HEADER       **Fv
  )
{
  EFI_STATUS                        Status;
  EFI_GUID_DEFINED_SECTION          *Section;
  UINT32                            OutputBufferSize;
  UINT32                            ScratchBufferSize;
  UINT16                            SectionAttribute;
  UINT32                            AuthenticationStatus;
  VOID                              *OutputBuffer;
  VOID                              *ScratchBuffer;
  EFI_FIRMWARE_VOLUME_IMAGE_SECTION *NewFvSection;
  EFI_FIRMWARE_VOLUME_HEADER        *NewFv;

  NewFvSection = (EFI_FIRMWARE_VOLUME_IMAGE_SECTION*) NULL;

  Status = FindFfsFileAndSection (
             *Fv,
             EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE,
             EFI_SECTION_GUID_DEFINED,
             (EFI_COMMON_SECTION_HEADER**) &Section
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unable to find GUID defined section\n"));
    return Status;
  }

  Status = ExtractGuidedSectionGetInfo (
             Section,
             &OutputBufferSize,
             &ScratchBufferSize,
             &SectionAttribute
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unable to GetInfo for GUIDed section\n"));
    return Status;
  }

  //PcdGet32 (PcdOvmfMemFvBase), PcdGet32 (PcdOvmfMemFvSize)
  OutputBuffer = (VOID*) ((UINT8*)(UINTN) PcdGet32 (PcdOvmfMemFvBase) + SIZE_1MB);
  ScratchBuffer = ALIGN_POINTER ((UINT8*) OutputBuffer + OutputBufferSize, SIZE_1MB);
  Status = ExtractGuidedSectionDecode (
             Section,
             &OutputBuffer,
             ScratchBuffer,
             &AuthenticationStatus
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error during GUID section decode\n"));
    return Status;
  }

  Status = FindFfsSectionInSections (
             OutputBuffer,
             OutputBufferSize,
             EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
             (EFI_COMMON_SECTION_HEADER**) &NewFvSection
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unable to find FV image in extracted data\n"));
    return Status;
  }

  NewFv = (EFI_FIRMWARE_VOLUME_HEADER*)(UINTN) PcdGet32 (PcdOvmfMemFvBase);
  CopyMem (NewFv, (VOID*) (NewFvSection + 1), PcdGet32 (PcdOvmfMemFvSize));

  if (NewFv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((EFI_D_ERROR, "Extracted FV at %p does not have FV header signature\n", NewFv));
    CpuDeadLoop ();
    return EFI_VOLUME_CORRUPTED;
  }

  *Fv = NewFv;
  return EFI_SUCCESS;
}

/**
  Locates the PEI Core entry point address

  @param[in]  Fv                 The firmware volume to search
  @param[out] PeiCoreEntryPoint  The entry point of the PEI Core image

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
EFIAPI
FindPeiCoreImageBaseInFv (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *Fv,
  OUT  EFI_PHYSICAL_ADDRESS             *PeiCoreImageBase
  )
{
  EFI_STATUS                  Status;
  EFI_COMMON_SECTION_HEADER   *Section;

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
      DEBUG ((EFI_D_ERROR, "Unable to find PEI Core image\n"));
      return Status;
    }
  }

  *PeiCoreImageBase = (EFI_PHYSICAL_ADDRESS)(UINTN)(Section + 1);
  return EFI_SUCCESS;
}

/**
  Locates the PEI Core entry point address

  @param[in,out]  Fv                 The firmware volume to search
  @param[out]     PeiCoreEntryPoint  The entry point of the PEI Core image

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
VOID
EFIAPI
FindPeiCoreImageBase (
  IN OUT  EFI_FIRMWARE_VOLUME_HEADER       **BootFv,
     OUT  EFI_PHYSICAL_ADDRESS             *PeiCoreImageBase
  )
{
  *PeiCoreImageBase = 0;

  FindMainFv (BootFv);

  DecompressGuidedFv (BootFv);

  FindPeiCoreImageBaseInFv (*BootFv, PeiCoreImageBase);
}

/**
  Find core image base.

**/
EFI_STATUS
EFIAPI
FindImageBase (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *BootFirmwareVolumePtr,
  OUT EFI_PHYSICAL_ADDRESS             *SecCoreImageBase
  )
{
  EFI_PHYSICAL_ADDRESS        CurrentAddress;
  EFI_PHYSICAL_ADDRESS        EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER         *File;
  UINT32                      Size;
  EFI_PHYSICAL_ADDRESS        EndOfFile;
  EFI_COMMON_SECTION_HEADER   *Section;
  EFI_PHYSICAL_ADDRESS        EndOfSection;

  *SecCoreImageBase = 0;

  CurrentAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) BootFirmwareVolumePtr;
  EndOfFirmwareVolume = CurrentAddress + BootFirmwareVolumePtr->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + BootFirmwareVolumePtr->HeaderLength; ; ) {

    CurrentAddress = (EndOfFile + 7) & 0xfffffffffffffff8ULL;
    if (CurrentAddress > EndOfFirmwareVolume) {
      return EFI_NOT_FOUND;
    }

    File = (EFI_FFS_FILE_HEADER*)(UINTN) CurrentAddress;
    Size = *(UINT32*) File->Size & 0xffffff;
    if (Size < sizeof (*File)) {
      return EFI_NOT_FOUND;
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      return EFI_NOT_FOUND;
    }

    //
    // Look for SEC Core
    //
    if (File->Type != EFI_FV_FILETYPE_SECURITY_CORE) {
      continue;
    }

    //
    // Loop through the FFS file sections within the FFS file
    //
    EndOfSection = (EFI_PHYSICAL_ADDRESS)(UINTN) (File + 1);
    for (;;) {
      CurrentAddress = (EndOfSection + 3) & 0xfffffffffffffffcULL;
      Section = (EFI_COMMON_SECTION_HEADER*)(UINTN) CurrentAddress;

      Size = *(UINT32*) Section->Size & 0xffffff;
      if (Size < sizeof (*Section)) {
        return EFI_NOT_FOUND;
      }

      EndOfSection = CurrentAddress + Size;
      if (EndOfSection > EndOfFile) {
        return EFI_NOT_FOUND;
      }

      //
      // Look for executable sections
      //
      if (Section->Type == EFI_SECTION_PE32 || Section->Type == EFI_SECTION_TE) {
        if (File->Type == EFI_FV_FILETYPE_SECURITY_CORE) {
          *SecCoreImageBase = (PHYSICAL_ADDRESS) (UINTN) (Section + 1);
        }
        break;
      }
    }

    //
    // SEC Core image found
    //
    if (*SecCoreImageBase != 0) {
      return EFI_SUCCESS;
    }
  }
}

/*
  Find and return Pei Core entry point.

  It also find SEC and PEI Core file debug inforamtion. It will report them if
  remote debug is enabled.

**/
VOID
EFIAPI
FindAndReportEntryPoints (
  IN  EFI_FIRMWARE_VOLUME_HEADER       **BootFirmwareVolumePtr,
  OUT EFI_PEI_CORE_ENTRY_POINT         *PeiCoreEntryPoint
  )
{
  EFI_STATUS                       Status;
  EFI_PHYSICAL_ADDRESS             SecCoreImageBase;
  EFI_PHYSICAL_ADDRESS             PeiCoreImageBase;
  PE_COFF_LOADER_IMAGE_CONTEXT     ImageContext;

  //
  // Find SEC Core and PEI Core image base
   //
  Status = FindImageBase (*BootFirmwareVolumePtr, &SecCoreImageBase);
  ASSERT_EFI_ERROR (Status);

  FindPeiCoreImageBase (BootFirmwareVolumePtr, &PeiCoreImageBase);
  
  ZeroMem ((VOID *) &ImageContext, sizeof (PE_COFF_LOADER_IMAGE_CONTEXT));
  //
  // Report SEC Core debug information when remote debug is enabled
  //
  ImageContext.ImageAddress = SecCoreImageBase;
  ImageContext.PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageContext.ImageAddress);
  PeCoffLoaderRelocateImageExtraAction (&ImageContext);

  //
  // Report PEI Core debug information when remote debug is enabled
  //
  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)PeiCoreImageBase;
  ImageContext.PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageContext.ImageAddress);
  PeCoffLoaderRelocateImageExtraAction (&ImageContext);

  //
  // Find PEI Core entry point
  //
  Status = PeCoffLoaderGetEntryPoint ((VOID *) (UINTN) PeiCoreImageBase, (VOID**) PeiCoreEntryPoint);
  if (EFI_ERROR (Status)) {
    *PeiCoreEntryPoint = 0;
  }

  return;
}

VOID
EFIAPI
SecCoreStartupWithStack (
  IN EFI_FIRMWARE_VOLUME_HEADER       *BootFv,
  IN VOID                             *TopOfCurrentStack
  )
{
  EFI_SEC_PEI_HAND_OFF        SecCoreData;
  SEC_IDT_TABLE               IdtTableInStack;
  IA32_DESCRIPTOR             IdtDescriptor;
  UINT32                      Index;

  ProcessLibraryConstructorList (NULL, NULL);

  DEBUG ((EFI_D_ERROR,
    "SecCoreStartupWithStack(0x%x, 0x%x)\n",
    (UINT32)(UINTN)BootFv,
    (UINT32)(UINTN)TopOfCurrentStack
    ));

  //
  // Initialize floating point operating environment
  // to be compliant with UEFI spec.
  //
  InitializeFloatingPointUnits ();

  //
  // Initialize IDT
  //  
  IdtTableInStack.PeiService = NULL;
  for (Index = 0; Index < SEC_IDT_ENTRY_COUNT; Index ++) {
    CopyMem (&IdtTableInStack.IdtTable[Index], &mIdtEntryTemplate, sizeof (mIdtEntryTemplate));
  }

  IdtDescriptor.Base  = (UINTN)&IdtTableInStack.IdtTable;
  IdtDescriptor.Limit = (UINT16)(sizeof (IdtTableInStack.IdtTable) - 1);

  AsmWriteIdtr (&IdtDescriptor);

  //
  // |-------------|       <-- TopOfCurrentStack
  // |   Stack     | 32k
  // |-------------|
  // |    Heap     | 32k
  // |-------------|       <-- SecCoreData.TemporaryRamBase
  //

  //
  // Initialize SEC hand-off state
  //
  SecCoreData.DataSize = sizeof(EFI_SEC_PEI_HAND_OFF);

  SecCoreData.TemporaryRamSize       = SIZE_64KB;
  SecCoreData.TemporaryRamBase       = (VOID*)((UINT8 *)TopOfCurrentStack - SecCoreData.TemporaryRamSize);

  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize >> 1;

  SecCoreData.StackBase              = (UINT8 *)SecCoreData.TemporaryRamBase + SecCoreData.PeiTemporaryRamSize;
  SecCoreData.StackSize              = SecCoreData.TemporaryRamSize >> 1;

  SecCoreData.BootFirmwareVolumeBase = BootFv;
  SecCoreData.BootFirmwareVolumeSize = (UINTN) BootFv->FvLength;

  //
  // Make sure the 8259 is masked before initializing the Debug Agent and the debug timer is enabled
  //
  IoWrite8 (0x21, 0xff);
  IoWrite8 (0xA1, 0xff);
  
  //
  // Initialize Debug Agent to support source level debug in SEC/PEI phases before memory ready.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_PREMEM_SEC, &SecCoreData, SecStartupPhase2);
}
  
/**
  Caller provided function to be invoked at the end of InitializeDebugAgent().

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param[in] Context    The first input parameter of InitializeDebugAgent().

**/
VOID
EFIAPI
SecStartupPhase2(
  IN VOID                     *Context
  )
{
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  EFI_FIRMWARE_VOLUME_HEADER  *BootFv;
  EFI_PEI_CORE_ENTRY_POINT    PeiCoreEntryPoint;
  
  SecCoreData = (EFI_SEC_PEI_HAND_OFF *) Context;
  
  //
  // Find PEI Core entry point. It will report SEC and Pei Core debug information if remote debug
  // is enabled.
  //
  BootFv = (EFI_FIRMWARE_VOLUME_HEADER *)SecCoreData->BootFirmwareVolumeBase;
  FindAndReportEntryPoints (&BootFv, &PeiCoreEntryPoint);
  SecCoreData->BootFirmwareVolumeBase = BootFv;
  SecCoreData->BootFirmwareVolumeSize = (UINTN) BootFv->FvLength;

  //
  // Transfer the control to the PEI core
  //
  (*PeiCoreEntryPoint) (SecCoreData, (EFI_PEI_PPI_DESCRIPTOR *)&mPrivateDispatchTable);
  
  //
  // If we get here then the PEI Core returned, which is not recoverable.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
}

EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  IA32_DESCRIPTOR                  IdtDescriptor;
  VOID                             *OldHeap;
  VOID                             *NewHeap;
  VOID                             *OldStack;
  VOID                             *NewStack;
  DEBUG_AGENT_CONTEXT_POSTMEM_SEC  DebugAgentContext;
  BOOLEAN                          OldStatus;
  BASE_LIBRARY_JUMP_BUFFER         JumpBuffer;
  
  DEBUG ((EFI_D_ERROR, "TemporaryRamMigration(0x%x, 0x%x, 0x%x)\n", (UINTN)TemporaryMemoryBase, (UINTN)PermanentMemoryBase, CopySize));
  
  OldHeap = (VOID*)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID*)((UINTN)PermanentMemoryBase + (CopySize >> 1));
  
  OldStack = (VOID*)((UINTN)TemporaryMemoryBase + (CopySize >> 1));
  NewStack = (VOID*)(UINTN)PermanentMemoryBase;

  DebugAgentContext.HeapMigrateOffset = (UINTN)NewHeap - (UINTN)OldHeap;
  DebugAgentContext.StackMigrateOffset = (UINTN)NewStack - (UINTN)OldStack;
  
  OldStatus = SaveAndSetDebugTimerInterrupt (FALSE);
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, (VOID *) &DebugAgentContext, NULL);

  //
  // Migrate Heap
  //
  CopyMem (NewHeap, OldHeap, CopySize >> 1);

  //
  // Migrate Stack
  //
  CopyMem (NewStack, OldStack, CopySize >> 1);
  
  //
  // Rebase IDT table in permanent memory
  //
  AsmReadIdtr (&IdtDescriptor);
  IdtDescriptor.Base = IdtDescriptor.Base - (UINTN)OldStack + (UINTN)NewStack;

  AsmWriteIdtr (&IdtDescriptor);

  //
  // Use SetJump()/LongJump() to switch to a new stack.
  // 
  if (SetJump (&JumpBuffer) == 0) {
#if defined (MDE_CPU_IA32)
    JumpBuffer.Esp = JumpBuffer.Esp + DebugAgentContext.StackMigrateOffset;
#endif    
#if defined (MDE_CPU_X64)
    JumpBuffer.Rsp = JumpBuffer.Rsp + DebugAgentContext.StackMigrateOffset;
#endif    
    LongJump (&JumpBuffer, (UINTN)-1);
  }

  SaveAndSetDebugTimerInterrupt (OldStatus);

  return EFI_SUCCESS;
}

