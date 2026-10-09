/**
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent
  @file
  LZ4 Decompress GUIDed Section Extraction Library.
  It wraps Lz4 decompress interfaces to GUIDed Section Extraction interfaces
  and registers them into GUIDed handler table.
**/

#include "Lz4DecompressLibInternal.h"

/**
  Retrieves the compressed data range from a GUID-defined section.

  Validates DataOffset before calculating the payload pointer and size.
  DataOffset must not be before the section header or beyond the section size.

  @param[in]  InputSection     GUID-defined section containing compressed data.
  @param[in]  Section2         TRUE if InputSection uses the Section2 format.
  @param[out] SectionData      Pointer to the compressed payload.
  @param[out] SectionDataSize  Size of the compressed payload in bytes.

  @retval EFI_SUCCESS          The payload range was retrieved.
  @retval EFI_INVALID_PARAMETER
                               DataOffset is outside the valid section range.
**/
STATIC
EFI_STATUS
Lz4GetSectionData (
  IN  CONST VOID   *InputSection,
  IN  BOOLEAN      Section2,
  OUT CONST VOID   **SectionData,
  OUT UINT32       *SectionDataSize
  )
{
  UINT32  SectionSize;
  UINT32  HeaderSize;
  UINT16  DataOffset;

  if (Section2) {
    SectionSize = SECTION2_SIZE (InputSection);
    HeaderSize  = sizeof (EFI_GUID_DEFINED_SECTION2);
    DataOffset  = ((EFI_GUID_DEFINED_SECTION2 *)InputSection)->DataOffset;
  } else {
    SectionSize = SECTION_SIZE (InputSection);
    HeaderSize  = sizeof (EFI_GUID_DEFINED_SECTION);
    DataOffset  = ((EFI_GUID_DEFINED_SECTION *)InputSection)->DataOffset;
  }

  if ((DataOffset < HeaderSize) || (DataOffset > SectionSize)) {
    return EFI_INVALID_PARAMETER;
  }

  *SectionData     = (UINT8 *)InputSection + DataOffset;
  *SectionDataSize = SectionSize - DataOffset;
  return EFI_SUCCESS;
}

