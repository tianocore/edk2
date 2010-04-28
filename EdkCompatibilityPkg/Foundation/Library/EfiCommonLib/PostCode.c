/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PostCode.c

Abstract:

  Worker functions for PostCode

--*/

#include "TianoCommon.h"
#include "EfiCommonLib.h"

BOOLEAN
CodeTypeToPostCode (
  IN  EFI_STATUS_CODE_TYPE    CodeType,
  IN  EFI_STATUS_CODE_VALUE   Value,
  OUT UINT8                   *PostCode
  )
/*++

Routine Description:

  Convert code value to an 8 bit post code

Arguments:

  CodeType  - Code type
  Value     - Code value
  PostCode  - Post code as output

Returns:

  TRUE    - Successfully converted
  
  FALSE   - Convertion failed

--*/
{
  //
  // Convert Value to an 8 bit post code
  //
  if (((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) ||
      ((CodeType & EFI_STATUS_CODE_TYPE_MASK)== EFI_ERROR_CODE)) {
    *PostCode = (UINT8) (((Value & EFI_STATUS_CODE_CLASS_MASK) >> 24) << 5);
    *PostCode = (UINT8)(*PostCode | (((Value & EFI_STATUS_CODE_SUBCLASS_MASK) >> 16) & 0x1f));
    return TRUE;
  }

  return FALSE;
}
