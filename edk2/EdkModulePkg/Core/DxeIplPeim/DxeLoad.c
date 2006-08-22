/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  DxeLoad.c

Abstract:

  Last PEIM.
  Responsibility of this module is to load the DXE Core from a Firmware Volume.

--*/

#include <DxeIpl.h>

BOOLEAN gInMemory = FALSE;

//
// Module Globals used in the DXE to PEI handoff
// These must be module globals, so the stack can be switched
//
static EFI_DXE_IPL_PPI mDxeIplPpi = {
  DxeLoadCore
};

static EFI_PEI_FV_FILE_LOADER_PPI mLoadFilePpi = {
  DxeIplLoadFile
};

static EFI_PEI_PPI_DESCRIPTOR     mPpiLoadFile = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiFvFileLoaderPpiGuid,
  &mLoadFilePpi
};

static EFI_PEI_PPI_DESCRIPTOR     mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiDxeIplPpiGuid,
  &mDxeIplPpi
};

static EFI_PEI_PPI_DESCRIPTOR     mPpiPeiInMemory = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiInMemoryGuid,
  NULL
};

static EFI_PEI_PPI_DESCRIPTOR     mPpiSignal = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  NULL
};

DECOMPRESS_LIBRARY  gEfiDecompress = {
  UefiDecompressGetInfo,
  UefiDecompress
};

DECOMPRESS_LIBRARY  gTianoDecompress = {
  TianoDecompressGetInfo,
  TianoDecompress
};

DECOMPRESS_LIBRARY  gCustomDecompress = {
  CustomDecompressGetInfo,
  CustomDecompress
};

STATIC
UINTN
GetOccupiedSize (
  IN UINTN   ActualSize,
  IN UINTN   Alignment
  )
{
  UINTN OccupiedSize;

  OccupiedSize = ActualSize;
  while ((OccupiedSize & (Alignment - 1)) != 0) {
    OccupiedSize++;
  }

  return OccupiedSize;
}

