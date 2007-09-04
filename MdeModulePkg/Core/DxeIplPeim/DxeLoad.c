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

#include "DxeIpl.h"
#include <Ppi/GuidedSectionExtraction.h>

// porting note remove later
#include "FrameworkPei.h"
// end of remove later

EFI_STATUS
CustomDecompressExtractSection (
  IN CONST  EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI *This,
  IN CONST  VOID                                  *InputSection,
  OUT       VOID                                  **OutputBuffer,
  OUT       UINTN                                 *OutputSize,
  OUT       UINT32                                *AuthenticationStatus
);

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

static EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI mCustomDecompressExtractiongPpi = {
  CustomDecompressExtractSection
};

static EFI_PEI_PPI_DESCRIPTOR     mPpiList[] = {
  {
  EFI_PEI_PPI_DESCRIPTOR_PPI,
  &gEfiPeiFvFileLoaderPpiGuid,
  &mLoadFilePpi
  },
  {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiDxeIplPpiGuid,
  &mDxeIplPpi
  }
};

static EFI_PEI_PPI_DESCRIPTOR     mPpiSignal = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  NULL
};

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
  EFI_GUID                                  **DecompressGuidList;
  UINT32                                    DecompressMethodNumber;
  EFI_PEI_PPI_DESCRIPTOR                    *GuidPpi;
  
  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  if (!gInMemory && (BootMode != BOOT_ON_S3_RESUME)) {   
    //
    // The DxeIpl has not yet been shadowed
    //
    PeiEfiPeiPeCoffLoader = (EFI_PEI_PE_COFF_LOADER_PROTOCOL *)GetPeCoffLoaderProtocol ();

    //
    // Shadow DxeIpl and then re-run its entry point
    //
    Status = ShadowDxeIpl (FfsHeader, PeiEfiPeiPeCoffLoader);
  } else {
    //
    // Get custom decompress method guid list 
    //
    DecompressGuidList     = NULL;
    DecompressMethodNumber = 0;
    Status = CustomDecompressGetAlgorithms (DecompressGuidList, &DecompressMethodNumber);
    if (Status == EFI_OUT_OF_RESOURCES) {
      DecompressGuidList = (EFI_GUID **) AllocatePages (EFI_SIZE_TO_PAGES (DecompressMethodNumber * sizeof (EFI_GUID *)));
      ASSERT (DecompressGuidList != NULL);
      Status = CustomDecompressGetAlgorithms (DecompressGuidList, &DecompressMethodNumber);
    }
    ASSERT_EFI_ERROR(Status);

    //
    // Install custom decompress extraction guid ppi
    //
    if (DecompressMethodNumber > 0) {
      GuidPpi = NULL;
      GuidPpi = (EFI_PEI_PPI_DESCRIPTOR *) AllocatePages (EFI_SIZE_TO_PAGES (DecompressMethodNumber * sizeof (EFI_PEI_PPI_DESCRIPTOR)));
      ASSERT (GuidPpi != NULL);
      while (DecompressMethodNumber-- > 0) {
        GuidPpi->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
        GuidPpi->Ppi   = &mCustomDecompressExtractiongPpi;
        GuidPpi->Guid  = DecompressGuidList [DecompressMethodNumber];
        Status = PeiServicesInstallPpi (GuidPpi++);
        ASSERT_EFI_ERROR(Status);
      }
    }
    
    //
    // Install FvFileLoader and DxeIpl PPIs.
    //
    Status = PeiServicesInstallPpi (mPpiList);
    ASSERT_EFI_ERROR(Status);
  }
  
  return Status;
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
  EFI_GUID                                  DxeCoreFileName;
  EFI_GUID                                  FirmwareFileName;
  VOID                                      *Pe32Data;
  VOID                                      *FvImageData;     
  EFI_PHYSICAL_ADDRESS                      DxeCoreAddress;
  UINT64                                    DxeCoreSize;
  EFI_PHYSICAL_ADDRESS                      DxeCoreEntryPoint;
  EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeiEfiPeiPeCoffLoader;
  EFI_BOOT_MODE                             BootMode;
  EFI_PEI_RECOVERY_MODULE_PPI               *PeiRecovery;
  EFI_PEI_S3_RESUME_PPI                     *S3Resume;

