/** @file
  Provide generic extract guided section functions for Dxe phase.

  Copyright (c) 2007 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ExtractGuidedSectionLib.h>

STATIC GUID                 *mExtractHandlerGuidTable;
STATIC UINT32               mNumberOfExtractHandler = 0;

STATIC EXTRACT_GUIDED_SECTION_DECODE_HANDLER   *mExtractDecodeHandlerTable;
STATIC EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER *mExtractGetInfoHandlerTable;

/**
  Construtor allocates the global memory to store the registered guid and Handler list.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval  RETURN_SUCCESS            Allocate the global memory space to store guid and funciton tables.
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to allocated.
**/
RETURN_STATUS
EFIAPI
DxeExtractGuidedSectionLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Allocate global pool space to store the registered handler and its guid value.
  //
  mExtractHandlerGuidTable    = (GUID *) AllocatePool (PcdGet32 (PcdMaximumGuidedExtractHandler) * sizeof (GUID));
  if (mExtractHandlerGuidTable == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  
  mExtractDecodeHandlerTable  = (EXTRACT_GUIDED_SECTION_DECODE_HANDLER *) AllocatePool (PcdGet32 (PcdMaximumGuidedExtractHandler) * sizeof (EXTRACT_GUIDED_SECTION_DECODE_HANDLER));
  if (mExtractDecodeHandlerTable == NULL) {
    FreePool (mExtractHandlerGuidTable);
    return RETURN_OUT_OF_RESOURCES;
  }

  mExtractGetInfoHandlerTable = (EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER *) AllocatePool (PcdGet32 (PcdMaximumGuidedExtractHandler) * sizeof (EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER));
  if (mExtractGetInfoHandlerTable == NULL) {
    FreePool (mExtractHandlerGuidTable);
    FreePool (mExtractDecodeHandlerTable);
    return RETURN_OUT_OF_RESOURCES;
  }
  
  return RETURN_SUCCESS;
}

/**
  Get the supported exract guided section Handler guid table, which is maintained
  by library. The caller can directly get the guid table 
  without responsibility to allocate or free this table buffer.  
  It will ASSERT () if ExtractHandlerGuidTable = NULL.

  @param[out]  ExtractHandlerGuidTable   The extract Handler guid pointer list.

  @return the number of the supported extract guided Handler.
**/
UINTN
EFIAPI
ExtractGuidedSectionGetGuidList (
  OUT  GUID  **ExtractHandlerGuidTable
  )
{
  ASSERT (ExtractHandlerGuidTable != NULL);

  *ExtractHandlerGuidTable = mExtractHandlerGuidTable;
  return mNumberOfExtractHandler;
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
  UINT32 Index;
  //
  // Check input paramter.
  //
  if (SectionGuid == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < mNumberOfExtractHandler; Index ++) {
    if (CompareGuid (&mExtractHandlerGuidTable[Index], SectionGuid)) {
      //
      // If the guided handler has been registered before, only update its handler.
      //
      mExtractDecodeHandlerTable [Index] = DecodeHandler;
      mExtractGetInfoHandlerTable [Index] = GetInfoHandler;
      return RETURN_SUCCESS;
    }
  }
  
  //
  // Check the global table is enough to contain new Handler.
  //
  if (mNumberOfExtractHandler >= PcdGet32 (PcdMaximumGuidedExtractHandler)) {
    return RETURN_OUT_OF_RESOURCES;
  }
  
  //
  // Register new Handler and guid value.
  //
  CopyGuid (&mExtractHandlerGuidTable [mNumberOfExtractHandler], SectionGuid);
  mExtractDecodeHandlerTable [mNumberOfExtractHandler] = DecodeHandler;
  mExtractGetInfoHandlerTable [mNumberOfExtractHandler++] = GetInfoHandler;
  
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
  @retval  RETURN_UNSUPPORTED       Guided section data is not supported.
  @retval  RETURN_INVALID_PARAMETER The input data is not the valid guided section.

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
  
  if (InputSection == NULL) {
    return RETURN_INVALID_PARAMETER;
  }
  
  ASSERT (OutputBufferSize != NULL);
  ASSERT (ScratchBufferSize != NULL);
  ASSERT (SectionAttribute != NULL);
 
  //
  // Search the match registered GetInfo handler for the input guided section.
  //
  for (Index = 0; Index < mNumberOfExtractHandler; Index ++) {
    if (CompareGuid (&mExtractHandlerGuidTable[Index], &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
      //
      // Call the match handler to getinfo for the input section data.
      //
      return mExtractGetInfoHandlerTable [Index] (
                InputSection,
                OutputBufferSize,
                ScratchBufferSize,
                SectionAttribute
              );
    }
  }

  //
  // Not found, the input guided section is not supported. 
  //
  return RETURN_UNSUPPORTED;
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

  @retval  RETURN_SUCCESS           Get the output data and AuthenticationStatus successfully.
  @retval  RETURN_UNSUPPORTED       Guided section data is not supported to be decoded.
  @retval  RETURN_INVALID_PARAMETER The input data is not the valid guided section.

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
  UINT32 Index;
  
  //
  // Check the input parameters
  //
  if (InputSection == NULL) {
    return RETURN_INVALID_PARAMETER;
  }
  
  ASSERT (OutputBuffer != NULL);
  ASSERT (AuthenticationStatus != NULL);

  //
  // Search the match registered extract handler for the input guided section.
  //
  for (Index = 0; Index < mNumberOfExtractHandler; Index ++) {
    if (CompareGuid (&mExtractHandlerGuidTable[Index], &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
      //
      // Call the match handler to extract raw data for the input section data.
      //
      return mExtractDecodeHandlerTable [Index] (
                InputSection,
                OutputBuffer,
                ScratchBuffer,
                AuthenticationStatus
              );
    }
  }

  //
  // Not found, the input guided section is not supported. 
  //
  return RETURN_UNSUPPORTED;
}
