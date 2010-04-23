/** @file
  This file declares the Section Extraction PPI.

  This PPI is defined in PEI CIS version 0.91. It supports encapsulating sections,
  such as GUIDed sections used to authenticate the file encapsulation of other domain-specific wrapping.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SECTION_EXTRACTION_H__
#define __SECTION_EXTRACTION_H__

#define EFI_PEI_SECTION_EXTRACTION_PPI_GUID \
  { \
    0x4F89E208, 0xE144, 0x4804, {0x9E, 0xC8, 0x0F, 0x89, 0x4F, 0x7E, 0x36, 0xD7 } \
  }

typedef struct _EFI_PEI_SECTION_EXTRACTION_PPI EFI_PEI_SECTION_EXTRACTION_PPI;

//
// Bit values for AuthenticationStatus
//
#define EFI_AUTH_STATUS_PLATFORM_OVERRIDE 0x01
#define EFI_AUTH_STATUS_IMAGE_SIGNED      0x02
#define EFI_AUTH_STATUS_NOT_TESTED        0x04
#define EFI_AUTH_STATUS_TEST_FAILED       0x08

/**
  The function is used to retrieve a section from within a section file.
  It will retrieve both encapsulation sections and leaf sections in their entirety,
  exclusive of the section header.

  @param PeiServices            The pointer to the PEI Services Table.
  @param This                   Indicates the calling context
  @param SectionType            The pointer to an EFI_SECTION_TYPE. If 
                                SectionType == NULL, the contents of the entire 
                                section are returned in Buffer. If SectionType
                                is not NULL, only the requested section is returned.
  @param SectionDefinitionGuid  The pointer to an EFI_GUID.
                                If SectionType == EFI_SECTION_GUID_DEFINED, 
                                SectionDefinitionGuid indicates for which section 
                                GUID to search.  If SectionType != EFI_SECTION_GUID_DEFINED, 
                                SectionDefinitionGuid is unused and is ignored.
  @param SectionInstance        If SectionType is not NULL, indicates which
                                instance of the requested section type to return.
  @param Buffer                 The pointer to a pointer to a buffer in which the 
                                section contents are returned.
  @param BufferSize             A pointer to a caller-allocated UINT32. On input, 
                                *BufferSize indicates the size in bytes of the 
                                memory region pointed to by Buffer. On output,
                                *BufferSize contains the number of bytes required 
                                to read the section.
  @param AuthenticationStatus   A pointer to a caller-allocated UINT32 in
                                which any metadata from encapsulating GUID-defined 
                                sections is returned.

  @retval EFI_SUCCESS           The section was successfully processed, and the section
                                contents were returned in Buffer.
  @retval EFI_PROTOCOL_ERROR    A GUID-defined section was encountered in
                                the file with its EFI_GUIDED_SECTION_PROCESSING_REQUIRED 
                                bit set, but there was no corresponding GUIDed 
                                Section Extraction Protocol in the handle database. 
                                *Buffer is unmodified.
  @retval EFI_NOT_FOUND         The requested section does not exist.*Buffer is 
                                unmodified.
  @retval EFI_OUT_OF_RESOURCES  The system has insufficient resources to process 
                                the request.
  @retval EFI_INVALID_PARAMETER The SectionStreamHandle does not exist.
  @retval EFI_WARN_TOO_SMALL    The size of the input buffer is insufficient to
                                contain the requested section. The input buffer 
                                is filled and contents are section contents are 
                                truncated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_SECTION)(
  IN EFI_PEI_SERVICES                         **PeiServices,
  IN EFI_PEI_SECTION_EXTRACTION_PPI           *This,
  IN EFI_SECTION_TYPE                         *SectionType,
  IN EFI_GUID                                 *SectionDefinitionGuid, OPTIONAL
  IN UINTN                                    SectionInstance,
  IN VOID                                     **Buffer,
  IN OUT UINT32                               *BufferSize,
  OUT UINT32                                  *AuthenticationStatus
  );

/**
  This PPI supports encapsulating sections, such as GUIDed sections used to
  authenticate the file encapsulation of other domain-specific wrapping.
**/
struct _EFI_PEI_SECTION_EXTRACTION_PPI {
  EFI_PEI_GET_SECTION GetSection;  ///< Retrieves a section from within a section file.
};

extern EFI_GUID gEfiPeiSectionExtractionPpiGuid;

#endif

