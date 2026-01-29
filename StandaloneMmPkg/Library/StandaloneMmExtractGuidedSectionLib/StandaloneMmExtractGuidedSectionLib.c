/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2024, Arm Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Pi/PiFirmwareFile.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

/**
  Extract guided section handler info.
 */
typedef struct {
  /// Number of handlers.
  UINT32                                     NumberOfExtractHandler;

  /// Padding.
  UINT32                                     Padding;

  /// Guid table of handlers
  GUID                                       *ExtractHandlerGuidTable;

  /// Handler table for decoding correspond guided section.
  EXTRACT_GUIDED_SECTION_DECODE_HANDLER      *ExtractDecodeHandlerTable;

  /// Handler table for getting information about correspond guided section.
  EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER    *ExtractGetInfoHandlerTable;
} EXTRACT_GUIDED_SECTION_HANDLER_INFO;

STATIC EXTRACT_GUIDED_SECTION_HANDLER_INFO  *mHandlerInfo;

/**
  Registers handlers of type EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER and EXTRACT_GUIDED_SECTION_DECODE_HANDLER
  for a specific GUID section type.

  Registers the handlers specified by GetInfoHandler and DecodeHandler with the GUID specified by SectionGuid.
  If the GUID value specified by SectionGuid has already been registered, then return RETURN_ALREADY_STARTED.
  If there are not enough resources available to register the handlers  then RETURN_OUT_OF_RESOURCES is returned.

  If SectionGuid is NULL, then ASSERT().
  If GetInfoHandler is NULL, then ASSERT().
  If DecodeHandler is NULL, then ASSERT().

  @param[in]  SectionGuid    A pointer to the GUID associated with the the handlers
                             of the GUIDed section type being registered.
  @param[in]  GetInfoHandler Pointer to a function that examines a GUIDed section and returns the
                             size of the decoded buffer and the size of an optional scratch buffer
                             required to actually decode the data in a GUIDed section.
  @param[in]  DecodeHandler  Pointer to a function that decodes a GUIDed section into a caller
                             allocated output buffer.

  @retval  RETURN_SUCCESS           The handlers were registered.
  @retval  RETURN_OUT_OF_RESOURCES  There are not enough resources available to register the handlers.

**/
RETURN_STATUS
EFIAPI
ExtractGuidedSectionRegisterHandlers (
  IN CONST  GUID                                     *SectionGuid,
  IN        EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER  GetInfoHandler,
  IN        EXTRACT_GUIDED_SECTION_DECODE_HANDLER    DecodeHandler
  )
{
  UINT32  Index;

  //
  // Check input parameter.
  //
  if (SectionGuid == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if (mHandlerInfo == NULL) {
    return RETURN_NOT_READY;
  }

  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < mHandlerInfo->NumberOfExtractHandler; Index++) {
    if (CompareGuid (&mHandlerInfo->ExtractHandlerGuidTable[Index], SectionGuid)) {
      break;
    }
  }

  //
  // If the guided handler has been registered before, only update its handler.
  //
  if (Index < mHandlerInfo->NumberOfExtractHandler) {
    mHandlerInfo->ExtractDecodeHandlerTable[Index]  = DecodeHandler;
    mHandlerInfo->ExtractGetInfoHandlerTable[Index] = GetInfoHandler;
    return RETURN_SUCCESS;
  }

  //
  // Check the global table is enough to contain new Handler.
  //
  if (mHandlerInfo->NumberOfExtractHandler >= PcdGet32 (PcdMaximumGuidedExtractHandler)) {
    return RETURN_OUT_OF_RESOURCES;
  }

  //
  // Register new Handler and guid value.
  //
  CopyGuid (&mHandlerInfo->ExtractHandlerGuidTable[mHandlerInfo->NumberOfExtractHandler], SectionGuid);
  mHandlerInfo->ExtractDecodeHandlerTable[mHandlerInfo->NumberOfExtractHandler]    = DecodeHandler;
  mHandlerInfo->ExtractGetInfoHandlerTable[mHandlerInfo->NumberOfExtractHandler++] = GetInfoHandler;

  return RETURN_SUCCESS;
}

