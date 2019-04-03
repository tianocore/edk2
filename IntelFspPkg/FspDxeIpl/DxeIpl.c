/** @file

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeIpl.h"


//
// Module Globals used in the DXE to PEI hand off
// These must be module globals, so the stack can be switched
//
CONST EFI_DXE_IPL_PPI mDxeIplPpi = {
  DxeLoadCore
};

CONST EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI mCustomGuidedSectionExtractionPpi = {
  CustomGuidedSectionExtract
};

CONST EFI_PEI_DECOMPRESS_PPI mDecompressPpi = {
  Decompress
};

CONST EFI_PEI_PPI_DESCRIPTOR mPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiDxeIplPpiGuid,
    (VOID *) &mDxeIplPpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiDecompressPpiGuid,
    (VOID *) &mDecompressPpi
  }
};

CONST EFI_PEI_PPI_DESCRIPTOR gEndOfPeiSignalPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  NULL
};

/**
  Entry point of DXE IPL PEIM.

  This function installs DXE IPL PPI and Decompress PPI.  It also reloads
  itself to memory on non-S3 resume boot path.

  @param[in] FileHandle  Handle of the file being invoked.
  @param[in] PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point of DXE IPL PEIM executes successfully.
  @retval Others      Some error occurs during the execution of this function.

**/
EFI_STATUS
EFIAPI
PeimInitializeDxeIpl (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                                Status;
  EFI_GUID                                  *ExtractHandlerGuidTable;
  UINTN                                     ExtractHandlerNumber;
  EFI_PEI_PPI_DESCRIPTOR                    *GuidPpi;

  //
  // Get custom extract guided section method guid list
  //
  ExtractHandlerNumber = ExtractGuidedSectionGetGuidList (&ExtractHandlerGuidTable);

  //
  // Install custom extraction guid PPI
  //
  if (ExtractHandlerNumber > 0) {
    GuidPpi = (EFI_PEI_PPI_DESCRIPTOR *) AllocatePool (ExtractHandlerNumber * sizeof (EFI_PEI_PPI_DESCRIPTOR));
    ASSERT (GuidPpi != NULL);
    while (ExtractHandlerNumber-- > 0) {
      GuidPpi->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
      GuidPpi->Ppi   = (VOID *) &mCustomGuidedSectionExtractionPpi;
      GuidPpi->Guid  = &ExtractHandlerGuidTable[ExtractHandlerNumber];
      Status = PeiServicesInstallPpi (GuidPpi++);
      ASSERT_EFI_ERROR(Status);
    }
  }

  //
  // Install DxeIpl and Decompress PPIs.
  //
  Status = PeiServicesInstallPpi (mPpiList);
  ASSERT_EFI_ERROR(Status);

  return Status;
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

  @param[in]  This                   Indicates the
                                     EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI instance.
                                     Buffer containing the input GUIDed section to be
                                     processed. OutputBuffer OutputBuffer is
                                     allocated from PEI permanent memory and contains
                                     the new section stream.
  @param[in]  InputSection           A pointer to the input buffer, which contains
                                     the input section to be processed.
  @param[out] OutputBuffer           A pointer to a caller-allocated buffer, whose
                                     size is specified by the contents of OutputSize.
  @param[out] OutputSize             A pointer to a caller-allocated
                                     UINTN in which the size of *OutputBuffer
                                     allocation is stored. If the function
                                     returns anything other than EFI_SUCCESS,
                                     the value of OutputSize is undefined.
  @param[out] AuthenticationStatus   A pointer to a caller-allocated
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

  @retval EFI_INVALID_PARAMETER The GUID in InputSection does
                                not match this instance of the
                                GUIDed Section Extraction PPI.

**/
EFI_STATUS
EFIAPI
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
    DEBUG ((DEBUG_ERROR, "GetInfo from guided section Failed - %r\n", Status));
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

  if (((SectionAttribute & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) != 0) && OutputBufferSize > 0) {
    //
    // Allocate output buffer
    //
    *OutputBuffer = AllocatePages (EFI_SIZE_TO_PAGES (OutputBufferSize) + 1);
    if (*OutputBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    DEBUG ((DEBUG_INFO, "Customized Guided section Memory Size required is 0x%x and address is 0x%p\n", OutputBufferSize, *OutputBuffer));
    //
    // *OutputBuffer still is one section. Adjust *OutputBuffer offset,
    // skip EFI section header to make section data at page alignment.
    //
    *OutputBuffer = (VOID *)((UINT8 *) *OutputBuffer + EFI_PAGE_SIZE - sizeof (EFI_COMMON_SECTION_HEADER));
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
    DEBUG ((DEBUG_ERROR, "Extract guided section Failed - %r\n", Status));
    return Status;
  }

  *OutputSize = (UINTN) OutputBufferSize;

  return EFI_SUCCESS;
}



/**
   Decompresses a section to the output buffer.

   This function looks up the compression type field in the input section and
   applies the appropriate compression algorithm to compress the section to a
   callee allocated buffer.

   @param[in]  This                  Points to this instance of the
                                     EFI_PEI_DECOMPRESS_PEI PPI.
   @param[in]  CompressionSection    Points to the compressed section.
   @param[out] OutputBuffer          Holds the returned pointer to the decompressed
                                     sections.
   @param[out] OutputSize            Holds the returned size of the decompress
                                     section streams.

   @retval EFI_SUCCESS           The section was decompressed successfully.
                                 OutputBuffer contains the resulting data and
                                 OutputSize contains the resulting size.

**/
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
  UINT32                          DstBufferSize;
  UINT32                          ScratchBufferSize;
  VOID                            *CompressionSource;
  UINT32                          CompressionSourceSize;
  UINT32                          UncompressedLength;
  UINT8                           CompressionType;

  if (CompressionSection->CommonHeader.Type != EFI_SECTION_COMPRESSION) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (IS_SECTION2 (CompressionSection)) {
    CompressionSource = (VOID *) ((UINT8 *) CompressionSection + sizeof (EFI_COMPRESSION_SECTION2));
    CompressionSourceSize = (UINT32) (SECTION2_SIZE (CompressionSection) - sizeof (EFI_COMPRESSION_SECTION2));
    UncompressedLength = ((EFI_COMPRESSION_SECTION2 *) CompressionSection)->UncompressedLength;
    CompressionType = ((EFI_COMPRESSION_SECTION2 *) CompressionSection)->CompressionType;
  } else {
    CompressionSource = (VOID *) ((UINT8 *) CompressionSection + sizeof (EFI_COMPRESSION_SECTION));
    CompressionSourceSize = (UINT32) (SECTION_SIZE (CompressionSection) - sizeof (EFI_COMPRESSION_SECTION));
    UncompressedLength = CompressionSection->UncompressedLength;
    CompressionType = CompressionSection->CompressionType;
  }

  //
  // This is a compression set, expand it
  //
  switch (CompressionType) {
  case EFI_STANDARD_COMPRESSION:
    //
    // Load EFI standard compression.
    // For compressed data, decompress them to destination buffer.
    //
    Status = UefiDecompressGetInfo (
               CompressionSource,
               CompressionSourceSize,
               &DstBufferSize,
               &ScratchBufferSize
               );
    if (EFI_ERROR (Status)) {
      //
      // GetInfo failed
      //
      DEBUG ((DEBUG_ERROR, "Decompress GetInfo Failed - %r\n", Status));
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
    // Allocate destination buffer, extra one page for adjustment
    //
    DstBuffer = AllocatePages (EFI_SIZE_TO_PAGES (DstBufferSize) + 1);
    if (DstBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // DstBuffer still is one section. Adjust DstBuffer offset, skip EFI section header
    // to make section data at page alignment.
    //
    DstBuffer = DstBuffer + EFI_PAGE_SIZE - sizeof (EFI_COMMON_SECTION_HEADER);
    //
    // Call decompress function
    //
    Status = UefiDecompress (
                CompressionSource,
                DstBuffer,
                ScratchBuffer
                );
    if (EFI_ERROR (Status)) {
      //
      // Decompress failed
      //
      DEBUG ((DEBUG_ERROR, "Decompress Failed - %r\n", Status));
      return EFI_NOT_FOUND;
    }
    break;

  case EFI_NOT_COMPRESSED:
    //
    // Allocate destination buffer
    //
    DstBufferSize = UncompressedLength;
    DstBuffer     = AllocatePages (EFI_SIZE_TO_PAGES (DstBufferSize) + 1);
    if (DstBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Adjust DstBuffer offset, skip EFI section header
    // to make section data at page alignment.
    //
    DstBuffer = DstBuffer + EFI_PAGE_SIZE - sizeof (EFI_COMMON_SECTION_HEADER);
    //
    // stream is not actually compressed, just encapsulated.  So just copy it.
    //
    CopyMem (DstBuffer, CompressionSource, DstBufferSize);
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

/**
   Main entry point to last PEIM.

   This function finds DXE Core in the firmware volume and transfer the control to
   DXE core.

   @param[in] This          Entry point for DXE IPL PPI.
   @param[in] PeiServices   General purpose services available to every PEIM.
   @param[in] HobList       Address to the Pei HOB list.

   @return EFI_SUCCESS              DXE core was successfully loaded.
   @return EFI_OUT_OF_RESOURCES     There are not enough resources to load DXE core.

**/
EFI_STATUS
EFIAPI
DxeLoadCore (
  IN CONST EFI_DXE_IPL_PPI *This,
  IN EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PEI_HOB_POINTERS  HobList
  )
{
  EFI_STATUS   Status;

  DEBUG ((DEBUG_INFO | DEBUG_INIT, "FSP HOB is located at 0x%08X\n", HobList));

  //
  // End of PEI phase signal
  //
  Status = PeiServicesInstallPpi (&gEndOfPeiSignalPpi);
  ASSERT_EFI_ERROR (Status);

  //
  // Give control back to BootLoader after FspInit
  //
  DEBUG ((DEBUG_INFO | DEBUG_INIT, "FSP is waiting for NOTIFY\n"));
  FspInitDone ();

  //
  // BootLoader called FSP again through NotifyPhase
  //
  FspWaitForNotify ();


  //
  // Give control back to the boot loader framework caller
  //
  DEBUG ((DEBUG_INFO | DEBUG_INIT,   "============= PEIM FSP is Completed =============\n\n"));

  SetFspApiReturnStatus(EFI_SUCCESS);

  SetFspMeasurePoint (FSP_PERF_ID_API_NOTIFY_RDYBOOT_EXIT);

  Pei2LoaderSwitchStack();

  //
  // Should not come here
  //
  while (TRUE) {
    DEBUG ((DEBUG_ERROR, "No FSP API should be called after FSP is DONE!\n"));
    SetFspApiReturnStatus(EFI_UNSUPPORTED);
    Pei2LoaderSwitchStack();
  }

  return EFI_SUCCESS;
}
