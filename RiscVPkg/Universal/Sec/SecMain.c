/** @file
  RISC-V SEC phase module.

  Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "SecMain.h"

EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI mTemporaryRamSupportPpi = {
  TemporaryRamMigration
};

EFI_PEI_TEMPORARY_RAM_DONE_PPI mTemporaryRamDonePpi = {
  TemporaryRamDone
};

EFI_PEI_PPI_DESCRIPTOR mPrivateDispatchTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiTemporaryRamSupportPpiGuid,
    &mTemporaryRamSupportPpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiTemporaryRamDonePpiGuid,
    &mTemporaryRamDonePpi
  },
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
EFI_STATUS
FindFfsSectionInstance (
  IN  VOID                             *Sections,
  IN  UINTN                            SizeOfSections,
  IN  EFI_SECTION_TYPE                 SectionType,
  IN  UINTN                            Instance,
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
EFI_STATUS
FindFfsSectionInSections (
  IN  VOID                             *Sections,
  IN  UINTN                            SizeOfSections,
  IN  EFI_SECTION_TYPE                 SectionType,
  OUT EFI_COMMON_SECTION_HEADER        **FoundSection
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
EFI_STATUS
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
    DEBUG ((EFI_D_ERROR, "FV at %p does not have FV header signature\n", Fv));
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
                                On output, the decompressed BOOT/PEI FV

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
DecompressMemFvs (
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
  EFI_FIRMWARE_VOLUME_IMAGE_SECTION *FvSection;
  EFI_FIRMWARE_VOLUME_HEADER        *PeiMemFv;
  EFI_FIRMWARE_VOLUME_HEADER        *DxeMemFv;

  FvSection = (EFI_FIRMWARE_VOLUME_IMAGE_SECTION*) NULL;

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

  OutputBuffer = (VOID*) ((UINT8*)(UINTN) PcdGet32 (PcdOvmfDxeMemFvBase) + SIZE_1MB);
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

  Status = FindFfsSectionInstance (
             OutputBuffer,
             OutputBufferSize,
             EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
             0,
             (EFI_COMMON_SECTION_HEADER**) &FvSection
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unable to find PEI FV section\n"));
    return Status;
  }

  ASSERT (SECTION_SIZE (FvSection) ==
          (PcdGet32 (PcdOvmfPeiMemFvSize) + sizeof (*FvSection)));
  ASSERT (FvSection->Type == EFI_SECTION_FIRMWARE_VOLUME_IMAGE);

  PeiMemFv = (EFI_FIRMWARE_VOLUME_HEADER*)(UINTN) PcdGet32 (PcdOvmfPeiMemFvBase);
  CopyMem (PeiMemFv, (VOID*) (FvSection + 1), PcdGet32 (PcdOvmfPeiMemFvSize));

  if (PeiMemFv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((EFI_D_ERROR, "Extracted FV at %p does not have FV header signature\n", PeiMemFv));
    CpuDeadLoop ();
    return EFI_VOLUME_CORRUPTED;
  }

  Status = FindFfsSectionInstance (
             OutputBuffer,
             OutputBufferSize,
             EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
             1,
             (EFI_COMMON_SECTION_HEADER**) &FvSection
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unable to find DXE FV section\n"));
    return Status;
  }

  ASSERT (FvSection->Type == EFI_SECTION_FIRMWARE_VOLUME_IMAGE);
  ASSERT (SECTION_SIZE (FvSection) ==
          (PcdGet32 (PcdOvmfDxeMemFvSize) + sizeof (*FvSection)));

  DxeMemFv = (EFI_FIRMWARE_VOLUME_HEADER*)(UINTN) PcdGet32 (PcdOvmfDxeMemFvBase);
  CopyMem (DxeMemFv, (VOID*) (FvSection + 1), PcdGet32 (PcdOvmfDxeMemFvSize));

  if (DxeMemFv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((EFI_D_ERROR, "Extracted FV at %p does not have FV header signature\n", DxeMemFv));
    CpuDeadLoop ();
    return EFI_VOLUME_CORRUPTED;
  }

  *Fv = PeiMemFv;
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
  DEBUG ((EFI_D_ERROR, "PeiCoreImageBase found\n"));
  *PeiCoreImageBase = (EFI_PHYSICAL_ADDRESS)(UINTN)(Section + 1);
  return EFI_SUCCESS;
}


/**
  Reads 8-bits of CMOS data.

  Reads the 8-bits of CMOS data at the location specified by Index.
  The 8-bit read value is returned.

  @param  Index  The CMOS location to read.

  @return The value read.

**/
STATIC
UINT8
CmosRead8 (
  IN      UINTN                     Index
  )
{
  IoWrite8 (0x70, (UINT8) Index);
  return IoRead8 (0x71);
}


