/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiCompareGuid.c

Abstract:

  Driver library routine to compare two GUIDs.

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"

BOOLEAN
EfiCompareGuid (
  IN EFI_GUID *Guid1,
  IN EFI_GUID *Guid2
  )
/*++

Routine Description:

  Compares two GUIDs

Arguments:

  Guid1 - guid to compare

  Guid2 - guid to compare

Returns:
  TRUE     if Guid1 == Guid2
  FALSE    if Guid1 != Guid2

--*/
{
  UINTN Index;

  //
  // compare byte by byte
  //
  for (Index = 0; Index < 16; ++Index) {
    if (*(((UINT8*) Guid1) + Index) != *(((UINT8*) Guid2) + Index)) {
      return FALSE;
    }
  }
  return TRUE;
}