//  PERF_START (PeiServices, L"DxeIpl", NULL, 0);

  //
  // if in S3 Resume, restore configure
  //
  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR(Status);

  if (BootMode == BOOT_ON_S3_RESUME) {
    Status = PeiServicesLocatePpi (
               &gEfiPeiS3ResumePpiGuid,
               0,
               NULL,
               (VOID **)&S3Resume
               );
    ASSERT_EFI_ERROR (Status);

    Status = S3Resume->S3RestoreConfig (PeiServices);
    ASSERT_EFI_ERROR (Status);
  } else if (BootMode == BOOT_IN_RECOVERY_MODE) {

    Status = PeiServicesLocatePpi (
               &gEfiPeiRecoveryModulePpiGuid,
               0,
               NULL,
               (VOID **)&PeiRecovery
               );
    ASSERT_EFI_ERROR (Status);

    Status = PeiRecovery->LoadRecoveryCapsule (PeiServices, PeiRecovery);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Load Recovery Capsule Failed.(Status = %r)\n", Status));
      CpuDeadLoop ();
    }

    //
    // Now should have a HOB with the DXE core w/ the old HOB destroyed
    //
  }

  //
  // Install the PEI Protocols that are shared between PEI and DXE
  //
  PeiEfiPeiPeCoffLoader = (EFI_PEI_PE_COFF_LOADER_PROTOCOL *)GetPeCoffLoaderProtocol ();
  ASSERT (PeiEfiPeiPeCoffLoader != NULL);

  //
  // Find the EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE type compressed Firmware Volume file
  // The file found will be processed by PeiProcessFile: It will first be decompressed to
  // a normal FV, then a corresponding FV type hob will be built. 
  //
  Status = PeiFindFile (
             EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE,
             EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
             &FirmwareFileName,
             &FvImageData
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
  // Add HOB for the DXE Core
  //
  BuildModuleHob (
    &DxeCoreFileName,
    DxeCoreAddress,
    DxeCoreSize,
    DxeCoreEntryPoint
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
  // Report Status Code EFI_SW_PEI_PC_HANDOFF_TO_NEXT
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_CORE_PC_HANDOFF_TO_NEXT
    );

  //
  // Transfer control to the DXE Core
  // The handoff state is simply a pointer to the HOB list
  //

  DEBUG ((EFI_D_INFO, "DXE Core Entry Point 0x%08x\n", (UINTN) DxeCoreEntryPoint));
  HandOffToDxeCore (DxeCoreEntryPoint, HobList, &mPpiSignal);
  //
  // If we get here, then the DXE Core returned.  This is an error
  // Dxe Core should not return.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();

  return EFI_OUT_OF_RESOURCES;
}

