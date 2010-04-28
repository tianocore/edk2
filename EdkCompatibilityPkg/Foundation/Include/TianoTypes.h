/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TianoTypes.h

Abstract:

  Tiano specific part besides EfiTypes.h

--*/

#ifndef _TIANO_TYPES_H_
#define _TIANO_TYPES_H_

#include "EfiTypes.h"
#include "TianoSpecTypes.h"

//
// attributes for reserved memory before it is promoted to system memory
//
#define EFI_MEMORY_PRESENT      0x0100000000000000
#define EFI_MEMORY_INITIALIZED  0x0200000000000000
#define EFI_MEMORY_TESTED       0x0400000000000000

//
// range for memory mapped port I/O on IPF
//
#define EFI_MEMORY_PORT_IO  0x4000000000000000

//
// A pointer to a function in IPF points to a plabel.
//
typedef struct {
  UINT64  EntryPoint;
  UINT64  GP;
} EFI_PLABEL;

#endif
