/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GuidedSectionExtraction.h

Abstract:

  PI 1.0 spec definition.

--*/


#ifndef __GUIDED_SECTION_EXTRACTION_PPI_H__
#define __GUIDED_SECTION_EXTRACTION_PPI_H__

EFI_FORWARD_DECLARATION (EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI);

typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_EXTRACT_GUIDED_SECTION)(
  IN CONST EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI  *This,
  IN CONST VOID                                   *InputSection,
  OUT VOID                                        **OutputBuffer,
  OUT UINTN                                       *OutputSize,
  OUT UINT32                                      *AuthenticationStatus
  );

struct _EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI {
  EFI_PEI_EXTRACT_GUIDED_SECTION  ExtractSection;
};

#endif
