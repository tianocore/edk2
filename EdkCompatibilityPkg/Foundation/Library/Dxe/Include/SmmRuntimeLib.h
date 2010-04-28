/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SmmRuntimeLib.h

Abstract:

  SMM Related prototypes that can be referenced for Preboot Configuration only.

--*/

#ifndef _SMM_RUNTIME_LIB_H_
#define _SMM_RUNTIME_LIB_H_

#include "Tiano.h"
#include "EfiRuntimeLib.h"

BOOLEAN
EfiInSmm (
  VOID
  )
/*++

Routine Description:

  Test whether in Smm mode currently.

Arguments:

  None

Returns:

  TRUE      - In Smm mode
  FALSE     - Not in Smm mode

--*/
;

EFI_STATUS
RegisterSmmRuntimeDriver (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable,
  OUT EFI_HANDLE            *SmmImageHandle
  )
/*++

Routine Description:

  Registers a Driver with the SMM.

Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.  
  SystemTable     - A pointer to the EFI System Table.
  SmmImageHandle  - Image handle returned by the SMM driver.

Returns:

  Status code

--*/
;

#endif
