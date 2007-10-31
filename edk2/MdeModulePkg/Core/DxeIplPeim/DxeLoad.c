/**@file
  Last PEIM.
  Responsibility of this module is to load the DXE Core from a Firmware Volume.

Copyright (c) 2006 - 2007 Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeIpl.h"
#include <Ppi/GuidedSectionExtraction.h>
#include <FrameworkPei.h>

EFI_STATUS
CustomGuidedSectionExtract (
  IN CONST  EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI *This,
  IN CONST  VOID                                  *InputSection,
  OUT       VOID                                  **OutputBuffer,
  OUT       UINTN                                 *OutputSize,
  OUT       UINT32                                *AuthenticationStatus
);

STATIC
EFI_STATUS
EFIAPI 
Decompress (
  IN CONST  EFI_PEI_DECOMPRESS_PPI  *This,
  IN CONST  EFI_COMPRESSION_SECTION *InputSection,
  OUT       VOID                    **OutputBuffer,
  OUT       UINTN                   *OutputSize
);


BOOLEAN gInMemory = FALSE;

//
// Module Globals used in the DXE to PEI handoff
// These must be module globals, so the stack can be switched
//
static EFI_DXE_IPL_PPI mDxeIplPpi = {
  DxeLoadCore
};

STATIC EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI mCustomGuidedSectionExtractionPpi = {
  CustomGuidedSectionExtract
};

STATIC EFI_PEI_DECOMPRESS_PPI mDecompressPpi = {
  Decompress
};

static EFI_PEI_PPI_DESCRIPTOR     mPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiDxeIplPpiGuid,
    &mDxeIplPpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiDecompressPpiGuid,
    &mDecompressPpi
  }
};

static EFI_PEI_PPI_DESCRIPTOR     mPpiSignal = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  NULL
};

/**
  Initializes the Dxe Ipl PPI

  @param  FfsHandle   The handle of FFS file.
  @param  PeiServices General purpose services available to
                      every PEIM.
  @return EFI_SUCESS 
*/ 
EFI_STATUS
EFIAPI
PeimInitializeDxeIpl (
  IN EFI_PEI_FILE_HANDLE       FfsHandle,
  IN EFI_PEI_SERVICES          **PeiServices
  )
{
  EFI_STATUS                                Status;
  EFI_BOOT_MODE                             BootMode;
  EFI_GUID                                  *ExtractHandlerGuidTable;
  UINTN                                     ExtractHandlerNumber;
  EFI_PEI_PPI_DESCRIPTOR                    *GuidPpi;
  
  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  if (BootMode != BOOT_ON_S3_RESUME) {
    Status = PeiServicesRegisterForShadow (FfsHandle);
    if (Status == EFI_SUCCESS) {
      //
      // EFI_SUCESS means the first time call register for shadow 
      // 
      return Status;
    } else if (Status == EFI_ALREADY_STARTED) {
      
      gInMemory = TRUE;
      
      //
      // Get custom extract guided section method guid list 
      //
      ExtractHandlerNumber = ExtractGuidedSectionGetGuidList (&ExtractHandlerGuidTable);
      
      //
      // Install custom extraction guid ppi
      //
      if (ExtractHandlerNumber > 0) {
      	GuidPpi = NULL;
      	GuidPpi = (EFI_PEI_PPI_DESCRIPTOR *) AllocatePool (ExtractHandlerNumber * sizeof (EFI_PEI_PPI_DESCRIPTOR));
      	ASSERT (GuidPpi != NULL);
      	while (ExtractHandlerNumber-- > 0) {
      	  GuidPpi->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
      	  GuidPpi->Ppi   = &mCustomGuidedSectionExtractionPpi;
      	  GuidPpi->Guid  = &(ExtractHandlerGuidTable [ExtractHandlerNumber]);
      	  Status = PeiServicesInstallPpi (GuidPpi++);
      	  ASSERT_EFI_ERROR(Status);
      	}
      }
    } else {
      ASSERT (FALSE);
    }
  }
  
  //
  // Install FvFileLoader and DxeIpl PPIs.
  //
  Status = PeiServicesInstallPpi (mPpiList);
  ASSERT_EFI_ERROR(Status);  
	
  return Status;
}

