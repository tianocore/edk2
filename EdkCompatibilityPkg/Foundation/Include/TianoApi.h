/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TianoApi.h

Abstract:

  Tiano intrinsic definitions. 


--*/

#ifndef _TIANO_API_H_
#define _TIANO_API_H_

#include "EfiApi.h"
#include "TianoSpecApi.h"

//
// Pointer to internal runtime function
//
#define EFI_INTERNAL_FUNCTION 0x00000002

//
// Pointer to internal runtime pointer
//
#define EFI_INTERNAL_POINTER  0x00000004

//
// Pointer to internal runtime pointer
//
#define EFI_IPF_GP_POINTER  0x00000008

#define EFI_TPL_DRIVER      6

#endif
