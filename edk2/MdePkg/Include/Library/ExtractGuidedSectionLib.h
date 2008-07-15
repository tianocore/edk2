/** @file
  Extract Guided Section Library class defintions, 
  which can provide many handlers for the different guided section data.

  Copyright (c) 2007 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/
#ifndef __EXTRACT_GUIDED_SECTION_H__
#define __EXTRACT_GUIDED_SECTION_H__

/**
  Get information for the input guided section data.
  It will ASSERT () if the pointer to OutputBufferSize is NULL.
  It will ASSERT () if the pointer to ScratchBufferSize is NULL.
  It will ASSERT () if the pointer to SectionAttribute is NULL.

  @param[in]  InputSection          Buffer containing the input GUIDed section to be processed. 
  @param[out] OutputBufferSize      The size of OutputBuffer.
  @param[out] ScratchBufferSize     The size of ScratchBuffer.
  @param[out] SectionAttribute      The attribute of the input guided section.

  @retval  RETURN_SUCCESS           Get the required information successfully.
  @retval  RETURN_INVALID_PARAMETER The input data is not the valid guided section.

**/
typedef
RETURN_STATUS
(EFIAPI *EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER)(
  IN CONST  VOID    *InputSection,
  OUT       UINT32  *OutputBufferSize,
  OUT       UINT32  *ScratchBufferSize,
  OUT       UINT16  *SectionAttribute
  );

/**
  Extract data and Auth from the specific guided section.
  It will ASSERT () if the pointer to OutputBuffer is NULL.
  It will ASSERT () if the pointer to AuthenticationStatus is NULL.

  @param[in]  InputSection  Buffer containing the input GUIDed section to be processed. 
  @param[out] OutputBuffer  OutputBuffer directly points to the start of the section's contents,
                            if guided data is not prcessed. Otherwise,
                            OutputBuffer contains the output data, which is allocated by the caller.
  @param[out] ScratchBuffer A pointer to a caller-allocated buffer for function internal use. 
  @param[out] AuthenticationStatus 
                            A pointer to a caller-allocated UINT32 that indicates the
                            authentication status of the output buffer. 

  @retval  RETURN_SUCCESS               Get the output data and AuthenticationStatus successfully.
  @retval  RETURN_INVALID_PARAMETER     The input data is not the valid guided section.

**/
typedef
RETURN_STATUS
(EFIAPI *EXTRACT_GUIDED_SECTION_DECODE_HANDLER)(
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  IN        VOID    *ScratchBuffer,        OPTIONAL
  OUT       UINT32  *AuthenticationStatus
  );

/**
  Register Guided Section Extract and GetInfo Handler.

  @param[in]     SectionGuid    The guid matches this Extraction Handler.
  @param[in]     GetInfoHandler Handler to get information from guided section.
  @param[in]     DecodeHandler  Handler to extract guided section.

  @retval  RETURN_SUCCESS           Register Guided Section Extract Handler successfully.
  @retval  RETURN_OUT_OF_RESOURCES  Resource is not enough to register new Handler. 
  @retval  RETURN_INVALID_PARAMETER Pointer to Guid value is not valid.

**/
RETURN_STATUS
EFIAPI
ExtractGuidedSectionRegisterHandlers (
  IN CONST  GUID                                     *SectionGuid,
  IN        EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER  GetInfoHandler,
  IN        EXTRACT_GUIDED_SECTION_DECODE_HANDLER    DecodeHandler
  );

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
  );

/**
  Get information from the guided section. This function first gets the guid value
  from guided section header, then match this guid in the registered extract Handler list
  to its corresponding getinfo Handler. 
  If not found, RETURN_UNSUPPORTED will be return. 
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
  );

/**
  Extract data from the guided section. This function first gets the guid value
  from guided section header, then match this guid in the registered extract Handler list
  to its corresponding extract Handler. 
  If not found, RETURN_UNSUPPORTED will be return. 
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
  );

#endif