/**
   Main entry point to last PEIM 
    
   @param This          Entry point for DXE IPL PPI
   @param PeiServices   General purpose services available to every PEIM.
   @param HobList       Address to the Pei HOB list
   
   @return EFI_SUCCESS              DXE core was successfully loaded. 
   @return EFI_OUT_OF_RESOURCES     There are not enough resources to load DXE core.
**/
EFI_STATUS
EFIAPI
DxeLoadCore (
  IN EFI_DXE_IPL_PPI       *This,
  IN EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PEI_HOB_POINTERS  HobList
  )
{
  EFI_STATUS                                Status;
  EFI_GUID                                  DxeCoreFileName;
  EFI_PHYSICAL_ADDRESS                      DxeCoreAddress;
  UINT64                                    DxeCoreSize;
  EFI_PHYSICAL_ADDRESS                      DxeCoreEntryPoint;
  EFI_BOOT_MODE                             BootMode;
  EFI_PEI_FV_HANDLE                         VolumeHandle;
  EFI_PEI_FILE_HANDLE                       FileHandle;
  UINTN                                     Instance;

  //
  // if in S3 Resume, restore configure
  //
  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR(Status);

  if (BootMode == BOOT_ON_S3_RESUME) {
    Status = AcpiS3ResumeOs();
    ASSERT_EFI_ERROR (Status);
  } else if (BootMode == BOOT_IN_RECOVERY_MODE) {
    Status = PeiRecoverFirmware ();
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Load Recovery Capsule Failed.(Status = %r)\n", Status));
      CpuDeadLoop ();
    }

    //
    // Now should have a HOB with the DXE core w/ the old HOB destroyed
    //
  }
  
  //
  // If any FV contains an encapsulated FV extract that FV
  //
  DxeIplAddEncapsulatedFirmwareVolumes ();
  
  //
  // Look in all the FVs present in PEI and find the DXE Core
  //
  Instance = 0;
  Status = DxeIplFindFirmwareVolumeInstance (&Instance, EFI_FV_FILETYPE_DXE_CORE, &VolumeHandle, &FileHandle);
  ASSERT_EFI_ERROR (Status);

  CopyMem(&DxeCoreFileName, &(((EFI_FFS_FILE_HEADER*)FileHandle)->Name), sizeof (EFI_GUID));

  //
  // Load the DXE Core from a Firmware Volume
  //
  Status = PeiLoadFile (
            FileHandle,
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
    EFI_SIZE_TO_PAGES ((UINT32) DxeCoreSize) * EFI_PAGE_SIZE,
    DxeCoreEntryPoint
    );

  //
  // Report Status Code EFI_SW_PEI_PC_HANDOFF_TO_NEXT
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_CORE_PC_HANDOFF_TO_NEXT
    );

  DEBUG_CODE_BEGIN ();

    EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION       PtrPeImage;
    PtrPeImage.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) ((UINTN) DxeCoreAddress + ((EFI_IMAGE_DOS_HEADER *) (UINTN) DxeCoreAddress)->e_lfanew);
    
    if (PtrPeImage.Pe32->FileHeader.Machine != IMAGE_FILE_MACHINE_IA64) {
      DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Loading DXE CORE at 0x%08x EntryPoint=0x%08x\n", (UINTN) DxeCoreAddress, (UINTN) DxeCoreEntryPoint));
    } else {
      //
      // For IPF Image, the real entry point should be print.
      //
      DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Loading DXE CORE at 0x%08x EntryPoint=0x%08x\n", (UINTN) DxeCoreAddress, (UINTN) (*(UINT64 *)(UINTN)DxeCoreEntryPoint)));
    }

  DEBUG_CODE_END ();
  //
  // Transfer control to the DXE Core
  // The handoff state is simply a pointer to the HOB list
  //
  HandOffToDxeCore (DxeCoreEntryPoint, HobList, &mPpiSignal);
  //
  // If we get here, then the DXE Core returned.  This is an error
  // Dxe Core should not return.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();

  return EFI_OUT_OF_RESOURCES;
}


