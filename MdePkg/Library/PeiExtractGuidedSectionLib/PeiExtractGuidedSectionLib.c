/** @file
  Provide generic extract guided section functions.

  Copyright (c) 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/ExtractGuidedSectionLib.h>

#define PEI_EXTRACT_HANDLER_INFO_SIGNATURE EFI_SIGNATURE_32 ('P', 'E', 'H', 'I')

typedef struct {
  UINT32                                  Signature;
  UINT32                                  NumberOfExtractHandler;
  GUID                                    *ExtractHandlerGuidTable;
  EXTRACT_GUIDED_SECTION_DECODE_HANDLER   *ExtractDecodeHandlerTable;
  EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER *ExtractGetInfoHandlerTable;
} PEI_EXTRACT_GUIDED_SECTION_HANDLER_INFO;

/**
  Build guid hob for the global memory to store the registered guid and Handler list.
  If GuidHob exists, HandlerInfo will be directly got from Guid hob data.

  @param[in, out]  InfoPointer   Pointer to pei handler info structure.

  @retval  RETURN_SUCCESS            Build Guid hob for the global memory space to store guid and funciton tables.
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to allocated.
**/
RETURN_STATUS
EFIAPI
PeiGetExtractGuidedSectionHandlerInfo (
  IN OUT PEI_EXTRACT_GUIDED_SECTION_HANDLER_INFO **InfoPointer
  )
{
  PEI_EXTRACT_GUIDED_SECTION_HANDLER_INFO *HandlerInfo;
  EFI_PEI_HOB_POINTERS                    Hob;
  
  //
  // First try to get handler info from guid hob specified by CallerId.
  //
  Hob.Raw = GetNextHob (EFI_HOB_TYPE_GUID_EXTENSION, GetHobList ());
  while (Hob.Raw != NULL) {
    if (CompareGuid (&(Hob.Guid->Name), &gEfiCallerIdGuid)) {
      HandlerInfo = (PEI_EXTRACT_GUIDED_SECTION_HANDLER_INFO *) GET_GUID_HOB_DATA (Hob.Guid);
      if (HandlerInfo->Signature == PEI_EXTRACT_HANDLER_INFO_SIGNATURE) {
        //
        // Update Table Pointer when hob start address is changed.
        //
        if (HandlerInfo->ExtractHandlerGuidTable != (GUID *) (HandlerInfo + 1)) {
          HandlerInfo->ExtractHandlerGuidTable    = (GUID *) (HandlerInfo + 1);
          HandlerInfo->ExtractDecodeHandlerTable  = (EXTRACT_GUIDED_SECTION_DECODE_HANDLER *) (
                                                      (UINT8 *)HandlerInfo->ExtractHandlerGuidTable + 
                                                      PcdGet32 (PcdMaximumGuidedExtractHandler) * sizeof (GUID)
                                                     );
          HandlerInfo->ExtractGetInfoHandlerTable = (EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER *) (
                                                      (UINT8 *)HandlerInfo->ExtractDecodeHandlerTable + 
                                                      PcdGet32 (PcdMaximumGuidedExtractHandler) * 
                                                      sizeof (EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER)
                                                     );
        }
        //
        // Return HandlerInfo pointer.
        //
        *InfoPointer = HandlerInfo;
        return EFI_SUCCESS;
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_GUID_EXTENSION, Hob.Raw);
  }
  
  //
  // If Guid Hob is not found, Build CallerId Guid hob to store Handler Info
  //
  HandlerInfo = BuildGuidHob (
                 &gEfiCallerIdGuid, 
                 sizeof (PEI_EXTRACT_GUIDED_SECTION_HANDLER_INFO) +
                 PcdGet32 (PcdMaximumGuidedExtractHandler) * 
                 (sizeof (GUID) + sizeof (EXTRACT_GUIDED_SECTION_DECODE_HANDLER) + sizeof (EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER))
                );
  if (HandlerInfo == NULL) {
    //
    // No enough resource to build guid hob.
    //
    return EFI_OUT_OF_RESOURCES;
  }
  HandlerInfo->Signature = PEI_EXTRACT_HANDLER_INFO_SIGNATURE;
  HandlerInfo->NumberOfExtractHandler     = 0;
  HandlerInfo->ExtractHandlerGuidTable    = (GUID *) (HandlerInfo + 1);
  HandlerInfo->ExtractDecodeHandlerTable  = (EXTRACT_GUIDED_SECTION_DECODE_HANDLER *) (
                                              (UINT8 *)HandlerInfo->ExtractHandlerGuidTable + 
                                              PcdGet32 (PcdMaximumGuidedExtractHandler) * sizeof (GUID)
                                             );
  HandlerInfo->ExtractGetInfoHandlerTable = (EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER *) (
                                              (UINT8 *)HandlerInfo->ExtractDecodeHandlerTable + 
                                              PcdGet32 (PcdMaximumGuidedExtractHandler) * 
                                              sizeof (EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER)
                                             );
  
  *InfoPointer = HandlerInfo;
  return EFI_SUCCESS;
}

/**
  Get the supported exract guided section Handler guid list.
  If ExtractHandlerGuidTable = NULL, then ASSERT.

  @param[in, out]  ExtractHandlerGuidTable   The extract Handler guid pointer list.

  @retval  return the number of the supported extract guided Handler.
**/
UINTN
EFIAPI
ExtractGuidedSectionGetGuidList (
  IN OUT  GUID  **ExtractHandlerGuidTable
  )
{
  EFI_STATUS Status;
  PEI_EXTRACT_GUIDED_SECTION_HANDLER_INFO *HandlerInfo;

  ASSERT (ExtractHandlerGuidTable != NULL);

  Status = PeiGetExtractGuidedSectionHandlerInfo (&HandlerInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *ExtractHandlerGuidTable = HandlerInfo->ExtractHandlerGuidTable;
  return HandlerInfo->NumberOfExtractHandler;
}

/**
  Register Guided Section Extract and GetInfo handler.

  @param[in]     SectionGuid    The guid matches this Extraction function.
  @param[in]     GetInfoHandler Function to get info from guided section.
  @param[in]     DecodeHandler  Function to extract guided section.

  @retval  RETURN_SUCCESS           Register Guided Section Extract function successfully.
  @retval  RETURN_OUT_OF_RESOURCES  Resource is not enough to register new function. 
  @retval  RETURN_INVALID_PARAMETER Input pointer to Guid value is not valid.
**/
RETURN_STATUS
EFIAPI
ExtractGuidedSectionRegisterHandlers (
  IN CONST  GUID                                     *SectionGuid,
  IN        EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER  GetInfoHandler,
  IN        EXTRACT_GUIDED_SECTION_DECODE_HANDLER    DecodeHandler
  )
{
  EFI_STATUS Status;
  UINT32     Index;
  PEI_EXTRACT_GUIDED_SECTION_HANDLER_INFO *HandlerInfo;

  //
  // Check input paramter.
  //
  if (SectionGuid == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Get the registered handler information
  //
  Status = PeiGetExtractGuidedSectionHandlerInfo (&HandlerInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < HandlerInfo->NumberOfExtractHandler; Index ++) {
    if (CompareGuid (HandlerInfo->ExtractHandlerGuidTable + Index, SectionGuid)) {
      break;
    }
  }

  //
  // If the guided handler has been registered before, only update its handler.
  //
  if (Index < HandlerInfo->NumberOfExtractHandler) {
    HandlerInfo->ExtractDecodeHandlerTable [Index] = DecodeHandler;
    HandlerInfo->ExtractGetInfoHandlerTable [Index] = GetInfoHandler;
    return RETURN_SUCCESS;
  }

  //
  // Check the global table is enough to contain new Handler.
  //
  if (HandlerInfo->NumberOfExtractHandler >= PcdGet32 (PcdMaximumGuidedExtractHandler)) {
    return RETURN_OUT_OF_RESOURCES;
  }
  
  //
  // Register new Handler and guid value.
  //
  CopyGuid (HandlerInfo->ExtractHandlerGuidTable + HandlerInfo->NumberOfExtractHandler, SectionGuid);
  HandlerInfo->ExtractDecodeHandlerTable [HandlerInfo->NumberOfExtractHandler] = DecodeHandler;
  HandlerInfo->ExtractGetInfoHandlerTable [HandlerInfo->NumberOfExtractHandler++] = GetInfoHandler;
  
  return RETURN_SUCCESS;
}

/**
  Get information from the guided section. This function first gets the guid value
  from guided section header, then match this guid in the registered extract Handler list
  to its corresponding getinfo Handler. 
  If not found, RETURN_INVALID_PARAMETER will be return. 
  If found, it will call the getinfo Handler to get the required size and attribute.

  It will ASSERT () if the pointer to OutputBufferSize is NULL.
  It will ASSERT () if the pointer to ScratchBufferSize is NULL.
  It will ASSERT () if the pointer to SectionAttribute is NULL.

  @param[in]  InputSection          Buffer containing the input GUIDed section to be processed. 
  @param[out] OutputBufferSize      The size of OutputBuffer.
  @param[out] ScratchBufferSize     The size of ScratchBuffer.  
  @param[out] SectionAttribute      The attribute of the input guided section.

  @retval  RETURN_SUCCESS           Get the required information successfully.
  @retval  RETURN_INVALID_PARAMETER The input data can't be parsed correctly. 
                                    The GUID in InputSection does not match any registered guid list.

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
  UINT32 Index;
  EFI_STATUS Status;
  PEI_EXTRACT_GUIDED_SECTION_HANDLER_INFO *HandlerInfo;
  
  //
  // Check input paramter
  //
  if (InputSection == NULL) {
    return RETURN_INVALID_PARAMETER;
  }
  
  ASSERT (OutputBufferSize != NULL);
  ASSERT (ScratchBufferSize != NULL);
  ASSERT (SectionAttribute != NULL);

  //
  // Get the registered handler information.
  //
  Status = PeiGetExtractGuidedSectionHandlerInfo (&HandlerInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < HandlerInfo->NumberOfExtractHandler; Index ++) {
    if (CompareGuid (HandlerInfo->ExtractHandlerGuidTable + Index, &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
      break;
    }
  }

  //
  // Not found, the input guided section is not supported. 
  //
  if (Index == HandlerInfo->NumberOfExtractHandler) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Call the match handler to getinfo for the input section data.
  //
  return HandlerInfo->ExtractGetInfoHandlerTable [Index] (
            InputSection,
            OutputBufferSize,
            ScratchBufferSize,
            SectionAttribute
          );
}

/**
  Extract data from the guided section. This function first gets the guid value
  from guided section header, then match this guid in the registered extract Handler list
  to its corresponding extract Handler. 
  If not found, RETURN_INVALID_PARAMETER will be return. 
  If found, it will call this extract Handler to get output data and AuthenticationStatus.

  It will ASSERT () if the pointer to OutputBuffer is NULL.
  It will ASSERT () if the pointer to AuthenticationStatus is NULL.

  @param[in]  InputSection  Buffer containing the input GUIDed section to be processed. 
  @param[out] OutputBuffer  OutputBuffer to point the start of the section's contents 
                            if guided data is not required prcessing. Otherwise,
                            OutputBuffer to contain the output data, which is 
                            allocated by the caller.
  @param[out] ScratchBuffer A pointer to a caller-allocated buffer for function internal use. 
  @param[out] AuthenticationStatus 
                            A pointer to a caller-allocated UINT32 that indicates the
                            authentication status of the output buffer. 

  @retval  RETURN_SUCCESS           Get the output data, size and AuthenticationStatus successfully.
  @retval  RETURN_INVALID_PARAMETER The input data can't be parsed correctly. 
                                    The GUID in InputSection does not match any registered guid list.

**/
RETURN_STATUS
EFIAPI
ExtractGuidedSectionDecode (
  IN  CONST VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  OUT       VOID    *ScratchBuffer,        OPTIONAL
  OUT       UINT32  *AuthenticationStatus  
  )
{
  UINT32     Index;
  EFI_STATUS Status;
  PEI_EXTRACT_GUIDED_SECTION_HANDLER_INFO *HandlerInfo;
  
  if (InputSection == NULL) {
    return RETURN_INVALID_PARAMETER;
  }
  
  ASSERT (OutputBuffer != NULL);
  ASSERT (AuthenticationStatus != NULL);
  
  Status = PeiGetExtractGuidedSectionHandlerInfo (&HandlerInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < HandlerInfo->NumberOfExtractHandler; Index ++) {
    if (CompareGuid (HandlerInfo->ExtractHandlerGuidTable + Index, &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
      break;
    }
  }

  //
  // Not found, the input guided section is not supported. 
  //
  if (Index == HandlerInfo->NumberOfExtractHandler) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Call the match handler to getinfo for the input section data.
  //
  return HandlerInfo->ExtractDecodeHandlerTable [Index] (
            InputSection,
            OutputBuffer,
            ScratchBuffer,
            AuthenticationStatus
          );
}