/**
  Retrieve the list GUIDs that have been registered through ExtractGuidedSectionRegisterHandlers().

  Sets ExtractHandlerGuidTable so it points at a callee allocated array of registered GUIDs.
  The total number of GUIDs in the array are returned. Since the array of GUIDs is callee allocated
  and caller must treat this array of GUIDs as read-only data.
  If ExtractHandlerGuidTable is NULL, then ASSERT().

  @param[in, out]  ExtractHandlerGuidTable  A pointer to the array of GUIDs that have been registered through
                                        ExtractGuidedSectionRegisterHandlers().

  @return the number of the supported extract guided Handler.

**/
UINTN
EFIAPI
ExtractGuidedSectionGetGuidList (
  IN OUT  GUID  **ExtractHandlerGuidTable
  )
{
  ASSERT (ExtractHandlerGuidTable != NULL);
  ASSERT (mHandlerInfo != NULL);

  *ExtractHandlerGuidTable = mHandlerInfo->ExtractHandlerGuidTable;

  return mHandlerInfo->NumberOfExtractHandler;
}

/**
  Retrieves a GUID from a GUIDed section and uses that GUID to select an associated handler of type
  EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER that was registered with ExtractGuidedSectionRegisterHandlers().
  The selected handler is used to retrieve and return the size of the decoded buffer and the size of an
  optional scratch buffer required to actually decode the data in a GUIDed section.

  Examines a GUIDed section specified by InputSection.
  If GUID for InputSection does not match any of the GUIDs registered through ExtractGuidedSectionRegisterHandlers(),
  then RETURN_UNSUPPORTED is returned.
  If the GUID of InputSection does match the GUID that this handler supports, then the the associated handler
  of type EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER that was registered with ExtractGuidedSectionRegisterHandlers()
  is used to retrieve the OututBufferSize, ScratchSize, and Attributes values. The return status from the handler of
  type EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER is returned.

  If InputSection is NULL, then ASSERT().
  If OutputBufferSize is NULL, then ASSERT().
  If ScratchBufferSize is NULL, then ASSERT().
  If SectionAttribute is NULL, then ASSERT().

  @param[in]  InputSection       A pointer to a GUIDed section of an FFS formatted file.
  @param[out] OutputBufferSize   A pointer to the size, in bytes, of an output buffer required if the buffer
                                 specified by InputSection were decoded.
  @param[out] ScratchBufferSize  A pointer to the size, in bytes, required as scratch space if the buffer specified by
                                 InputSection were decoded.
  @param[out] SectionAttribute   A pointer to the attributes of the GUIDed section.  See the Attributes field of
                                 EFI_GUID_DEFINED_SECTION in the PI Specification.

  @retval  RETURN_SUCCESS      Get the required information successfully.
  @retval  RETURN_UNSUPPORTED  The GUID from the section specified by InputSection does not match any of
                               the GUIDs registered with ExtractGuidedSectionRegisterHandlers().
  @retval  Others              The return status from the handler associated with the GUID retrieved from
                               the section specified by InputSection.

**/
RETURN_STATUS
EFIAPI
ExtractGuidedSectionGetInfo (
  IN  CONST VOID    *InputSection,
  OUT       UINT32  *OutputBufferSize,
  OUT       UINT32  *ScratchBufferSize,
  OUT       UINT16  *SectionAttribute
  )
{
  UINT32    Index;
  EFI_GUID  *SectionDefinitionGuid;

  if (InputSection == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  ASSERT (OutputBufferSize != NULL);
  ASSERT (ScratchBufferSize != NULL);
  ASSERT (SectionAttribute != NULL);

  if (IS_SECTION2 (InputSection)) {
    SectionDefinitionGuid = &(((EFI_GUID_DEFINED_SECTION2 *)InputSection)->SectionDefinitionGuid);
  } else {
    SectionDefinitionGuid = &(((EFI_GUID_DEFINED_SECTION *)InputSection)->SectionDefinitionGuid);
  }

  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < mHandlerInfo->NumberOfExtractHandler; Index++) {
    if (CompareGuid (&mHandlerInfo->ExtractHandlerGuidTable[Index], SectionDefinitionGuid)) {
      break;
    }
  }

  //
  // Not found, the input guided section is not supported.
  //
  if (Index == mHandlerInfo->NumberOfExtractHandler) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Call the match handler to getinfo for the input section data.
  //
  return mHandlerInfo->ExtractGetInfoHandlerTable[Index](
                                                         InputSection,
                                                         OutputBufferSize,
                                                         ScratchBufferSize,
                                                         SectionAttribute
                                                         );
}

