/**@file

  This file contains the keyboard processing code to the HII database.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"

EFI_STATUS
EFIAPI
HiiGetKeyboardLayout (
  IN     EFI_HII_PROTOCOL   *This,
  OUT    UINT16             *DescriptorCount,
  OUT    FRAMEWORK_EFI_KEY_DESCRIPTOR *Descriptor
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  ASSERT (FALSE);
  //
  // In previous Framewok HII implementation, GetKeyBoardLayout is defined in HII 0.92 specification,
  // but it is not implemented. We ASSERT and return UNSUPPORTED here. 
  //
  return EFI_UNSUPPORTED;
}
