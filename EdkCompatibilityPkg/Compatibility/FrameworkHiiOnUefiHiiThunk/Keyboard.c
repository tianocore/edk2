/** @file

  This file contains the keyboard processing code to the HII database.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"

/**
  Retrieves the current keyboard layout. 
  This function is not implemented by HII Thunk Module.

  @param This             A pointer to the EFI_HII_PROTOCOL instance. 
  @param DescriptorCount  A pointer to the number of Descriptor entries being described in the keyboard layout being retrieved.
  @param Descriptor       A pointer to a buffer containing an array of EFI_KEY_DESCRIPTOR entries. Each entry will reflect the 
                          definition of a specific physical key. Type EFI_KEY_DESCRIPTOR is defined in "Related Definitions" below.

  @retval  EFI_SUCCESS   The keyboard layout was retrieved successfully.
 
**/
EFI_STATUS
EFIAPI
HiiGetKeyboardLayout (
  IN     EFI_HII_PROTOCOL   *This,
  OUT    UINT16             *DescriptorCount,
  OUT    FRAMEWORK_EFI_KEY_DESCRIPTOR *Descriptor
  )
{
  ASSERT (FALSE);
  //
  // In previous Framewok HII implementation, GetKeyBoardLayout is defined in HII 0.92 specification,
  // but it is not implemented. We ASSERT and return UNSUPPORTED here. 
  //
  return EFI_UNSUPPORTED;
}