EFI_STATUS
EFIAPI
PeimInitializeDxeIpl (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:

  Initializes the Dxe Ipl PPI

Arguments:

  FfsHeader   - Pointer to FFS file header
  PeiServices - General purpose services available to every PEIM.

Returns:

  EFI_SUCCESS

--*/
{
  EFI_STATUS                                Status;
  EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeiEfiPeiPeCoffLoader;
  EFI_BOOT_MODE                             BootMode;

  Status = PeiServicesGetBootMode (&BootMode);

  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesLocatePpi (
             &gPeiInMemoryGuid,
             0,
             NULL,
             NULL
             );

  if (EFI_ERROR (Status) && (BootMode != BOOT_ON_S3_RESUME)) {
    //
    // The DxeIpl has not yet been shadowed
    //
    PeiEfiPeiPeCoffLoader = (EFI_PEI_PE_COFF_LOADER_PROTOCOL *)GetPeCoffLoaderProtocol ();

    //
    // Shadow DxeIpl and then re-run its entry point
    //
    Status = ShadowDxeIpl (FfsHeader, PeiEfiPeiPeCoffLoader);
    if (EFI_ERROR (Status)) {
      return Status;
    }

  } else {
    if (BootMode != BOOT_ON_S3_RESUME) {
    //
    // The DxeIpl has been shadowed
    //
    gInMemory = TRUE;

    //
    // Install LoadFile PPI
    //
    Status = PeiServicesInstallPpi (&mPpiLoadFile);

    if (EFI_ERROR (Status)) {
      return Status;
      }
    }
    //
    // Install DxeIpl PPI
    //
    PeiServicesInstallPpi (&mPpiList);

    if (EFI_ERROR (Status)) {
      return Status;
    }

  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DxeLoadCore (
  IN EFI_DXE_IPL_PPI       *This,
  IN EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PEI_HOB_POINTERS  HobList
  )
/*++

Routine Description:

  Main entry point to last PEIM

Arguments:
  This         - Entry point for DXE IPL PPI
  PeiServices  - General purpose services available to every PEIM.
  HobList      - Address to the Pei HOB list

Returns:

  EFI_SUCCESS          - DEX core was successfully loaded.
  EFI_OUT_OF_RESOURCES - There are not enough resources to load DXE core.

--*/
{
  EFI_STATUS                                Status;
  VOID                                      *TopOfStack;
  VOID                                      *BaseOfStack;
  EFI_PHYSICAL_ADDRESS                      BspStore;
  EFI_GUID                                  DxeCoreFileName;
  EFI_GUID                                  FirmwareFileName;
  VOID                                      *Pe32Data;
  EFI_PHYSICAL_ADDRESS                      DxeCoreAddress;
  UINT64                                    DxeCoreSize;
  EFI_PHYSICAL_ADDRESS                      DxeCoreEntryPoint;
  EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeiEfiPeiPeCoffLoader;
  EFI_BOOT_MODE                             BootMode;
  EFI_PEI_RECOVERY_MODULE_PPI               *PeiRecovery;
  EFI_PEI_S3_RESUME_PPI                     *S3Resume;

//  PERF_START (PeiServices, L"DxeIpl", NULL, 0);
  TopOfStack  = NULL;
  BaseOfStack = NULL;
  BspStore    = 0;
  Status      = EFI_SUCCESS;

  //
  // if in S3 Resume, restore configure
  //
  Status = PeiServicesGetBootMode (&BootMode);

  if (!EFI_ERROR (Status) && (BootMode == BOOT_ON_S3_RESUME)) {
    Status = PeiServicesLocatePpi (
               &gEfiPeiS3ResumePpiGuid,
               0,
               NULL,
               (VOID **)&S3Resume
               );

    ASSERT_EFI_ERROR (Status);

    Status = S3Resume->S3RestoreConfig (PeiServices);

    ASSERT_EFI_ERROR (Status);
  }

  Status = EFI_SUCCESS;

  //
  // Install the PEI Protocols that are shared between PEI and DXE
  //
  PeiEfiPeiPeCoffLoader = (EFI_PEI_PE_COFF_LOADER_PROTOCOL *)GetPeCoffLoaderProtocol ();
  ASSERT (PeiEfiPeiPeCoffLoader != NULL);


  //
  // Allocate 128KB for the Stack
  //
  BaseOfStack = AllocatePages (EFI_SIZE_TO_PAGES (STACK_SIZE));
  ASSERT (BaseOfStack != NULL);

  //
  // Compute the top of the stack we were allocated. Pre-allocate a UINTN
  // for safety.
  //
  TopOfStack = (VOID *)((UINTN)BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - sizeof (UINTN));

  //
  // Add architecture-specifc HOBs (including the BspStore HOB)
  //
  Status = CreateArchSpecificHobs (&BspStore);

  ASSERT_EFI_ERROR (Status);

  //
  // Add HOB for the EFI Decompress Protocol
  //
  BuildGuidDataHob (
    &gEfiDecompressProtocolGuid,
    (VOID *)&gEfiDecompress,
    sizeof (gEfiDecompress)
    );

  //
  // Add HOB for the Tiano Decompress Protocol
  //
  BuildGuidDataHob (
    &gEfiTianoDecompressProtocolGuid,
    (VOID *)&gTianoDecompress,
    sizeof (gTianoDecompress)
    );

  //
  // Add HOB for the user customized Decompress Protocol
  //
  BuildGuidDataHob (
    &gEfiCustomizedDecompressProtocolGuid,
    (VOID *)&gCustomDecompress,
    sizeof (gCustomDecompress)
    );

  //
  // Add HOB for the PE/COFF Loader Protocol
  //
  BuildGuidDataHob (
    &gEfiPeiPeCoffLoaderGuid,
    (VOID *)&PeiEfiPeiPeCoffLoader,
    sizeof (VOID *)
    );

  //
  // See if we are in crisis recovery
  //
  Status = PeiServicesGetBootMode (&BootMode);

  if (!EFI_ERROR (Status) && (BootMode == BOOT_IN_RECOVERY_MODE)) {

    Status = PeiServicesLocatePpi (
               &gEfiPeiRecoveryModulePpiGuid,
               0,
               NULL,
               (VOID **)&PeiRecovery
               );

    ASSERT_EFI_ERROR (Status);
    Status = PeiRecovery->LoadRecoveryCapsule (PeiServices, PeiRecovery);
    ASSERT_EFI_ERROR (Status);

    //
    // Now should have a HOB with the DXE core w/ the old HOB destroyed
    //
  }

  //
  // Find the EFI_FV_FILETYPE_RAW type compressed Firmware Volume file in FTW spare block
  // The file found will be processed by PeiProcessFile: It will first be decompressed to
  // a normal FV, then a corresponding FV type hob will be built which is provided for DXE
  // core to find and dispatch drivers in this FV. Because PeiProcessFile typically checks
  // for EFI_FV_FILETYPE_DXE_CORE type file, in this condition we need not check returned
  // status
  //
  Status = PeiFindFile (
            EFI_FV_FILETYPE_RAW,
            EFI_SECTION_PE32,
            &FirmwareFileName,
            &Pe32Data
            );

  //
  // Find the DXE Core in a Firmware Volume
  //
  Status = PeiFindFile (
            EFI_FV_FILETYPE_DXE_CORE,
            EFI_SECTION_PE32,
            &DxeCoreFileName,
            &Pe32Data
            );

  ASSERT_EFI_ERROR (Status);

  //
  // Load the DXE Core from a Firmware Volume
  //
  Status = PeiLoadFile (
            PeiEfiPeiPeCoffLoader,
            Pe32Data,
            &DxeCoreAddress,
            &DxeCoreSize,
            &DxeCoreEntryPoint
            );

  ASSERT_EFI_ERROR (Status);

  //
  // Transfer control to the DXE Core
  // The handoff state is simply a pointer to the HOB list
  //

  Status = PeiServicesInstallPpi (&mPpiSignal);

  ASSERT_EFI_ERROR (Status);

  //
  // Add HOB for the DXE Core
  //
  BuildModuleHob (
    &DxeCoreFileName,
    DxeCoreAddress,
    DxeCoreSize,
    DxeCoreEntryPoint
    );

  //
  // Report Status Code EFI_SW_PEI_PC_HANDOFF_TO_NEXT
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_CORE_PC_HANDOFF_TO_NEXT
    );

  DEBUG ((EFI_D_INFO, "DXE Core Entry\n"));
  SwitchIplStacks (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)DxeCoreEntryPoint,
    HobList.Raw,
    NULL,
    TopOfStack,
    (VOID *) (UINTN) BspStore
    );

  //
  // If we get here, then the DXE Core returned.  This is an error
  //
  ASSERT_EFI_ERROR (Status);

  return EFI_OUT_OF_RESOURCES;
}

EFI_STATUS
PeiFindFile (
  IN  UINT8                  Type,
  IN  UINT16                 SectionType,
  OUT EFI_GUID               *FileName,
  OUT VOID                   **Pe32Data
  )
/*++

Routine Description:

  Finds a PE/COFF of a specific Type and SectionType in the Firmware Volumes
  described in the HOB list. Able to search in a compression set in a FFS file.
  But only one level of compression is supported, that is, not able to search
  in a compression set that is within another compression set.

Arguments:

  Type        - The Type of file to retrieve

  SectionType - The type of section to retrieve from a file

  FileName    - The name of the file found in the Firmware Volume

  Pe32Data    - Pointer to the beginning of the PE/COFF file found in the Firmware Volume

Returns:

  EFI_SUCCESS   - The file was found, and the name is returned in FileName, and a pointer to
                  the PE/COFF image is returned in Pe32Data

  EFI_NOT_FOUND - The file was not found in the Firmware Volumes present in the HOB List

--*/
{
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  EFI_FFS_FILE_HEADER         *FfsFileHeader;
  VOID                        *SectionData;
  EFI_STATUS                  Status;
  EFI_PEI_HOB_POINTERS        Hob;


  FwVolHeader   = NULL;
  FfsFileHeader = NULL;
  SectionData   = NULL;

  //
  // Foreach Firmware Volume, look for a specified type
  // of file and break out when one is found
  //
  Hob.Raw = GetHobList ();
  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_FV, Hob.Raw)) != NULL) {
    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (Hob.FirmwareVolume->BaseAddress);
    Status = PeiServicesFfsFindNextFile (
               Type,
               FwVolHeader,
               &FfsFileHeader
               );
    if (!EFI_ERROR (Status)) {
      Status = PeiProcessFile (
                 SectionType,
                 &FfsFileHeader,
                 Pe32Data
                 );
      CopyMem (FileName, &FfsFileHeader->Name, sizeof (EFI_GUID));
      return Status;
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  return EFI_NOT_FOUND;
}

EFI_STATUS
PeiLoadFile (
  IN  EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeiEfiPeiPeCoffLoader,
  IN  VOID                                      *Pe32Data,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  )
/*++

Routine Description:

  Loads and relocates a PE/COFF image into memory.

Arguments:

  PeiEfiPeiPeCoffLoader - Pointer to a PE COFF loader protocol

  Pe32Data         - The base address of the PE/COFF file that is to be loaded and relocated

  ImageAddress     - The base address of the relocated PE/COFF image

  ImageSize        - The size of the relocated PE/COFF image

  EntryPoint       - The entry point of the relocated PE/COFF image

Returns:

  EFI_SUCCESS   - The file was loaded and relocated

  EFI_OUT_OF_RESOURCES - There was not enough memory to load and relocate the PE/COFF file

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;

  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle = Pe32Data;
  Status              = GetImageReadFunction (&ImageContext);

  ASSERT_EFI_ERROR (Status);

  Status = PeiEfiPeiPeCoffLoader->GetImageInfo (PeiEfiPeiPeCoffLoader, &ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Allocate Memory for the image
  //
  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) AllocatePages (EFI_SIZE_TO_PAGES ((UINT32) ImageContext.ImageSize));
  ASSERT (ImageContext.ImageAddress != 0);

  //
  // Load the image to our new buffer
  //
  Status = PeiEfiPeiPeCoffLoader->LoadImage (PeiEfiPeiPeCoffLoader, &ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Relocate the image in our new buffer
  //
  Status = PeiEfiPeiPeCoffLoader->RelocateImage (PeiEfiPeiPeCoffLoader, &ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Flush the instruction cache so the image data is written before we execute it
  //
  InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

  *ImageAddress = ImageContext.ImageAddress;
  *ImageSize    = ImageContext.ImageSize;
  *EntryPoint   = ImageContext.EntryPoint;

  return EFI_SUCCESS;
}

EFI_STATUS
ShadowDxeIpl (
  IN EFI_FFS_FILE_HEADER                       *DxeIplFileHeader,
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeiEfiPeiPeCoffLoader
  )
/*++

Routine Description:

  Shadow the DXE IPL to a different memory location. This occurs after permanent
  memory has been discovered.

Arguments:

  DxeIplFileHeader      - Pointer to the FFS file header of the DXE IPL driver

  PeiEfiPeiPeCoffLoader - Pointer to a PE COFF loader protocol

Returns:

  EFI_SUCCESS   - DXE IPL was successfully shadowed to a different memory location.

  EFI_ ERROR    - The shadow was unsuccessful.


--*/
{
  UINTN                     SectionLength;
  UINTN                     OccupiedSectionLength;
  EFI_PHYSICAL_ADDRESS      DxeIplAddress;
  UINT64                    DxeIplSize;
  EFI_PHYSICAL_ADDRESS      DxeIplEntryPoint;
  EFI_STATUS                Status;
  EFI_COMMON_SECTION_HEADER *Section;

  Section = (EFI_COMMON_SECTION_HEADER *) (DxeIplFileHeader + 1);

  while ((Section->Type != EFI_SECTION_PE32) && (Section->Type != EFI_SECTION_TE)) {
    SectionLength         = *(UINT32 *) (Section->Size) & 0x00ffffff;
    OccupiedSectionLength = GetOccupiedSize (SectionLength, 4);
    Section               = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) Section + OccupiedSectionLength);
  }
  //
  // Relocate DxeIpl into memory by using loadfile service
  //
  Status = PeiLoadFile (
            PeiEfiPeiPeCoffLoader,
            (VOID *) (Section + 1),
            &DxeIplAddress,
            &DxeIplSize,
            &DxeIplEntryPoint
            );

  if (Status == EFI_SUCCESS) {
    //
    // Install PeiInMemory to indicate the Dxeipl is shadowed
    //
    Status = PeiServicesInstallPpi (&mPpiPeiInMemory);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = ((EFI_PEIM_ENTRY_POINT) (UINTN) DxeIplEntryPoint) (DxeIplFileHeader, GetPeiServicesTablePointer());
  }

  return Status;
}

