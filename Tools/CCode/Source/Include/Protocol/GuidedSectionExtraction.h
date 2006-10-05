/** @file
  This file declares GUIDed section extraction protocol.

  This interface provides a means of decoding a GUID defined encapsulation 
  section. There may be multiple different GUIDs associated with the GUIDed
  section extraction protocol. That is, all instances of the GUIDed section
  extraction protocol must have the same interface structure.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  GuidedSectionExtraction.h

  @par Revision Reference:
  This protocol is defined in Firmware Volume Specification.
  Version 0.9

**/

#ifndef __GUIDED_SECTION_EXTRACTION_PROTOCOL_H__
#define __GUIDED_SECTION_EXTRACTION_PROTOCOL_H__


//
// Protocol GUID definition. Each GUIDed section extraction protocol has the
// same interface but with different GUID. All the GUIDs is defined here.
// May add multiple GUIDs here.
//
#define EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID \
  { \
    0xFC1BCDB0, 0x7D31, 0x49aa, {0x93, 0x6A, 0xA4, 0x60, 0x0D, 0x9D, 0xD0, 0x83 } \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL;

//
// Protocol member functions
//
/**
  Processes the input section and returns the data contained therein along 
  with the authentication status.

  @param  This                  Indicates the EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL instance.  
  @param  InputSection          Buffer containing the input GUIDed section to be processed.  
  @param  OutputBuffer          *OutputBuffer is allocated from boot services pool memory 
                                and contains the new section stream.  
  @param  OutputSize            A pointer to a caller-allocated UINTN in which the size 
                                of *OutputBuffer allocation is stored.   
  @param  AuthenticationStatus  A pointer to a caller-allocated UINT32 that 
                                indicates the authentication status of the output buffer.
                                
  @retval EFI_SUCCESS           The InputSection was successfully processed and the 
                                section contents were returned.
  @retval EFI_OUT_OF_RESOURCES  The system has insufficient resources to 
                                process the request.
  @retval EFI_INVALID_PARAMETER The GUID in InputSection does not match 
                                this instance of the GUIDed Section Extraction Protocol.

**/

typedef
EFI_STATUS
(EFIAPI *EFI_EXTRACT_GUIDED_SECTION) (
  IN  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL          *This,
  IN  VOID                                            *InputSection,
  OUT VOID                                            **OutputBuffer,
  OUT UINTN                                           *OutputSize,
  OUT UINT32                                          *AuthenticationStatus
  );

//
// Protocol definition
//
/**
  @par Protocol Description:
  If a GUID-defined section is encountered when doing section extraction, 
  the section extraction driver calls the appropriate instance of the GUIDed 
  Section Extraction Protocol to extract the section stream contained therein.

  @param ExtractSection
  Takes the GUIDed section as input and produces the section stream data. 

**/
struct _EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL {
  EFI_EXTRACT_GUIDED_SECTION  ExtractSection;
};

//
// may add other GUID here
//
extern EFI_GUID gEfiCrc32GuidedSectionExtractionProtocolGuid;

#endif