EFI_STATUS
PeiFindFile (
  IN  UINT8                  Type,
  IN  EFI_SECTION_TYPE       SectionType,
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
  EFI_STATUS                  Status;
  EFI_PEI_HOB_POINTERS        Hob;


  FwVolHeader   = NULL;
  FfsFileHeader = NULL;
  Status        = EFI_SUCCESS;

  //
  // For each Firmware Volume, look for a specified type
  // of file and break out until no one is found 
  //
  Hob.Raw = GetHobList ();
  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_FV, Hob.Raw)) != NULL) {
    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (Hob.FirmwareVolume->BaseAddress);
    //
    // Make sure the FV HOB does not get corrupted.
    //
    ASSERT (FwVolHeader->Signature == EFI_FVH_SIGNATURE);

    Status = PeiServicesFfsFindNextFile (
               Type,
               (EFI_PEI_FV_HANDLE)    FwVolHeader,
               (EFI_PEI_FILE_HANDLE*) &FfsFileHeader
               );
    if (!EFI_ERROR (Status)) {
      Status = PeiProcessFile (
                 SectionType,
                 FfsFileHeader,
                 Pe32Data,
                 &Hob
                 );
      CopyMem (FileName, &FfsFileHeader->Name, sizeof (EFI_GUID));
      //
      // Find all Fv type ffs to get all FvImage and add them into FvHob
      //
      if (!EFI_ERROR (Status) && (Type != EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE)) {
        return EFI_SUCCESS;
      }
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
    OccupiedSectionLength = GET_OCCUPIED_SIZE (SectionLength, 4);
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
    // Set gInMemory global variable to TRUE to indicate the dxeipl is shadowed.
    //
    *(BOOLEAN *) ((UINTN) &gInMemory + (UINTN) DxeIplEntryPoint - (UINTN) _ModuleEntryPoint) = TRUE;
    Status = ((EFI_PEIM_ENTRY_POINT2) (UINTN) DxeIplEntryPoint) ((EFI_PEI_FILE_HANDLE *) DxeIplFileHeader, GetPeiServicesTablePointer());
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
            EFI_SECTION_TE,
            FfsHeader,
            &Pe32Data,
            NULL
            );
  if (EFI_ERROR (Status)) {
    Status = PeiProcessFile (
              EFI_SECTION_PE32,
              FfsHeader,
              &Pe32Data,
              NULL
              );
    
    if (EFI_ERROR (Status)) {
      return Status;
    }
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
  IN      EFI_SECTION_TYPE       SectionType,
  IN      EFI_FFS_FILE_HEADER    *FfsFileHeader,
  OUT     VOID                   **Pe32Data,
  IN      EFI_PEI_HOB_POINTERS   *OrigHob
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
  UINT8                           *DstBuffer;
  UINT8                           *ScratchBuffer;
  UINTN                           DstBufferSize;
  UINT32                          ScratchBufferSize;
  EFI_COMMON_SECTION_HEADER       *CmpSection;
  UINTN                           CmpSectionLength;
  UINTN                           OccupiedCmpSectionLength;
  VOID                            *CmpFileData;
  UINTN                           CmpFileSize;
  EFI_COMMON_SECTION_HEADER       *Section;
  UINTN                           SectionLength;
  UINTN                           OccupiedSectionLength;
  UINTN                           FileSize;
  EFI_FIRMWARE_VOLUME_HEADER      *FvHeader;
  EFI_COMPRESSION_SECTION         *CompressionSection;
  EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI  *SectionExtract;
  UINT32                          AuthenticationStatus;

  //
  // First try to find the required section in this ffs file.
  //
  Status = PeiServicesFfsFindSectionData (
             SectionType,
             FfsFileHeader,
             Pe32Data
             );
  if (!EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // If not found, the required section may be in guided or compressed section.
  // So, search guided or compressed section to process
  //
  Section   = (EFI_COMMON_SECTION_HEADER *) (UINTN) (VOID *) ((UINT8 *) (FfsFileHeader) + (UINTN) sizeof (EFI_FFS_FILE_HEADER));
  FileSize  = FfsFileHeader->Size[0] & 0xFF;
  FileSize += (FfsFileHeader->Size[1] << 8) & 0xFF00;
  FileSize += (FfsFileHeader->Size[2] << 16) & 0xFF0000;
  FileSize &= 0x00FFFFFF;
  OccupiedSectionLength = 0;

  do {
    //
    // Initialize local variables.
    //
    DstBuffer         = NULL;
    DstBufferSize     = 0; 

    Section               = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) Section + OccupiedSectionLength);
    SectionLength         = *(UINT32 *) (Section->Size) & 0x00ffffff;
    OccupiedSectionLength = GET_OCCUPIED_SIZE (SectionLength, 4);

    //
    // Was the DXE Core file encapsulated in a GUID'd section?
    //
    if (Section->Type == EFI_SECTION_GUID_DEFINED) {
      //
      // Set a default authenticatino state
      //
      AuthenticationStatus = 0;
      //
      // Locate extract guid section ppi
      //
      Status = PeiServicesLocatePpi (
                 (EFI_GUID *) (Section + 1),
                 0,
                 NULL,
                 (VOID **)&SectionExtract
                 );

      if (EFI_ERROR (Status)) {
        //
        // ignore the unknown guid section
        //
        continue;
      }
      //
      // Extract the contents from guid section
      //
      Status = SectionExtract->ExtractSection (
                                SectionExtract,
                                (VOID *) Section,
                                (VOID **) &DstBuffer,
                                &DstBufferSize,
                                &AuthenticationStatus
                                );

      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Extract section content failed - %r\n", Status));
        return Status;
      }
      
      //
      // Todo check AuthenticationStatus and do the verify
      //
    } else if (Section->Type == EFI_SECTION_COMPRESSION) {
      //
      // This is a compression set, expand it
      //
      CompressionSection  = (EFI_COMPRESSION_SECTION *) Section;

      switch (CompressionSection->CompressionType) {
      case EFI_STANDARD_COMPRESSION:
        //
        // Load EFI standard compression.
        // For compressed data, decompress them to dstbuffer.
        //
        Status = UefiDecompressGetInfo (
                   (UINT8 *) ((EFI_COMPRESSION_SECTION *) Section + 1),
                   (UINT32) SectionLength - sizeof (EFI_COMPRESSION_SECTION),
                   (UINT32 *) &DstBufferSize,
                   &ScratchBufferSize
                   );
        if (EFI_ERROR (Status)) {
          //
          // GetInfo failed
          //
          DEBUG ((EFI_D_ERROR, "Decompress GetInfo Failed - %r\n", Status));
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
        Status = UefiDecompress (
                    (CHAR8 *) ((EFI_COMPRESSION_SECTION *) Section + 1),
                    DstBuffer,
                    ScratchBuffer
                    );
        if (EFI_ERROR (Status)) {
          //
          // Decompress failed
          //
          DEBUG ((EFI_D_ERROR, "Decompress Failed - %r\n", Status));
          return EFI_NOT_FOUND;
        }
        break;

      // porting note the original branch for customized compress is removed, it should be change to use GUID compress

      case EFI_NOT_COMPRESSED:
        //
        // Allocate destination buffer
        //
        DstBufferSize = CompressionSection->UncompressedLength;
        DstBuffer     = AllocatePages (EFI_SIZE_TO_PAGES (DstBufferSize));
        if (DstBuffer == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }
        //
        // stream is not actually compressed, just encapsulated.  So just copy it.
        //
        CopyMem (DstBuffer, CompressionSection + 1, DstBufferSize);
        break;

      default:
        //
        // Don't support other unknown compression type.
        //
        ASSERT_EFI_ERROR (Status);
        return EFI_NOT_FOUND;
      }
    } else {
      //
      // ignore other type sections
      //
      continue;
    }

    //
    // Extract contents from guided or compressed sections.
    // Loop the decompressed data searching for expected section.
    //
    CmpSection = (EFI_COMMON_SECTION_HEADER *) DstBuffer;
    CmpFileData = (VOID *) DstBuffer;
    CmpFileSize = DstBufferSize;
    do {
      CmpSectionLength          = *(UINT32 *) (CmpSection->Size) & 0x00ffffff;
      if (CmpSection->Type == SectionType) {
        //
        // This is what we want
        //
        if (SectionType == EFI_SECTION_FIRMWARE_VOLUME_IMAGE) {
          // 
          // Firmware Volume Image in this Section
          // Skip the section header to get FvHeader
          //
          FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (CmpSection + 1);

          if (FvHeader->Signature == EFI_FVH_SIGNATURE) {
            //
            // Because FvLength in FvHeader is UINT64 type, 
            // so FvHeader must meed at least 8 bytes alignment.
            // If current FvImage base address doesn't meet its alignment,
            // we need to reload this FvImage to another correct memory address.
            //
            if (((UINTN) FvHeader % sizeof (UINT64)) != 0) {
              CopyMem (DstBuffer, FvHeader, (UINTN) CmpSectionLength - sizeof (EFI_COMMON_SECTION_HEADER));
              FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) DstBuffer;  
            }

            //
            // Build new FvHob for new decompressed Fv image.
            //
            BuildFvHob ((EFI_PHYSICAL_ADDRESS) (UINTN) FvHeader, FvHeader->FvLength);
            
            //
            // Set the original FvHob to unused.
            //
            if (OrigHob != NULL) {
              OrigHob->Header->HobType = EFI_HOB_TYPE_UNUSED;
            }
            //
            // return found FvImage data.
            //
            *Pe32Data = (VOID *) FvHeader;
            return EFI_SUCCESS;
          }
        } else {
          //
          // direct return the found section.
          //
          *Pe32Data = (VOID *) (CmpSection + 1);
          return EFI_SUCCESS;
        }
      }
      OccupiedCmpSectionLength  = GET_OCCUPIED_SIZE (CmpSectionLength, 4);
      CmpSection                = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) CmpSection + OccupiedCmpSectionLength);
    } while (CmpSection->Type != 0 && (UINTN) ((UINT8 *) CmpSection - (UINT8 *) CmpFileData) < CmpFileSize);
  } while (Section->Type != 0 && (UINTN) ((UINT8 *) Section + OccupiedSectionLength - (UINT8 *) FfsFileHeader) < FileSize);

  //
  // search all sections (compression and non compression) in this FFS, don't 
  // find expected section.
  //
  return EFI_NOT_FOUND;
}