STATIC
EFI_STATUS
GetFvAlignment (
  IN    EFI_FIRMWARE_VOLUME_HEADER   *FvHeader,
  OUT   UINT32                      *FvAlignment
  )
{
  //
  // Because FvLength in FvHeader is UINT64 type, 
  // so FvHeader must meed at least 8 bytes alignment.
  // Get the appropriate alignment requirement.
  // 
  if ((FvHeader->Attributes & EFI_FVB2_ALIGNMENT) < EFI_FVB2_ALIGNMENT_8) {
    return EFI_UNSUPPORTED;
  }
  
   *FvAlignment = 1 << ((FvHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16);
   return EFI_SUCCESS;
}

/**
   Search EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE image and expand 
   as memory FV 
    
   @return EFI_OUT_OF_RESOURCES There are no memory space to exstract FV
   @return EFI_SUCESS           Sucess to find the FV 
**/
EFI_STATUS
DxeIplAddEncapsulatedFirmwareVolumes (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_STATUS                  VolumeStatus;
  UINTN                       Index;
  EFI_FV_INFO                 VolumeInfo; 
  EFI_PEI_FV_HANDLE           VolumeHandle;
  EFI_PEI_FILE_HANDLE         FileHandle;
  UINT32                      SectionLength;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  EFI_FIRMWARE_VOLUME_IMAGE_SECTION *SectionHeader;
  VOID                        *DstBuffer;
  UINT32                       FvAlignment;

  Status = EFI_NOT_FOUND;
  Index  = 0;

  do {
    VolumeStatus = DxeIplFindFirmwareVolumeInstance (
                    &Index, 
                    EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, 
                    &VolumeHandle, 
                    &FileHandle
                    );
                    
    if (!EFI_ERROR (VolumeStatus)) {
         Status = PeiServicesFfsFindSectionData (
                    EFI_SECTION_FIRMWARE_VOLUME_IMAGE, 
                    (EFI_FFS_FILE_HEADER *)FileHandle, 
                    (VOID **)&FvHeader
                    );
                    
      if (!EFI_ERROR (Status)) {
        if (FvHeader->Signature == EFI_FVH_SIGNATURE) {
          //
          // Because FvLength in FvHeader is UINT64 type, 
          // so FvHeader must meed at least 8 bytes alignment.
          // If current FvImage base address doesn't meet its alignment,
          // we need to reload this FvImage to another correct memory address.
          //
          Status = GetFvAlignment(FvHeader, &FvAlignment); 
          if (EFI_ERROR(Status)) {
            return Status;
          }
          if (((UINTN) FvHeader % FvAlignment) != 0) {
            SectionHeader = (EFI_FIRMWARE_VOLUME_IMAGE_SECTION*)((UINTN)FvHeader - sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION));
            SectionLength =  *(UINT32 *)SectionHeader->Size & 0x00FFFFFF;
            
            DstBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES ((UINTN) SectionLength - sizeof (EFI_COMMON_SECTION_HEADER)), FvAlignment);
            if (DstBuffer == NULL) {
              return EFI_OUT_OF_RESOURCES;
            }
            CopyMem (DstBuffer, FvHeader, (UINTN) SectionLength - sizeof (EFI_COMMON_SECTION_HEADER));
            FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) DstBuffer;  
          }

          //
          // This new Firmware Volume comes from a firmware file within a firmware volume.
          // Record the original Firmware Volume Name.
          //
          PeiServicesFfsGetVolumeInfo (&VolumeHandle, &VolumeInfo);

          PiLibInstallFvInfoPpi (
            NULL,
            FvHeader,
            (UINT32) FvHeader->FvLength,
            &(VolumeInfo.FvName),
            &(((EFI_FFS_FILE_HEADER*)FileHandle)->Name)
            );

          //
          // Inform HOB consumer phase, i.e. DXE core, the existance of this FV
          //
          BuildFvHob (
            (EFI_PHYSICAL_ADDRESS) (UINTN) FvHeader,
            FvHeader->FvLength
          );
            
          ASSERT_EFI_ERROR (Status);

          //
          // Makes the encapsulated volume show up in DXE phase to skip processing of
          // encapsulated file again.
          //
          BuildFv2Hob (
            (EFI_PHYSICAL_ADDRESS)(UINTN)FvHeader,
            FvHeader->FvLength, 
            &VolumeInfo.FvName,
            &(((EFI_FFS_FILE_HEADER *)FileHandle)->Name)
            );
          return Status;
        }
      }
    }
  } while (!EFI_ERROR (VolumeStatus));
  
  return Status;
}

/**
   Find the First Volume that contains the first FileType.

   @param Instance      The Fv instance.
   @param SeachType     The type of file to search.
   @param VolumeHandle  Pointer to Fv which contains the file to search. 
   @param FileHandle    Pointer to FFS file to search.
   
   @return EFI_SUCESS   Success to find the FFS in specificed FV
   @return others       Fail to find the FFS in specificed FV
 */
EFI_STATUS
DxeIplFindFirmwareVolumeInstance (
  IN OUT UINTN              *Instance,
  IN  EFI_FV_FILETYPE       SeachType,
  OUT EFI_PEI_FV_HANDLE     *VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE   *FileHandle
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  VolumeStatus;

  do {
    VolumeStatus = PeiServicesFfsFindNextVolume (*Instance, VolumeHandle);
    if (!EFI_ERROR (VolumeStatus)) {
      *FileHandle = NULL;
      Status = PeiServicesFfsFindNextFile (SeachType, *VolumeHandle, FileHandle);
      if (!EFI_ERROR (Status)) {
        return Status;
      }
    }
    *Instance += 1;
  } while (!EFI_ERROR (VolumeStatus));

  return VolumeStatus;
}