EFI_STATUS
EFIAPI
DxeIplLoadFile (
  IN EFI_PEI_FV_FILE_LOADER_PPI                 *This,
  IN  EFI_FFS_FILE_HEADER                       *FfsHeader,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  )
/*++

Routine Description:

  Given a pointer to an FFS file containing a PE32 image, get the
  information on the PE32 image, and then "load" it so that it
  can be executed.

Arguments:

  This  - pointer to our file loader protocol

  FfsHeader - pointer to the FFS file header of the FFS file that
              contains the PE32 image we want to load

  ImageAddress  - returned address where the PE32 image is loaded

  ImageSize     - returned size of the loaded PE32 image

  EntryPoint    - entry point to the loaded PE32 image

Returns:

  EFI_SUCCESS  - The FFS file was successfully loaded.

  EFI_ERROR    - Unable to load the FFS file.

--*/
{
  EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeiEfiPeiPeCoffLoader;
  EFI_STATUS                                Status;
  VOID                                      *Pe32Data;

  Pe32Data = NULL;
  PeiEfiPeiPeCoffLoader = (EFI_PEI_PE_COFF_LOADER_PROTOCOL *)GetPeCoffLoaderProtocol ();

  //
  // Preprocess the FFS file to get a pointer to the PE32 information
  // in the enclosed PE32 image.
  //
  Status = PeiProcessFile (
            EFI_SECTION_PE32,
            &FfsHeader,
            &Pe32Data
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Load the PE image from the FFS file
  //
  Status = PeiLoadFile (
            PeiEfiPeiPeCoffLoader,
            Pe32Data,
            ImageAddress,
            ImageSize,
            EntryPoint
            );

  return Status;
}

EFI_STATUS
PeiProcessFile (
  IN      UINT16                 SectionType,
  IN OUT  EFI_FFS_FILE_HEADER    **RealFfsFileHeader,
  OUT     VOID                   **Pe32Data
  )
/*++

Routine Description:

Arguments:

  SectionType       - The type of section in the FFS file to process.

  FfsFileHeader     - Pointer to the FFS file to process, looking for the
                      specified SectionType

  Pe32Data          - returned pointer to the start of the PE32 image found
                      in the FFS file.

Returns:

  EFI_SUCCESS       - found the PE32 section in the FFS file

--*/
{
  EFI_STATUS                      Status;
  VOID                            *SectionData;
  DECOMPRESS_LIBRARY              *DecompressLibrary;
  UINT8                           *DstBuffer;
  UINT8                           *ScratchBuffer;
  UINT32                          DstBufferSize;
  UINT32                          ScratchBufferSize;
  EFI_COMMON_SECTION_HEADER       *CmpSection;
  UINTN                           CmpSectionLength;
  UINTN                           OccupiedCmpSectionLength;
  VOID                            *CmpFileData;
  UINTN                           CmpFileSize;
  EFI_COMMON_SECTION_HEADER       *Section;
  UINTN                           SectionLength;
  UINTN                           OccupiedSectionLength;
  UINT64                          FileSize;
  EFI_GUID_DEFINED_SECTION        *GuidedSectionHeader;
  UINT32                          AuthenticationStatus;
  EFI_PEI_SECTION_EXTRACTION_PPI  *SectionExtract;
  UINT32                          BufferSize;
  UINT8                           *Buffer;
  EFI_PEI_SECURITY_PPI            *Security;
  BOOLEAN                         StartCrisisRecovery;
  EFI_GUID                        TempGuid;
  EFI_FIRMWARE_VOLUME_HEADER      *FvHeader;
  EFI_COMPRESSION_SECTION         *CompressionSection;
  EFI_FFS_FILE_HEADER             *FfsFileHeader;

  FfsFileHeader = *RealFfsFileHeader;

  Status = PeiServicesFfsFindSectionData (
             EFI_SECTION_COMPRESSION,
             FfsFileHeader,
             &SectionData
             );

  //
  // Upon finding a DXE Core file, see if there is first a compression section
  //
  if (!EFI_ERROR (Status)) {
    //
    // Yes, there is a compression section, so extract the contents
    // Decompress the image here
    //
    Section = (EFI_COMMON_SECTION_HEADER *) (UINTN) (VOID *) ((UINT8 *) (FfsFileHeader) + (UINTN) sizeof (EFI_FFS_FILE_HEADER));

    do {
      SectionLength         = *(UINT32 *) (Section->Size) & 0x00ffffff;
      OccupiedSectionLength = GetOccupiedSize (SectionLength, 4);

      //
      // Was the DXE Core file encapsulated in a GUID'd section?
      //
      if (Section->Type == EFI_SECTION_GUID_DEFINED) {
        //
        // Locate the GUID'd Section Extractor
        //
        GuidedSectionHeader = (VOID *) (Section + 1);

        //
        // This following code constitutes the addition of the security model
        // to the DXE IPL.
        //
        //
        // Set a default authenticatino state
        //
        AuthenticationStatus = 0;

        Status = PeiServicesLocatePpi (
                   &gEfiPeiSectionExtractionPpiGuid,
                   0,
                   NULL,
                   (VOID **)&SectionExtract
                   );

        if (EFI_ERROR (Status)) {
          return Status;
        }
        //
        // Verify Authentication State
        //
        CopyMem (&TempGuid, Section + 1, sizeof (EFI_GUID));

        Status = SectionExtract->PeiGetSection (
                                  GetPeiServicesTablePointer(),
                                  SectionExtract,
                                  (EFI_SECTION_TYPE *) &SectionType,
                                  &TempGuid,
                                  0,
                                  (VOID **) &Buffer,
                                  &BufferSize,
                                  &AuthenticationStatus
                                  );

        if (EFI_ERROR (Status)) {
          return Status;
        }
        //
        // If not ask the Security PPI, if exists, for disposition
        //
        //
        Status = PeiServicesLocatePpi (
                   &gEfiPeiSecurityPpiGuid,
                   0,
                   NULL,
                   (VOID **)&Security
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }

        Status = Security->AuthenticationState (
                            GetPeiServicesTablePointer(),
                            (struct _EFI_PEI_SECURITY_PPI *) Security,
                            AuthenticationStatus,
                            FfsFileHeader,
                            &StartCrisisRecovery
                            );

        if (EFI_ERROR (Status)) {
          return Status;
        }
        //
        // If there is a security violation, report to caller and have
        // the upper-level logic possible engender a crisis recovery
        //
        if (StartCrisisRecovery) {
          return EFI_SECURITY_VIOLATION;
        }
      }

      if (Section->Type == EFI_SECTION_PE32) {
        //
        // This is what we want
        //
        *Pe32Data = (VOID *) (Section + 1);
        return EFI_SUCCESS;
      } else if (Section->Type == EFI_SECTION_COMPRESSION) {
        //
        // This is a compression set, expand it
        //
        CompressionSection  = (EFI_COMPRESSION_SECTION *) Section;

        switch (CompressionSection->CompressionType) {
        case EFI_STANDARD_COMPRESSION:
          DecompressLibrary = &gTianoDecompress;
          break;

        case EFI_CUSTOMIZED_COMPRESSION:
          //
          // Load user customized compression protocol.
          //
          DecompressLibrary = &gCustomDecompress;
          break;

        case EFI_NOT_COMPRESSED:
        default:
          //
          // Need to support not compressed file
          //
          ASSERT_EFI_ERROR (Status);
          return EFI_NOT_FOUND;
        }

        Status = DecompressLibrary->GetInfo (
                   (UINT8 *) ((EFI_COMPRESSION_SECTION *) Section + 1),
                   (UINT32) SectionLength - sizeof (EFI_COMPRESSION_SECTION),
                   &DstBufferSize,
                   &ScratchBufferSize
                   );
        if (EFI_ERROR (Status)) {
          //
          // GetInfo failed
          //
          return EFI_NOT_FOUND;
        }

        //
        // Allocate scratch buffer
        //
        ScratchBuffer = AllocatePages (EFI_SIZE_TO_PAGES (ScratchBufferSize));
        if (ScratchBuffer == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        //
        // Allocate destination buffer
        //
        DstBuffer = AllocatePages (EFI_SIZE_TO_PAGES (DstBufferSize));
        if (DstBuffer == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        //
        // Call decompress function
        //
        Status = DecompressLibrary->Decompress (
                    (CHAR8 *) ((EFI_COMPRESSION_SECTION *) Section + 1),
                    DstBuffer,
                    ScratchBuffer
                    );

        CmpSection = (EFI_COMMON_SECTION_HEADER *) DstBuffer;
        if (CmpSection->Type == EFI_SECTION_RAW) {
          //
          // Skip the section header and
          // adjust the pointer alignment to 16
          //
          FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (DstBuffer + 16);

          if (FvHeader->Signature == EFI_FVH_SIGNATURE) {
            FfsFileHeader = NULL;
            BuildFvHob ((EFI_PHYSICAL_ADDRESS) (UINTN) FvHeader, FvHeader->FvLength);
            Status = PeiServicesFfsFindNextFile (
                       EFI_FV_FILETYPE_DXE_CORE,
                       FvHeader,
                       &FfsFileHeader
                       );

            if (EFI_ERROR (Status)) {
              return EFI_NOT_FOUND;
            }

            //
            // Reture the FfsHeader that contain Pe32Data.
            //
            *RealFfsFileHeader = FfsFileHeader;
            return PeiProcessFile (SectionType, RealFfsFileHeader, Pe32Data);
          }
        }
        //
        // Decompress successfully.
        // Loop the decompressed data searching for expected section.
        //
        CmpFileData = (VOID *) DstBuffer;
        CmpFileSize = DstBufferSize;
        do {
          CmpSectionLength = *(UINT32 *) (CmpSection->Size) & 0x00ffffff;
          if (CmpSection->Type == EFI_SECTION_PE32) {
            //
            // This is what we want
            //
            *Pe32Data = (VOID *) (CmpSection + 1);
            return EFI_SUCCESS;
          }

          OccupiedCmpSectionLength  = GetOccupiedSize (CmpSectionLength, 4);
          CmpSection                = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) CmpSection + OccupiedCmpSectionLength);
        } while (CmpSection->Type != 0 && (UINTN) ((UINT8 *) CmpSection - (UINT8 *) CmpFileData) < CmpFileSize);
      }

      Section   = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) Section + OccupiedSectionLength);
      FileSize  = FfsFileHeader->Size[0] & 0xFF;
      FileSize += (FfsFileHeader->Size[1] << 8) & 0xFF00;
      FileSize += (FfsFileHeader->Size[2] << 16) & 0xFF0000;
      FileSize &= 0x00FFFFFF;
    } while (Section->Type != 0 && (UINTN) ((UINT8 *) Section - (UINT8 *) FfsFileHeader) < FileSize);

    //
    // End of the decompression activity
    //
  } else {

    Status = PeiServicesFfsFindSectionData (
               EFI_SECTION_PE32,
               FfsFileHeader,
               &SectionData
               );

    if (EFI_ERROR (Status)) {
      Status = PeiServicesFfsFindSectionData (
                 EFI_SECTION_TE,
                 FfsFileHeader,
                 &SectionData
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  *Pe32Data = SectionData;

  return EFI_SUCCESS;
}