/**
  The ExtractSection() function processes the input section and
  returns a pointer to the section contents. If the section being
  extracted does not require processing (if the section
  GuidedSectionHeader.Attributes has the
  EFI_GUIDED_SECTION_PROCESSING_REQUIRED field cleared), then
  OutputBuffer is just updated to point to the start of the
  section's contents. Otherwise, *Buffer must be allocated
  from PEI permanent memory.

  @param This                   Indicates the
                                EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI instance.
                                Buffer containing the input GUIDed section to be
                                processed. OutputBuffer OutputBuffer is
                                allocated from PEI permanent memory and contains
                                the new section stream.
  
  @param OutputSize             A pointer to a caller-allocated
                                UINTN in which the size of *OutputBuffer
                                allocation is stored. If the function
                                returns anything other than EFI_SUCCESS,
                                the value of OutputSize is undefined.
  
  @param AuthenticationStatus   A pointer to a caller-allocated
                                UINT32 that indicates the
                                authentication status of the
                                output buffer. If the input
                                section's GuidedSectionHeader.
                                Attributes field has the
                                EFI_GUIDED_SECTION_AUTH_STATUS_VALID 
                                bit as clear,
                                AuthenticationStatus must return
                                zero. These bits reflect the
                                status of the extraction
                                operation. If the function
                                returns anything other than
                                EFI_SUCCESS, the value of
                                AuthenticationStatus is
                                undefined.
  
  @retval EFI_SUCCESS           The InputSection was
                                successfully processed and the
                                section contents were returned.
  
  @retval EFI_OUT_OF_RESOURCES  The system has insufficient
                                resources to process the request.
  
  @reteval EFI_INVALID_PARAMETER The GUID in InputSection does
                                not match this instance of the
                                GUIDed Section Extraction PPI.
**/
EFI_STATUS
CustomDecompressExtractSection (
  IN CONST  EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI *This,
  IN CONST  VOID                                  *InputSection,
  OUT       VOID                                  **OutputBuffer,
  OUT       UINTN                                 *OutputSize,
  OUT       UINT32                                *AuthenticationStatus
)
{
  EFI_STATUS      Status;
  UINT8           *ScratchBuffer;
  UINT32          ScratchSize;
  UINT32          SectionLength;
  UINT32          DestinationSize;  
  
  //
  // Set authentic value to zero.
  //
  *AuthenticationStatus = 0;
  //
  // Calculate Section data Size
  //
  SectionLength   = *(UINT32 *) (((EFI_COMMON_SECTION_HEADER *) InputSection)->Size) & 0x00ffffff;
  //
  // Get compressed data information
  //
  Status = CustomDecompressGetInfo (
             (GUID *) ((UINT8 *) InputSection + sizeof (EFI_COMMON_SECTION_HEADER)),
             (UINT8 *) InputSection + sizeof (EFI_GUID_DEFINED_SECTION),
             SectionLength - sizeof (EFI_GUID_DEFINED_SECTION),
             &DestinationSize,
             &ScratchSize
             );
  if (EFI_ERROR (Status)) {
    //
    // GetInfo failed
    //
    DEBUG ((EFI_D_ERROR, "Extract guided section Failed - %r\n", Status));
    return Status;
  }

  //
  // Allocate scratch buffer
  //
  ScratchBuffer = AllocatePages (EFI_SIZE_TO_PAGES (ScratchSize));
  if (ScratchBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Allocate destination buffer
  //
  *OutputSize   = (UINTN) DestinationSize;
  *OutputBuffer = AllocatePages (EFI_SIZE_TO_PAGES (*OutputSize));
  if (*OutputBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Call decompress function
  //
  Status = CustomDecompress (
             (GUID *) ((UINT8 *) InputSection + sizeof (EFI_COMMON_SECTION_HEADER)),
             (UINT8 *) InputSection + sizeof (EFI_GUID_DEFINED_SECTION),
             *OutputBuffer,
             ScratchBuffer
             );

  if (EFI_ERROR (Status)) {
    //
    // Decompress failed
    //
    DEBUG ((EFI_D_ERROR, "Extract guided section Failed - %r\n", Status));
    return Status;
  }
  
  return EFI_SUCCESS;
}
