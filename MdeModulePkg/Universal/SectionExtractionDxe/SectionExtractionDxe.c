/** @file
 Section Extraction DXE Driver

Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PiDxe.h>
#include <Protocol/GuidedSectionExtraction.h>
#include <Library/DebugLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  The ExtractSection() function processes the input section and
  allocates a buffer from the pool in which it returns the section
  contents. If the section being extracted contains
  authentication information (the section's
  GuidedSectionHeader.Attributes field has the
  EFI_GUIDED_SECTION_AUTH_STATUS_VALID bit set), the values
  returned in AuthenticationStatus must reflect the results of
  the authentication operation. Depending on the algorithm and
  size of the encapsulated data, the time that is required to do
  a full authentication may be prohibitively long for some
  classes of systems. To indicate this, use
  EFI_SECURITY_POLICY_PROTOCOL_GUID, which may be published by
  the security policy driver (see the Platform Initialization
  Driver Execution Environment Core Interface Specification for
  more details and the GUID definition). If the
  EFI_SECURITY_POLICY_PROTOCOL_GUID exists in the handle
  database, then, if possible, full authentication should be
  skipped and the section contents simply returned in the
  OutputBuffer. In this case, the
  EFI_AUTH_STATUS_PLATFORM_OVERRIDE bit AuthenticationStatus
  must be set on return. ExtractSection() is callable only from
  TPL_NOTIFY and below. Behavior of ExtractSection() at any
  EFI_TPL above TPL_NOTIFY is undefined. Type EFI_TPL is
  defined in RaiseTPL() in the UEFI 2.0 specification.


  @param This         Indicates the
                      EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL instance.
  @param InputSection Buffer containing the input GUIDed section
                      to be processed. OutputBuffer OutputBuffer
                      is allocated from boot services pool
                      memory and contains the new section
                      stream. The caller is responsible for
                      freeing this buffer.
  @param OutputBuffer *OutputBuffer is allocated from boot services
                      pool memory and contains the new section stream.
                      The caller is responsible for freeing this buffer.
  @param OutputSize   A pointer to a caller-allocated UINTN in
                      which the size of OutputBuffer allocation
                      is stored. If the function returns
                      anything other than EFI_SUCCESS, the value
                      of OutputSize is undefined.

  @param AuthenticationStatus A pointer to a caller-allocated
                              UINT32 that indicates the
                              authentication status of the
                              output buffer. If the input
                              section's
                              GuidedSectionHeader.Attributes
                              field has the
                              EFI_GUIDED_SECTION_AUTH_STATUS_VAL
                              bit as clear, AuthenticationStatus
                              must return zero. Both local bits
                              (19:16) and aggregate bits (3:0)
                              in AuthenticationStatus are
                              returned by ExtractSection().
                              These bits reflect the status of
                              the extraction operation. The bit
                              pattern in both regions must be
                              the same, as the local and
                              aggregate authentication statuses
                              have equivalent meaning at this
                              level. If the function returns
                              anything other than EFI_SUCCESS,
                              the value of AuthenticationStatus
                              is undefined.


  @retval EFI_SUCCESS          The InputSection was successfully
                               processed and the section contents were
                               returned.

  @retval EFI_OUT_OF_RESOURCES The system has insufficient
                               resources to process the
                               request.

  @retval EFI_INVALID_PARAMETER The GUID in InputSection does
                                not match this instance of the
                                GUIDed Section Extraction
                                Protocol.

**/
EFI_STATUS
EFIAPI
CustomGuidedSectionExtract (
  IN CONST  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL *This,
  IN CONST  VOID                                   *InputSection,
  OUT       VOID                                   **OutputBuffer,
  OUT       UINTN                                  *OutputSize,
  OUT       UINT32                                 *AuthenticationStatus
  );

//
// Module global for the Section Extraction Protocol handle
//
EFI_HANDLE mSectionExtractionHandle = NULL;

//
// Module global for the Section Extraction Protocol instance
//
EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL mCustomGuidedSectionExtractionProtocol = {
  CustomGuidedSectionExtract
};