/**
  Examines a GUIDed section and returns the size of the decoded buffer and the
  size of an scratch buffer required to actually decode the data in a GUIDed section.

  Examines a GUIDed section specified by InputSection.
  If GUID for InputSection does not match the GUID that this handler supports,
  then EFI_UNSUPPORTED is returned.
  If the required information can not be retrieved from InputSection,
  then EFI_INVALID_PARAMETER is returned.
  If the GUID of InputSection does match the GUID that this handler supports,
  then the size required to hold the decoded buffer is returned in OututBufferSize,
  the size of an optional scratch buffer is returned in ScratchSize, and the Attributes field
  from EFI_GUID_DEFINED_SECTION header of InputSection is returned in SectionAttribute.

  If InputSection is NULL, then ASSERT().
  If OutputBufferSize is NULL, then ASSERT().
  If ScratchBufferSize is NULL, then ASSERT().
  If SectionAttribute is NULL, then ASSERT().


  @param[in]  InputSection       A pointer to a GUIDed section of an FFS formatted file.
  @param[out] OutputBufferSize   A pointer to the size, in bytes, of an output buffer required
                                 if the buffer specified by InputSection were decoded.
  @param[out] ScratchBufferSize  A pointer to the size, in bytes, required as scratch space
                                 if the buffer specified by InputSection were decoded.
  @param[out] SectionAttribute   A pointer to the attributes of the GUIDed section. See the Attributes
                                 field of EFI_GUID_DEFINED_SECTION in the PI Specification.

  @retval  EFI_SUCCESS            The information about InputSection was returned.
  @retval  EFI_UNSUPPORTED        The section specified by InputSection does not match the GUID this handler supports.
  @retval  EFI_INVALID_PARAMETER  The information can not be retrieved from the section specified by InputSection.

**/
EFI_STATUS
EFIAPI
Lz4GuidedSectionGetInfo (
  IN  CONST VOID  *InputSection,
  OUT UINT32      *OutputBufferSize,
  OUT UINT32      *ScratchBufferSize,
  OUT UINT16      *SectionAttribute
  )
{
  CONST VOID    *SectionData;
  UINT32        SectionDataSize;
  EFI_STATUS    Status;
  BOOLEAN       Section2;

  ASSERT (InputSection != NULL);
  ASSERT (OutputBufferSize != NULL);
  ASSERT (ScratchBufferSize != NULL);
  ASSERT (SectionAttribute != NULL);

  if ((InputSection == NULL) ||
      (OutputBufferSize == NULL) ||
      (ScratchBufferSize == NULL) ||
      (SectionAttribute == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Section2 = IS_SECTION2 (InputSection);
  if (Section2) {
    if (!CompareGuid (
           &gLz4CustomDecompressGuid,
           &(((EFI_GUID_DEFINED_SECTION2 *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_UNSUPPORTED;
    }

    *SectionAttribute = ((EFI_GUID_DEFINED_SECTION2 *)InputSection)->Attributes;
  } else {
    if (!CompareGuid (
           &gLz4CustomDecompressGuid,
           &(((EFI_GUID_DEFINED_SECTION *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_UNSUPPORTED;
    }

    *SectionAttribute = ((EFI_GUID_DEFINED_SECTION *)InputSection)->Attributes;
  }

  Status = Lz4GetSectionData (
             InputSection,
             Section2,
             &SectionData,
             &SectionDataSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Lz4UefiDecompressGetInfo (
           SectionData,
           SectionDataSize,
           OutputBufferSize,
           ScratchBufferSize
           );
}

/**
  Decompress a LZ4 compressed GUIDed section into a caller allocated output buffer.

  Decodes the GUIDed section specified by InputSection.
  If GUID for InputSection does not match the GUID that this handler supports, then EFI_UNSUPPORTED is returned.
  If the data in InputSection can not be decoded, then EFI_INVALID_PARAMETER is returned.
  If the GUID of InputSection does match the GUID that this handler supports, then InputSection
  is decoded into the buffer specified by OutputBuffer and the authentication status of this
  decode operation is returned in AuthenticationStatus.  If the decoded buffer is identical to the
  data in InputSection, then OutputBuffer is set to point at the data in InputSection.  Otherwise,
  the decoded data will be placed in caller allocated buffer specified by OutputBuffer.

  If InputSection is NULL, then ASSERT().
  If OutputBuffer is NULL, then ASSERT().
  If ScratchBuffer is NULL and this decode operation requires a scratch buffer, then ASSERT().
  If AuthenticationStatus is NULL, then ASSERT().


  @param[in]  InputSection  A pointer to a GUIDed section of an FFS formatted file.
  @param[out] OutputBuffer  A pointer to a buffer that contains the result of a decode operation.
  @param[out] ScratchBuffer A caller allocated buffer that may be required by this function
                            as a scratch buffer to perform the decode operation.
  @param[out] AuthenticationStatus
                            A pointer to the authentication status of the decoded output buffer.
                            See the definition of authentication status in the EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI
                            section of the PI Specification. EFI_AUTH_STATUS_PLATFORM_OVERRIDE must
                            never be set by this handler.

  @retval  EFI_SUCCESS            The buffer specified by InputSection was decoded.
  @retval  EFI_UNSUPPORTED        The section specified by InputSection does not match the GUID this handler supports.
  @retval  EFI_INVALID_PARAMETER  The section specified by InputSection can not be decoded.

**/
EFI_STATUS
EFIAPI
Lz4GuidedSectionExtraction (
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  OUT       VOID    *ScratchBuffer         OPTIONAL,
  OUT       UINT32  *AuthenticationStatus
  )
{
  CONST VOID    *SectionData;
  UINT32        SectionDataSize;
  EFI_STATUS    Status;
  BOOLEAN       Section2;

  ASSERT (OutputBuffer != NULL);
  ASSERT (InputSection != NULL);
  ASSERT (AuthenticationStatus != NULL);

  if ((InputSection == NULL) ||
      (OutputBuffer == NULL) ||
      (AuthenticationStatus == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Section2 = IS_SECTION2 (InputSection);
  if (Section2) {
    if (!CompareGuid (
           &gLz4CustomDecompressGuid,
           &(((EFI_GUID_DEFINED_SECTION2 *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_UNSUPPORTED;
    }

    //
    // Authentication is set to Zero, which may be ignored.
    //
    *AuthenticationStatus = 0;
  } else {
    if (!CompareGuid (
           &gLz4CustomDecompressGuid,
           &(((EFI_GUID_DEFINED_SECTION *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_UNSUPPORTED;
    }

    //
    // Authentication is set to Zero, which may be ignored.
    //
    *AuthenticationStatus = 0;
  }

  Status = Lz4GetSectionData (
             InputSection,
             Section2,
             &SectionData,
             &SectionDataSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Lz4UefiDecompress (
           SectionData,
           SectionDataSize,
           *OutputBuffer,
           ScratchBuffer
           );
}

/**
  Register LZ4Decompress and LZ4DecompressGetInfo handlers with LZ4CustomDecompressGuid.

  @retval  EFI_SUCCESS            Register successfully.
  @retval  EFI_OUT_OF_RESOURCES   No enough memory to store this handler.
**/
EFI_STATUS
EFIAPI
Lz4DecompressLibConstructor (
  VOID
  )
{
  return ExtractGuidedSectionRegisterHandlers (
           &gLz4CustomDecompressGuid,
           Lz4GuidedSectionGetInfo,
           Lz4GuidedSectionExtraction
           );
}
