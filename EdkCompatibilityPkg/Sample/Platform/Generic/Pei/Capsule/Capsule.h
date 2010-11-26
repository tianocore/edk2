/*++        

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Capsule.h

Abstract:
  
--*/

#ifndef _CAPSULE_PEIM_H
#define _CAPSULE_PEIM_H

#include "Tiano.h"

//
// If capsule data is passed via a variable, then this name should be used.
//
#define EFI_CAPSULE_VARIABLE_NAME L"CapsuleUpdateData"

//
// The variable describes the long mode buffer used by IA32 Capsule PEIM
// to call X64 CapsuleCoalesce code to handle >4GB capsule blocks.
//
#define EFI_CAPSULE_LONG_MODE_BUFFER_NAME L"CapsuleLongModeBuffer"
typedef struct {
  UINT64   Base;
  UINT64   Length;
} EFI_CAPSULE_LONG_MODE_BUFFER;

#endif
