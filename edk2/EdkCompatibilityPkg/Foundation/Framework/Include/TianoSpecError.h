/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TianoSpecError.h

Abstract:

  Tiano error codes defined in Tiano spec.

--*/

#ifndef _TIANO_SPEC_ERROR_H_
#define _TIANO_SPEC_ERROR_H_

#include <EfiBind.h>
#define TIANO_ERROR(a)          (MAX_2_BITS | (a))

//
// Tiano added a couple of return types. These are owned by UEFI specification
// and Tiano can not use them. Thus for UEFI 2.0/R8.6 support we moved the values
// to a UEFI OEM extension range to conform to UEFI specification.
//
#if (EFI_SPECIFICATION_VERSION < 0x00020000)
  #define EFI_NOT_AVAILABLE_YET   EFIERR (28)
  #define EFI_UNLOAD_IMAGE        EFIERR (29)
#else
  #define EFI_NOT_AVAILABLE_YET   TIANO_ERROR (0)
  #define EFI_UNLOAD_IMAGE        TIANO_ERROR (1)
#endif

#endif