STATIC
BOOLEAN
IsS3Resume (
  VOID
  )
{
  return (CmosRead8 (0xF) == 0xFE);
}


STATIC
EFI_STATUS
GetS3ResumePeiFv (
  IN OUT EFI_FIRMWARE_VOLUME_HEADER       **PeiFv
  )
{
  *PeiFv = (EFI_FIRMWARE_VOLUME_HEADER*)(UINTN) PcdGet32 (PcdOvmfPeiMemFvBase);
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
FindPeiCoreImageBase (
  IN OUT  EFI_FIRMWARE_VOLUME_HEADER       **BootFv,
     OUT  EFI_PHYSICAL_ADDRESS             *PeiCoreImageBase
  )
{
  *PeiCoreImageBase = 0;

  if (IsS3Resume ()) {
    DEBUG ((EFI_D_VERBOSE, "SEC: S3 resume\n"));
    GetS3ResumePeiFv (BootFv);
  } else {
    DEBUG ((EFI_D_VERBOSE, "SEC: Normal boot\n"));
    //FindMainFv (BootFv);

    //DecompressMemFvs (BootFv);
  }
  DEBUG ((EFI_D_INFO, "FindPeiCoreImageBaseInFv\n"));
  FindPeiCoreImageBaseInFv (*BootFv, PeiCoreImageBase);
}

/**
  Find core image base.

**/
EFI_STATUS
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
FindAndReportEntryPoints (
  IN  EFI_FIRMWARE_VOLUME_HEADER       **BootFirmwareVolumePtr,
  OUT EFI_PEI_CORE_ENTRY_POINT         *PeiCoreEntryPoint
  )
{
  EFI_STATUS                       Status;
  //EFI_PHYSICAL_ADDRESS             SecCoreImageBase;
  EFI_PHYSICAL_ADDRESS             PeiCoreImageBase;
  //PE_COFF_LOADER_IMAGE_CONTEXT     ImageContext;

  DEBUG ((EFI_D_INFO, "FindAndReportEntryPoints\n"));

  //
  // Find SEC Core and PEI Core image base
  //
  //Status = FindImageBase (*BootFirmwareVolumePtr, &SecCoreImageBase);
  //ASSERT_EFI_ERROR (Status);
  //DEBUG ((EFI_D_INFO, "FindImageBase success\n"));

  FindPeiCoreImageBase (BootFirmwareVolumePtr, &PeiCoreImageBase);
  
  //ZeroMem ((VOID *) &ImageContext, sizeof (PE_COFF_LOADER_IMAGE_CONTEXT));
  //
  // Report SEC Core debug information when remote debug is enabled
  //
  //ImageContext.ImageAddress = SecCoreImageBase;
  //ImageContext.PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageContext.ImageAddress);
  //PeCoffLoaderRelocateImageExtraAction (&ImageContext);
  //DEBUG ((EFI_D_INFO, "PeCoffLoaderRelocateImageExtraAction done!\n"));

  //
  // Report PEI Core debug information when remote debug is enabled
  //
  //ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)PeiCoreImageBase;
  DEBUG ((EFI_D_INFO, "PeiCoreImageBase at %x\n", PeiCoreImageBase));
  //ImageContext.PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageContext.ImageAddress);
  //DEBUG ((EFI_D_INFO, "PdbPointer at %x\n", ImageContext.PdbPointer));
  //PeCoffLoaderRelocateImageExtraAction (&ImageContext);
  //DEBUG ((EFI_D_INFO, "PeCoffLoaderGetPdbPointer done!\n"));

  //
  // Find PEI Core entry point
  //
  Status = PeCoffLoaderGetEntryPoint ((VOID *) (UINTN) PeiCoreImageBase, (VOID**) PeiCoreEntryPoint);
  if (EFI_ERROR(Status)) {
    *PeiCoreEntryPoint = 0;
  }
  DEBUG ((EFI_D_INFO, "PeCoffLoaderGetEntryPoint success: %x\n", *PeiCoreEntryPoint));

  return;
}

VOID
EFIAPI
SecCoreStartupWithStack (
  IN EFI_FIRMWARE_VOLUME_HEADER       *BootFv,
  IN VOID                             *TopOfCurrentStack
  )
{
  EFI_SEC_PEI_HAND_OFF SecCoreData;
  RISCV_MACHINE_MODE_CONTEXT *RiscvContext;

  ProcessLibraryConstructorList (NULL, NULL);
  DEBUG ((EFI_D_INFO,
    "SecCoreStartupWithStack(0x%x, 0x%x)\n",
    (UINT32)(UINTN)BootFv,
    (UINT32)(UINTN)TopOfCurrentStack
    ));

  //
  // Initialize SEC hand-off state
  //
  SecCoreData.DataSize = sizeof(EFI_SEC_PEI_HAND_OFF);

  //
  // Temporary memory usage
  //
  //   (TemporaryRamBase + TemporaryRamSize) = --> |==============================
  //                                               |                            ^
  //                                               |                            |
  //                                               |  1/2 * (TemporaryRamSize - RISCV_MACHINE_MODE_CONTEXT)
  //                                               |                            |
  //                                               |                            v
  //                               StackBase = --> |==============================
  //                                               |                            ^
  //                                               |                            |
  //                                               |  1/2 * (TemporaryRamSize - RISCV_MACHINE_MODE_CONTEXT)
  //                                               |                            |
  //                                               |                            v
  //                     PeiTemporaryRamBase ----> |==============================
  //                                               |             
  //TemporaryRamBase = RISCV_MACHINE_MODE_CONTEXT-> ==============================
 
  SecCoreData.TemporaryRamSize       = (UINTN) PcdGet32 (PcdRiscVSecPeiTempRamSize) - RISCV_MACHINE_CONTEXT_SIZE;
  SecCoreData.TemporaryRamBase       = (VOID*)((UINT8 *)TopOfCurrentStack - SecCoreData.TemporaryRamSize);

  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize >> 1;

  SecCoreData.StackBase              = (UINT8 *)SecCoreData.TemporaryRamBase +
                                       (SecCoreData.TemporaryRamSize >> 1);
  SecCoreData.StackSize              = (SecCoreData.TemporaryRamSize >> 1);

  SecCoreData.BootFirmwareVolumeBase = BootFv;
  SecCoreData.BootFirmwareVolumeSize = (UINTN) BootFv->FvLength;

  //
  // Setup RISCV_MACHINE_MODE_CONTEXT.
  //
  RiscvContext = (RISCV_MACHINE_MODE_CONTEXT *)(SecCoreData.TemporaryRamBase - RISCV_MACHINE_CONTEXT_SIZE);
  RiscvContext->MachineModeTrapHandler = (EFI_PHYSICAL_ADDRESS)(UINTN)SecMachineModeTrapHandler;
  RiscVSetScratch (RiscvContext);
  DEBUG ((DEBUG_INFO, "RISC-V Machine mode context at %x\n", RiscVGetScratch ()));
   
  //
  // Initialize Debug Agent to support source level debug in SEC/PEI phases before memory ready.
  //
  InitializeDebugAgent(DEBUG_AGENT_INIT_PREMEM_SEC, &SecCoreData, SecStartupPhase2); 
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

  DEBUG ((EFI_D_INFO, "SecStartupPhase2\n"));

  SecCoreData = (EFI_SEC_PEI_HAND_OFF *) Context;
  
  //
  // Find PEI Core entry point. It will report SEC and Pei Core debug information if remote debug
  // is enabled.
  //
  BootFv = (EFI_FIRMWARE_VOLUME_HEADER *)SecCoreData->BootFirmwareVolumeBase;
  FindAndReportEntryPoints (&BootFv, &PeiCoreEntryPoint);
  SecCoreData->BootFirmwareVolumeBase = BootFv;
  SecCoreData->BootFirmwareVolumeSize = (UINTN) BootFv->FvLength;

  DEBUG ((EFI_D_INFO, "Transfer the control to the PEI core BootFv:%x , FvLength:%x\n", BootFv, BootFv->FvLength));
  //
  // Transfer the control to the PEI core
  //
  (*PeiCoreEntryPoint) (SecCoreData, (EFI_PEI_PPI_DESCRIPTOR *)&mPrivateDispatchTable);
  
  //
  // If we get here then the PEI Core returned, which is not recoverable.
  //
  ASSERT (FALSE);
  //CpuDeadLoop ();
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
  VOID                             *OldHeap;
  VOID                             *NewHeap;
  VOID                             *OldStack;
  VOID                             *NewStack;
  DEBUG_AGENT_CONTEXT_POSTMEM_SEC  DebugAgentContext;
  BOOLEAN                          OldStatus;
  BASE_LIBRARY_JUMP_BUFFER         JumpBuffer;

  DEBUG ((EFI_D_INFO,
    "TemporaryRamMigration(0x%Lx, 0x%Lx, 0x%Lx)\n",
    TemporaryMemoryBase,
    PermanentMemoryBase,
    (UINT64)CopySize
    ));

  OldHeap = (VOID*)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID*)((UINTN)PermanentMemoryBase + (CopySize >> 1));

  OldStack = (VOID*)((UINTN)TemporaryMemoryBase + (CopySize >> 1));
  NewStack = (VOID*)(UINTN)PermanentMemoryBase;

  DebugAgentContext.HeapMigrateOffset = (UINTN)NewHeap - (UINTN)OldHeap;
  DebugAgentContext.StackMigrateOffset = (UINTN)NewStack - (UINTN)OldStack;

  OldStatus = SaveAndSetDebugTimerInterrupt (FALSE);
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, (VOID *) &DebugAgentContext, NULL);

  CopyMem (NewHeap, OldHeap, CopySize >> 1);   // Migrate Heap
  CopyMem (NewStack, OldStack, CopySize >> 1); // Migrate Stack

  //
  // Use SetJump()/LongJump() to switch to a new stack.
  // 
  if (SetJump (&JumpBuffer) == 0) {
    DEBUG ((EFI_D_INFO,"Return from SetJump\n"));
    JumpBuffer.SP = JumpBuffer.SP + DebugAgentContext.StackMigrateOffset;
    LongJump (&JumpBuffer, (UINTN)-1);
    //
    // LongJump will return to before TemporaryRamMigration.
    //
  }

  SaveAndSetDebugTimerInterrupt (OldStatus);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