/**
  Retrieves the GUID from a GUIDed section and uses that GUID to select an associated handler of type
  EXTRACT_GUIDED_SECTION_DECODE_HANDLER that was registered with ExtractGuidedSectionRegisterHandlers().
  The selected handler is used to decode the data in a GUIDed section and return the result in a caller
  allocated output buffer.

  Decodes the GUIDed section specified by InputSection.
  If GUID for InputSection does not match any of the GUIDs registered through ExtractGuidedSectionRegisterHandlers(),
  then RETURN_UNSUPPORTED is returned.
  If the GUID of InputSection does match the GUID that this handler supports, then the the associated handler
  of type EXTRACT_GUIDED_SECTION_DECODE_HANDLER that was registered with ExtractGuidedSectionRegisterHandlers()
  is used to decode InputSection into the buffer specified by OutputBuffer and the authentication status of this
  decode operation is returned in AuthenticationStatus.  If the decoded buffer is identical to the data in InputSection,
  then OutputBuffer is set to point at the data in InputSection.  Otherwise, the decoded data will be placed in caller
  allocated buffer specified by OutputBuffer.    This function is responsible for computing the  EFI_AUTH_STATUS_PLATFORM_OVERRIDE
  bit of in AuthenticationStatus.  The return status from the handler of type EXTRACT_GUIDED_SECTION_DECODE_HANDLER is returned.

  If InputSection is NULL, then ASSERT().
  If OutputBuffer is NULL, then ASSERT().
  If ScratchBuffer is NULL and this decode operation requires a scratch buffer, then ASSERT().
  If AuthenticationStatus is NULL, then ASSERT().

  @param[in]  InputSection   A pointer to a GUIDed section of an FFS formatted file.
  @param[out] OutputBuffer   A pointer to a buffer that contains the result of a decode operation.
  @param[out] ScratchBuffer  A caller allocated buffer that may be required by this function as a scratch buffer to perform the decode operation.
  @param[out] AuthenticationStatus
                             A pointer to the authentication status of the decoded output buffer. See the definition
                             of authentication status in the EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI section of the PI
                             Specification.

  @retval  RETURN_SUCCESS           The buffer specified by InputSection was decoded.
  @retval  RETURN_UNSUPPORTED       The section specified by InputSection does not match the GUID this handler supports.
  @retval  RETURN_INVALID_PARAMETER The section specified by InputSection can not be decoded.

**/
RETURN_STATUS
EFIAPI
ExtractGuidedSectionDecode (
  IN  CONST VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  OUT       VOID    *ScratchBuffer         OPTIONAL,
  OUT       UINT32  *AuthenticationStatus
  )
{
  UINT32    Index;
  EFI_GUID  *SectionDefinitionGuid;

  if (InputSection == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  ASSERT (OutputBuffer != NULL);
  ASSERT (AuthenticationStatus != NULL);

  if (IS_SECTION2 (InputSection)) {
    SectionDefinitionGuid = &(((EFI_GUID_DEFINED_SECTION2 *)InputSection)->SectionDefinitionGuid);
  } else {
    SectionDefinitionGuid = &(((EFI_GUID_DEFINED_SECTION *)InputSection)->SectionDefinitionGuid);
  }

  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < mHandlerInfo->NumberOfExtractHandler; Index++) {
    if (CompareGuid (&mHandlerInfo->ExtractHandlerGuidTable[Index], SectionDefinitionGuid)) {
      break;
    }
  }

  //
  // Not found, the input guided section is not supported.
  //
  if (Index == mHandlerInfo->NumberOfExtractHandler) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Call the match handler to getinfo for the input section data.
  //
  return mHandlerInfo->ExtractDecodeHandlerTable[Index](
                                                        InputSection,
                                                        OutputBuffer,
                                                        ScratchBuffer,
                                                        AuthenticationStatus
                                                        );
}