/**
  The ExtractSection() function processes the input section and
  allocates a buffer from the pool in which it returns the section
  contents. If the section being extracted contains
  authentication information (the section's
  GuidedSectionHeader.Attributes field has the
  EFI_GUIDED_SECTION_AUTH_STATUS_VALID bit set), the values
  returned in AuthenticationStatus must reflect the results of
  the authentication operation. Depending on the algorithm and
  size of the encapsulated data, the time that is required to do
  a full authentication may be prohibitively long for some
  classes of systems. To indicate this, use
  EFI_SECURITY_POLICY_PROTOCOL_GUID, which may be published by
  the security policy driver (see the Platform Initialization
  Driver Execution Environment Core Interface Specification for
  more details and the GUID definition). If the
  EFI_SECURITY_POLICY_PROTOCOL_GUID exists in the handle
  database, then, if possible, full authentication should be
  skipped and the section contents simply returned in the
  OutputBuffer. In this case, the
  EFI_AUTH_STATUS_PLATFORM_OVERRIDE bit AuthenticationStatus
  must be set on return. ExtractSection() is callable only from
  TPL_NOTIFY and below. Behavior of ExtractSection() at any
  EFI_TPL above TPL_NOTIFY is undefined. Type EFI_TPL is
  defined in RaiseTPL() in the UEFI 2.0 specification.


  @param This         Indicates the
                      EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL instance.
  @param InputSection Buffer containing the input GUIDed section
                      to be processed. OutputBuffer OutputBuffer
                      is allocated from boot services pool
                      memory and contains the new section
                      stream. The caller is responsible for
                      freeing this buffer.
  @param OutputBuffer *OutputBuffer is allocated from boot services
                      pool memory and contains the new section stream.
                      The caller is responsible for freeing this buffer.
  @param OutputSize   A pointer to a caller-allocated UINTN in
                      which the size of OutputBuffer allocation
                      is stored. If the function returns
                      anything other than EFI_SUCCESS, the value
                      of OutputSize is undefined.

  @param AuthenticationStatus A pointer to a caller-allocated
                              UINT32 that indicates the
                              authentication status of the
                              output buffer. If the input
                              section's
                              GuidedSectionHeader.Attributes
                              field has the
                              EFI_GUIDED_SECTION_AUTH_STATUS_VAL
                              bit as clear, AuthenticationStatus
                              must return zero. Both local bits
                              (19:16) and aggregate bits (3:0)
                              in AuthenticationStatus are
                              returned by ExtractSection().
                              These bits reflect the status of
                              the extraction operation. The bit
                              pattern in both regions must be
                              the same, as the local and
                              aggregate authentication statuses
                              have equivalent meaning at this
                              level. If the function returns
                              anything other than EFI_SUCCESS,
                              the value of AuthenticationStatus
                              is undefined.


  @retval EFI_SUCCESS          The InputSection was successfully
                               processed and the section contents were
                               returned.

  @retval EFI_OUT_OF_RESOURCES The system has insufficient
                               resources to process the
                               request.

  @retval EFI_INVALID_PARAMETER The GUID in InputSection does
                                not match this instance of the
                                GUIDed Section Extraction
                                Protocol.

**/
EFI_STATUS
EFIAPI
CustomGuidedSectionExtract (
  IN CONST  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL *This,
  IN CONST  VOID                                   *InputSection,
  OUT       VOID                                   **OutputBuffer,
  OUT       UINTN                                  *OutputSize,
  OUT       UINT32                                 *AuthenticationStatus
  )
{
  EFI_STATUS      Status;
  VOID            *ScratchBuffer;
  VOID            *AllocatedOutputBuffer;
  UINT32          OutputBufferSize;
  UINT32          ScratchBufferSize;
  UINT16          SectionAttribute;

  //
  // Init local variable
  //
  ScratchBuffer         = NULL;
  AllocatedOutputBuffer = NULL;

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

  if (ScratchBufferSize > 0) {
    //
    // Allocate scratch buffer
    //
    ScratchBuffer = AllocatePool (ScratchBufferSize);
    if (ScratchBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  if (OutputBufferSize > 0) {
    //
    // Allocate output buffer
    //
    AllocatedOutputBuffer = AllocatePool (OutputBufferSize);
    if (AllocatedOutputBuffer == NULL) {
      FreePool (ScratchBuffer);
      return EFI_OUT_OF_RESOURCES;
    }
    *OutputBuffer = AllocatedOutputBuffer;
  }

  //
  // Call decode function to extract raw data from the guided section.
  //
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
    if (AllocatedOutputBuffer != NULL) {
      FreePool (AllocatedOutputBuffer);
    }
    if (ScratchBuffer != NULL) {
      FreePool (ScratchBuffer);
    }
    DEBUG ((DEBUG_ERROR, "Extract guided section Failed - %r\n", Status));
    return Status;
  }

  if (*OutputBuffer != AllocatedOutputBuffer) {
    //
    // OutputBuffer was returned as a different value,
    // so copy section contents to the allocated memory buffer.
    //
    CopyMem (AllocatedOutputBuffer, *OutputBuffer, OutputBufferSize);
    *OutputBuffer = AllocatedOutputBuffer;
  }

  //
  // Set real size of output buffer.
  //
  *OutputSize = (UINTN) OutputBufferSize;

  //
  // Free unused scratch buffer.
  //
  if (ScratchBuffer != NULL) {
    FreePool (ScratchBuffer);
  }

  return EFI_SUCCESS;
}

/**
  Main entry for the Section Extraction DXE module.

  This routine registers the Section Extraction Protocols that have been registered 
  with the Section Extraction Library.
  
  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SectionExtractionDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_GUID    *ExtractHandlerGuidTable;
  UINTN       ExtractHandlerNumber;

  //
  // Get custom extract guided section method guid list
  //
  ExtractHandlerNumber = ExtractGuidedSectionGetGuidList (&ExtractHandlerGuidTable);

  //
  // Install custom guided extraction protocol
  //
  while (ExtractHandlerNumber-- > 0) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mSectionExtractionHandle,
                    &ExtractHandlerGuidTable [ExtractHandlerNumber], &mCustomGuidedSectionExtractionProtocol,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