TemporaryRamDone (
  VOID
  )
{
  EFI_PEI_SERVICES   **PeiServices;
  EFI_STATUS                 Status;
  RISCV_MACHINE_MODE_CONTEXT *MachineModeContext;
  RISCV_MACHINE_MODE_CONTEXT *OldMachineModeContext;
  EFI_HOB_GUID_TYPE          *HobGuid;

  DEBUG ((EFI_D_INFO, "TemporaryRamDone\n"));

  OldMachineModeContext = (RISCV_MACHINE_MODE_CONTEXT *)(UINTN)RiscVGetScratch ();
  PeiServices = (EFI_PEI_SERVICES **)(UINTN)OldMachineModeContext->PeiService;

  //
  // Copy RISCV_MACHINE_MODE_CONTEXT to HOB.
  //
  Status = (*PeiServices)->CreateHob (
                             (const EFI_PEI_SERVICES**)PeiServices,
                             EFI_HOB_TYPE_GUID_EXTENSION,
                             sizeof (EFI_HOB_GUID_TYPE) + sizeof (RISCV_MACHINE_MODE_CONTEXT),
                             (VOID **)&HobGuid
                             );
  ASSERT (!EFI_ERROR (Status));
  HobGuid->Name = gUefiRiscVMachineContextGuid;
  OldMachineModeContext = (RISCV_MACHINE_MODE_CONTEXT *)(UINTN)RiscVGetScratch ();
  MachineModeContext = (RISCV_MACHINE_MODE_CONTEXT *)(HobGuid + 1);
  CopyMem (MachineModeContext, OldMachineModeContext, sizeof (RISCV_MACHINE_MODE_CONTEXT));
  RiscVSetScratch (MachineModeContext);
  DEBUG ((DEBUG_INFO, "New RISCV_MACHINE_MODE_CONTEXT at %x.\n", RiscVGetScratch ()));

  //
  // Platform temporary memory done.
  //
  //RiscVPlatformTemporaryRamDone ();
  return EFI_SUCCESS;
}

