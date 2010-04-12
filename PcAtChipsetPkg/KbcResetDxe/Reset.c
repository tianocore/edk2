/** @file
  Reset Architectural Protocol implementation

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "Reset.h"

/**
  Reset the system.

  @param ResetType       warm or cold
  @param ResetStatus     possible cause of reset
  @param DataSize        Size of ResetData in bytes
  @param ResetData       Optional Unicode string

**/
VOID
EFIAPI
KbcResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN VOID             *ResetData OPTIONAL
  )
{
  UINT8   Data;

  switch (ResetType) {
  case EfiResetWarm:
  case EfiResetCold:
  case EfiResetShutdown:
    Data = 0xfe;
    IoWrite8 (0x64, Data);
    break;

  default:
    return ;
  }

  //
  // Given we should have reset getting here would be bad
  //
  ASSERT (FALSE);
}

