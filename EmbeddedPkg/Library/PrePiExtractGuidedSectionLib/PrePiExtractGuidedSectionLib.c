/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/PcdLib.h>
#include <Library/PrePiLib.h>

#define PRE_PI_EXTRACT_GUIDED_SECTION_DATA_GUID { 0x385A982C, 0x2F49, 0x4043, { 0xA5, 0x1E, 0x49, 0x01, 0x02, 0x5C, 0x8B, 0x6B }}

typedef struct {
  UINT32                                  NumberOfExtractHandler;
  GUID                                    *ExtractHandlerGuidTable;
  EXTRACT_GUIDED_SECTION_DECODE_HANDLER   *ExtractDecodeHandlerTable;
  EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER *ExtractGetInfoHandlerTable;
} PRE_PI_EXTRACT_GUIDED_SECTION_DATA;

PRE_PI_EXTRACT_GUIDED_SECTION_DATA *
GetSavedData (
  VOID
  )
{
  EFI_HOB_GUID_TYPE *GuidHob;
  GUID              SavedDataGuid = PRE_PI_EXTRACT_GUIDED_SECTION_DATA_GUID;

  GuidHob = GetFirstGuidHob(&SavedDataGuid);
  GuidHob++;

  return (PRE_PI_EXTRACT_GUIDED_SECTION_DATA *)GuidHob;
}

RETURN_STATUS
EFIAPI
ExtractGuidedSectionRegisterHandlers (
  IN CONST  GUID                                     *SectionGuid,
  IN        EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER  GetInfoHandler,
  IN        EXTRACT_GUIDED_SECTION_DECODE_HANDLER    DecodeHandler
  )
{
  PRE_PI_EXTRACT_GUIDED_SECTION_DATA  *SavedData;
  UINT32                              Index;
  //
  // Check input parameter.
  //
  if (SectionGuid == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  SavedData = GetSavedData();

  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < SavedData->NumberOfExtractHandler; Index ++) {
    if (CompareGuid (&SavedData->ExtractHandlerGuidTable[Index], SectionGuid)) {
      break;
    }
  }

  //
  // If the guided handler has been registered before, only update its handler.
  //
  if (Index < SavedData->NumberOfExtractHandler) {
    SavedData->ExtractDecodeHandlerTable [Index] = DecodeHandler;
    SavedData->ExtractGetInfoHandlerTable [Index] = GetInfoHandler;
    return RETURN_SUCCESS;
  }

  //
  // Check the global table is enough to contain new Handler.
  //
  if (SavedData->NumberOfExtractHandler >= PcdGet32 (PcdMaximumGuidedExtractHandler)) {
    return RETURN_OUT_OF_RESOURCES;
  }

  //
  // Register new Handler and guid value.
  //
  CopyGuid (&SavedData->ExtractHandlerGuidTable [SavedData->NumberOfExtractHandler], SectionGuid);
  SavedData->ExtractDecodeHandlerTable [SavedData->NumberOfExtractHandler] = DecodeHandler;
  SavedData->ExtractGetInfoHandlerTable [SavedData->NumberOfExtractHandler++] = GetInfoHandler;

  return RETURN_SUCCESS;
}

UINTN
EFIAPI
ExtractGuidedSectionGetGuidList (
  IN OUT  GUID  **ExtractHandlerGuidTable
  )
{
  PRE_PI_EXTRACT_GUIDED_SECTION_DATA  *SavedData;

  ASSERT(ExtractHandlerGuidTable != NULL);

  SavedData = GetSavedData();

  *ExtractHandlerGuidTable = SavedData->ExtractHandlerGuidTable;
  return SavedData->NumberOfExtractHandler;
}

