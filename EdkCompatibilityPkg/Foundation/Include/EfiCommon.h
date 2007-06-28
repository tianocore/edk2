/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  EfiCommon.h

Abstract:

--*/

#ifndef _EFI_COMMON_H_
#define _EFI_COMMON_H_

#include "EfiBind.h"
#include "EfiTypes.h"
#include "EfiStdArg.h"
#include "EfiError.h"

//
// Define macros for including Protocols and Guids.
//
#define EFI_STRINGIZE(a)            #a
#define EFI_PROTOCOL_DEFINITION(a)  EFI_STRINGIZE (Protocol/a/a.h)
#define EFI_GUID_DEFINITION(a)      EFI_STRINGIZE (Guid/a/a.h)

//
// These should be used to include protocols.  If they are followed,
// intelligent build tools can be created to check dependencies at build
// time.
//
#define EFI_PROTOCOL_PRODUCER(a)    EFI_PROTOCOL_DEFINITION (a)
#define EFI_PROTOCOL_CONSUMER(a)    EFI_PROTOCOL_DEFINITION (a)
#define EFI_PROTOCOL_DEPENDENCY(a)  EFI_PROTOCOL_DEFINITION (a)

//
// Mechanism to associate a short and long ascii string with a GUID.
// For normal builds the strings are not included. A build utility
// can be constructed to extract the strings and build a table. It may
// be possible to add a build opption to automatically generate a GUID
// string table for a GUID to string utility build.
//
#define EFI_GUID_STRING(guidpointer, shortstring, longstring)

#endif