/**
   Loads and relocates a PE/COFF image into memory.

   @param FileHandle        The image file handle
   @param ImageAddress      The base address of the relocated PE/COFF image
   @param ImageSize         The size of the relocated PE/COFF image
   @param EntryPoint        The entry point of the relocated PE/COFF image
   
   @return EFI_SUCCESS           The file was loaded and relocated
   @return EFI_OUT_OF_RESOURCES  There was not enough memory to load and relocate the PE/COFF file
**/
EFI_STATUS
PeiLoadFile (
  IN  EFI_PEI_FILE_HANDLE                       FileHandle,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  )
{

  EFI_STATUS                        Status;
  PE_COFF_LOADER_IMAGE_CONTEXT      ImageContext;
  VOID                              *Pe32Data;
  //
  // First try to find the required section in this ffs file.
  //
  Status = PeiServicesFfsFindSectionData (
             EFI_SECTION_PE32,
             FileHandle,
             &Pe32Data
             );

  if (EFI_ERROR (Status)) {
    Status = PeiServicesFfsFindSectionData (
               EFI_SECTION_TE,
               FileHandle,
               &Pe32Data
               );
  }
  
  if (EFI_ERROR (Status)) {
    //
    // NO image types we support so exit.
    //
    return Status;
  }

  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle = Pe32Data;
  Status              = GetImageReadFunction (&ImageContext);

  ASSERT_EFI_ERROR (Status);

  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Allocate Memory for the image
  //
  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) AllocatePages (EFI_SIZE_TO_PAGES ((UINT32) ImageContext.ImageSize));
  ASSERT (ImageContext.ImageAddress != 0);

  //
  // Skip the reserved space for the stripped PeHeader when load TeImage into memory.
  //
  if (ImageContext.IsTeImage) {
    ImageContext.ImageAddress = ImageContext.ImageAddress + 
                                ((EFI_TE_IMAGE_HEADER *) Pe32Data)->StrippedSize -
                                sizeof (EFI_TE_IMAGE_HEADER);
  }

  //
  // Load the image to our new buffer
  //
  Status = PeCoffLoaderLoadImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Relocate the image in our new buffer
  //
  Status = PeCoffLoaderRelocateImage (&ImageContext);
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
CustomGuidedSectionExtract (
  IN CONST  EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI *This,
  IN CONST  VOID                                  *InputSection,
  OUT       VOID                                  **OutputBuffer,
  OUT       UINTN                                 *OutputSize,
  OUT       UINT32                                *AuthenticationStatus
)
{
  EFI_STATUS      Status;
  UINT8           *ScratchBuffer;
  UINT32          ScratchBufferSize;
  UINT32          OutputBufferSize;
  UINT16          SectionAttribute;
  
  //
  // Init local variable
  //
  ScratchBuffer = NULL;

  //
  // Call GetInfo to get the size and attribute of input guided section data.
  //
  Status = ExtractGuidedSectionGetInfo (
            InputSection,
            &OutputBufferSize,
            &ScratchBufferSize,
            &SectionAttribute
           );
  
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "GetInfo from guided section Failed - %r\n", Status));
    return Status;
  }
  
  if (ScratchBufferSize != 0) {
    //
    // Allocate scratch buffer
    //
    ScratchBuffer = AllocatePages (EFI_SIZE_TO_PAGES (ScratchBufferSize));
    if (ScratchBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  if ((SectionAttribute & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) && OutputBufferSize > 0) {  
    //
    // Allocate output buffer
    //
    *OutputBuffer = AllocatePages (EFI_SIZE_TO_PAGES (OutputBufferSize));
    if (*OutputBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }
  
  Status = ExtractGuidedSectionDecode (
             InputSection, 
             OutputBuffer,
             ScratchBuffer,
             AuthenticationStatus
           );

  if (EFI_ERROR (Status)) {
    //
    // Decode failed
    //
    DEBUG ((EFI_D_ERROR, "Extract guided section Failed - %r\n", Status));
    return Status;
  }
  
  *OutputSize = (UINTN) OutputBufferSize;
  
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI 
Decompress (
  IN CONST  EFI_PEI_DECOMPRESS_PPI  *This,
  IN CONST  EFI_COMPRESSION_SECTION *CompressionSection,
  OUT       VOID                    **OutputBuffer,
  OUT       UINTN                   *OutputSize
 )
{
  EFI_STATUS                      Status;
  UINT8                           *DstBuffer;
  UINT8                           *ScratchBuffer;
  UINTN                           DstBufferSize;
  UINT32                          ScratchBufferSize;
  EFI_COMMON_SECTION_HEADER       *Section;
  UINTN                           SectionLength;

  if (CompressionSection->CommonHeader.Type != EFI_SECTION_COMPRESSION) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Section = (EFI_COMMON_SECTION_HEADER *) CompressionSection;
  SectionLength         = *(UINT32 *) (Section->Size) & 0x00ffffff;
  
  //
  // This is a compression set, expand it
  //
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
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  *OutputSize = DstBufferSize;
  *OutputBuffer = DstBuffer;

  return EFI_SUCCESS;
}

