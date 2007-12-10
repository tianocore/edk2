/**@file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SectionExtraction.c
  
Abstract:

  Section Extraction Protocol implementation.
  
  Stream database is implemented as a linked list of section streams,
  where each stream contains a linked list of children, which may be leaves or
  encapsulations.  
  
  Children that are encapsulations generate new stream entries
  when they are created.  Streams can also be created by calls to 
  SEP->OpenSectionStream().
  
  The database is only created far enough to return the requested data from
  any given stream, or to determine that the requested data is not found.
  
  If a GUIDed encapsulation is encountered, there are three possiblilites.
  
  1) A support protocol is found, in which the stream is simply processed with
     the support protocol.
     
  2) A support protocol is not found, but the data is available to be read
     without processing.  In this case, the database is built up through the
     recursions to return the data, and a RPN event is set that will enable
     the stream in question to be refreshed if and when the required section
     extraction protocol is published.This insures the AuthenticationStatus 
     does not become stale in the cache.
     
  3) A support protocol is not found, and the data is not available to be read
     without it.  This results in EFI_PROTOCOL_ERROR.
  
**/

#ifndef _SECION_EXTRACTION_H_
#define _SECION_EXTRACTION_H_

#include <FrameworkDxe.h>

#include <Protocol/SectionExtraction.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/Decompress.h>
#include <Protocol/GuidedSectionExtraction.h>

#endif  // _SECTION_EXTRACTION_H_
