/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  RtMemoryStatusCode.h
   
Abstract:

  EFI library to provide status code reporting via a memory journal.

--*/

#ifndef _EFI_RT_MEMORY_STATUS_CODE_H_
#define _EFI_RT_MEMORY_STATUS_CODE_H_

//
// Statements that include other files
//
#include "Tiano.h"
#include "Pei.h"
#include "TianoCommon.h"
#include "EfiRuntimeLib.h"
#include "EfiHobLib.h"
#include "RtPlatformStatusCodeLib.h"

//
// Referenced protocols
//
#include EFI_ARCH_PROTOCOL_DEFINITION (StatusCode)


//
// Consumed protocols
//
#include EFI_PPI_CONSUMER (StatusCodeMemory)

//
// Consumed GUID
//
#include EFI_GUID_DEFINITION (Hob)

#endif