/**
  ExtraGuidSectionLib constructor for StandaloneMmCore.

  @param  [in]  ImageHandle     The firmware allocated handle for the EFI image.
  @param  [in]  MmSystemTable   A pointer to the Management mode System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
RETURN_STATUS
EFIAPI
StandaloneMmExtractGuidedSectionLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  RETURN_STATUS            Status;
  UINTN                    Idx;
  EFI_CONFIGURATION_TABLE  *ConfigurationTable;
  UINT8                    *Buffer;
  UINTN                    HandlerInfoSize;

  Status = RETURN_NOT_FOUND;

  // Find ExtraGuidedSectionHandlerInfo in ConfigurationTable.
  ConfigurationTable = MmSystemTable->MmConfigurationTable;
  for (Idx = 0; Idx < MmSystemTable->NumberOfTableEntries; Idx++) {
    if (CompareGuid (&gExtraGuidedSectionHandlerInfoGuid, &ConfigurationTable[Idx].VendorGuid)) {
      Status = RETURN_SUCCESS;
      break;
    }
  }

  if (!RETURN_ERROR (Status)) {
    mHandlerInfo = ConfigurationTable[Idx].VendorTable;
    return RETURN_SUCCESS;
  }

  /// Allocate new ExtraGuidedSectionHandlerInfo and install in ConfigurationTable.
  HandlerInfoSize = (sizeof (EXTRACT_GUIDED_SECTION_HANDLER_INFO) +
                     (PcdGet32 (PcdMaximumGuidedExtractHandler) *
                      ((sizeof (GUID) +
                        sizeof (EXTRACT_GUIDED_SECTION_DECODE_HANDLER) +
                        sizeof (EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER)))
                     ));

  Buffer = AllocateZeroPool (HandlerInfoSize);
  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate HandlerInfo...\n", __func__));
    return RETURN_OUT_OF_RESOURCES;
  }

  mHandlerInfo = (EXTRACT_GUIDED_SECTION_HANDLER_INFO *)Buffer;

  Buffer                               += sizeof (EXTRACT_GUIDED_SECTION_HANDLER_INFO);
  mHandlerInfo->ExtractHandlerGuidTable = (GUID *)Buffer;

  Buffer                                 += (sizeof (GUID) * PcdGet32 (PcdMaximumGuidedExtractHandler));
  mHandlerInfo->ExtractDecodeHandlerTable = (EXTRACT_GUIDED_SECTION_DECODE_HANDLER *)Buffer;

  Buffer                                  += (sizeof (EXTRACT_GUIDED_SECTION_DECODE_HANDLER) * PcdGet32 (PcdMaximumGuidedExtractHandler));
  mHandlerInfo->ExtractGetInfoHandlerTable = (EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER *)Buffer;

  Status = MmSystemTable->MmInstallConfigurationTable (
                            MmSystemTable,
                            &gExtraGuidedSectionHandlerInfoGuid,
                            mHandlerInfo,
                            HandlerInfoSize
                            );
  if (RETURN_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to install ExtraGuidedSection Handler Info... (%r)\n",
      __func__,
      Status
      ));
    FreePool (mHandlerInfo);
    mHandlerInfo = NULL;
    return Status;
  }

  return RETURN_SUCCESS;
}