RETURN_STATUS
EFIAPI
ExtractGuidedSectionGetInfo (
  IN  CONST VOID    *InputSection,
  OUT       UINT32  *OutputBufferSize,
  OUT       UINT32  *ScratchBufferSize,
  OUT       UINT16  *SectionAttribute
  )
{
  PRE_PI_EXTRACT_GUIDED_SECTION_DATA  *SavedData;
  UINT32                              Index;
  EFI_GUID                            *SectionDefinitionGuid;

  if (InputSection == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  ASSERT (OutputBufferSize != NULL);
  ASSERT (ScratchBufferSize != NULL);
  ASSERT (SectionAttribute != NULL);

  SavedData = GetSavedData();

  if (IS_SECTION2 (InputSection)) {
    SectionDefinitionGuid = &(((EFI_GUID_DEFINED_SECTION2 *) InputSection)->SectionDefinitionGuid);
  } else {
    SectionDefinitionGuid = &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid);
  }

  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < SavedData->NumberOfExtractHandler; Index ++) {
    if (CompareGuid (&SavedData->ExtractHandlerGuidTable[Index], SectionDefinitionGuid)) {
      break;
    }
  }

  //
  // Not found, the input guided section is not supported.
  //
  if (Index == SavedData->NumberOfExtractHandler) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Call the match handler to getinfo for the input section data.
  //
  return SavedData->ExtractGetInfoHandlerTable [Index] (
            InputSection,
            OutputBufferSize,
            ScratchBufferSize,
            SectionAttribute
          );
}

RETURN_STATUS
EFIAPI
ExtractGuidedSectionDecode (
  IN  CONST VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  OUT       VOID    *ScratchBuffer,        OPTIONAL
  OUT       UINT32  *AuthenticationStatus
  )
{
  PRE_PI_EXTRACT_GUIDED_SECTION_DATA  *SavedData;
  UINT32                              Index;
  EFI_GUID                            *SectionDefinitionGuid;

  if (InputSection == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  ASSERT (OutputBuffer != NULL);
  ASSERT (AuthenticationStatus != NULL);

  SavedData = GetSavedData();

  if (IS_SECTION2 (InputSection)) {
    SectionDefinitionGuid = &(((EFI_GUID_DEFINED_SECTION2 *) InputSection)->SectionDefinitionGuid);
  } else {
    SectionDefinitionGuid = &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid);
  }

  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < SavedData->NumberOfExtractHandler; Index ++) {
    if (CompareGuid (&SavedData->ExtractHandlerGuidTable[Index], SectionDefinitionGuid)) {
      break;
    }
  }

  //
  // Not found, the input guided section is not supported.
  //
  if (Index == SavedData->NumberOfExtractHandler) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Call the match handler to getinfo for the input section data.
  //
  return SavedData->ExtractDecodeHandlerTable [Index] (
            InputSection,
            OutputBuffer,
            ScratchBuffer,
            AuthenticationStatus
          );
}

RETURN_STATUS
EFIAPI
ExtractGuidedSectionLibConstructor (
  VOID
  )
{
  PRE_PI_EXTRACT_GUIDED_SECTION_DATA  SavedData;
  GUID                                HobGuid = PRE_PI_EXTRACT_GUIDED_SECTION_DATA_GUID;

  //
  // Allocate global pool space to store the registered handler and its guid value.
  //
  SavedData.ExtractHandlerGuidTable = (GUID *)AllocatePool(PcdGet32(PcdMaximumGuidedExtractHandler) * sizeof(GUID));
  if (SavedData.ExtractHandlerGuidTable == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  SavedData.ExtractDecodeHandlerTable  = (EXTRACT_GUIDED_SECTION_DECODE_HANDLER *)AllocatePool(PcdGet32(PcdMaximumGuidedExtractHandler) * sizeof(EXTRACT_GUIDED_SECTION_DECODE_HANDLER));
  if (SavedData.ExtractDecodeHandlerTable == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  SavedData.ExtractGetInfoHandlerTable = (EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER *)AllocatePool(PcdGet32(PcdMaximumGuidedExtractHandler) * sizeof(EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER));
  if (SavedData.ExtractGetInfoHandlerTable == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  //
  // the initialized number is Zero.
  //
  SavedData.NumberOfExtractHandler = 0;

  BuildGuidDataHob(&HobGuid, &SavedData, sizeof(SavedData));

  return RETURN_SUCCESS;
}
