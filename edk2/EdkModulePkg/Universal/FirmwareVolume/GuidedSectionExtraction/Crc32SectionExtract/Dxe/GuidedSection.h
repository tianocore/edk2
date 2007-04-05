/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GuidedSection.h
  
Abstract:

  Header file for GuidedSection.c
  Please refer to the Framewokr Firmware Volume Specification 0.9.
  
--*/

#ifndef _GUIDED_SECTION_EXTRACTION_H
#define _GUIDED_SECTION_EXTRACTION_H

//
// Function prototype declarations
//
EFI_STATUS
GuidedSectionExtractionProtocolConstructor (
  OUT EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL      **GuidedSep,
  IN  EFI_EXTRACT_GUIDED_SECTION                  ExtractSection
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  GuidedSep       - TODO: add argument description
  ExtractSection  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
