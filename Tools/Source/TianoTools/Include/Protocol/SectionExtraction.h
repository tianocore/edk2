/** @file
  This file declares Section Extraction protocols.

  This interface provides a means of decoding a set of sections into a linked list of
  leaf sections.  This provides for an extensible and flexible file format.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SectionExtraction.h

  @par Revision Reference:
  This protocol is defined in Firmware Volume Specification.
  Version 0.9

**/

#ifndef __SECTION_EXTRACTION_PROTOCOL_H__
#define __SECTION_EXTRACTION_PROTOCOL_H__


//
// Protocol GUID definition
//
#define EFI_SECTION_EXTRACTION_PROTOCOL_GUID \
  { \
    0x448F5DA4, 0x6DD7, 0x4FE1, {0x93, 0x07, 0x69, 0x22, 0x41, 0x92, 0x21, 0x5D } \
  }

typedef struct _EFI_SECTION_EXTRACTION_PROTOCOL EFI_SECTION_EXTRACTION_PROTOCOL;

//
// Protocol member functions
//
/**
  Creates and returns a new section stream handle to represent the new section stream.

  @param  This                  Indicates the EFI_SECTION_EXTRACTION_PROTOCOL instance.  
  @param  SectionStreamLength   Size in bytes of the section stream.  
  @param  SectionStream         Buffer containing the new section stream.  
  @param  SectionStreamHandle   A pointer to a caller-allocated UINTN that, 
                                on output, contains the new section stream handle.

  @retval EFI_SUCCESS           The SectionStream  was successfully processed and 
                                the section stream handle was returned.  
  @retval EFI_OUT_OF_RESOURCES  The system has insufficient resources to 
                                process the request.  
  @retval EFI_INVALID_PARAMETER The section stream may be corrupt or the value 
                                of SectionStreamLength may be incorrect.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_OPEN_SECTION_STREAM) (
  IN  EFI_SECTION_EXTRACTION_PROTOCOL                   *This,
  IN  UINTN                                             SectionStreamLength,
  IN  VOID                                              *SectionStream,
  OUT UINTN                                             *SectionStreamHandle
  );

/**
  Reads and returns a single section from a section stream.

  @param  This                  Indicates the EFI_SECTION_EXTRACTION_PROTOCOL instance.                                
  @param  SectionStreamHandle   Indicates from which section stream to read.                                
  @param  SectionType           Pointer to an EFI_SECTION_TYPE.  
  @param  SectionDefinitionGuid Pointer to an EFI_GUID.If SectionType == 
                                EFI_SECTION_GUID_DEFINED, SectionDefinitionGuid indicates what section GUID 
                                to search for.If SectionType !=EFI_SECTION_GUID_DEFINED, then 
                                SectionDefinitionGuid is unused and is ignored.  
  @param  SectionInstance       Indicates which instance of the requested section 
                                type to return when SectionType is not NULL.  
  @param  SectionStreamHandle   A pointer to a caller-allocated UINTN that, on output,
                                contains the new section stream handle.  
  @param  Buffer                Pointer to a pointer to a buffer in which the section 
                                contents are returned.   
  @param  BufferSize            Pointer to a caller-allocated UINTN.  
  @param  AuthenticationStatus  Pointer to a caller-allocated UINT32 in 
                                which any meta-data from encapsulation GUID-defined sections is returned.

  @retval EFI_SUCCESS           The SectionStream was successfully processed and 
                                the section contents were returned in Buffer.  
  @retval EFI_PROTOCOL_ERROR    A GUID-defined section was encountered in 
                                the section stream with its EFI_GUIDED_SECTION_PROCESSING_REQUIRED bit set, 
                                but there was no corresponding GUIDed Section Extraction Protocol in 
                                the handle database.  
  @retval EFI_NOT_FOUND         An error was encountered when parsing the SectionStream, 
                                which indicates that the SectionStream is not correctly formatted.
                                Or The requested section does not exist.  
  @retval EFI_OUT_OF_RESOURCES  The system has insufficient resources to process 
                                the request.  
  @retval EFI_INVALID_PARAMETER The SectionStreamHandle does not exist.  
  @retval EFI_BUFFER_TOO_SMALL  The size of the input buffer is insufficient to 
                                contain the requested section.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_SECTION) (
  IN EFI_SECTION_EXTRACTION_PROTOCOL                    *This,
  IN UINTN                                              SectionStreamHandle,
  IN EFI_SECTION_TYPE                                   *SectionType,
  IN EFI_GUID                                           *SectionDefinitionGuid,
  IN UINTN                                              SectionInstance,
  IN VOID                                               **Buffer,
  IN OUT UINTN                                          *BufferSize,
  OUT UINT32                                            *AuthenticationStatus
  );

/**
  Deletes a section stream handle and returns all associated resources to the system.

  @param  This                  Indicates the EFI_SECTION_EXTRACTION_PROTOCOL instance.  
  @param  SectionStreamHandle   Indicates the section stream to close.
  
  @retval EFI_SUCCESS           The SectionStream was successfully processed and 
                                the section stream handle was returned.
  
  @retval EFI_INVALID_PARAMETER The SectionStreamHandle does not exist.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CLOSE_SECTION_STREAM) (
  IN EFI_SECTION_EXTRACTION_PROTOCOL                    *This,
  IN UINTN                                              SectionStreamHandle
  );

//
// Protocol definition
//
/**
  @par Protocol Description:
  The Section Extraction Protocol provides a simple method of extracting 
  sections from arbitrarily complex files.

  @param OpenSectionStream
  Takes a bounded stream of sections and returns a section stream handle. 

  @param GetSection
  Given a section stream handle, retrieves the requested section and 
  meta-data from the section stream. 

  @param CloseSectionStream
  Given a section stream handle, closes the section stream.

**/
struct _EFI_SECTION_EXTRACTION_PROTOCOL {
  EFI_OPEN_SECTION_STREAM   OpenSectionStream;
  EFI_GET_SECTION           GetSection;
  EFI_CLOSE_SECTION_STREAM  CloseSectionStream;
};

extern EFI_GUID gEfiSectionExtractionProtocolGuid;

#endif
