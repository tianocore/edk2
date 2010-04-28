/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiCompareMem.c

Abstract:

  Generic compare-memory routine. 

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"


INTN
EfiCompareMem (
  IN VOID     *MemOne,
  IN VOID     *MemTwo,
  IN UINTN    Length
  )
/*++

Routine Description:

  Compares two memory buffers of a given length.

Arguments:

  MemOne - First memory buffer

  MemTwo - Second memory buffer

  Len    - Length of Mem1 and Mem2 memory regions to compare

Returns:

  = 0     if MemOne == MemTwo

--*/
{
  INTN ReturnValue;

  if (!(EFI_UINTN_ALIGNED (MemOne) || EFI_UINTN_ALIGNED (MemTwo) || EFI_UINTN_ALIGNED (Length))) {
    //
    // If Destination/Source/Length are aligned do UINTN conpare
    //
    for (; Length > 0; Length -= sizeof (INTN), MemOne = (VOID *)((UINTN)MemOne + sizeof (INTN)), MemTwo = (VOID *)((UINTN)MemTwo + sizeof (INTN))) {
      if (*(INTN *)MemOne != *(INTN *)MemTwo) {
        break;
      }
    }
  }

  //
  // If Destination/Source/Length not aligned do byte compare
  //
  for (; Length > 0; Length--, MemOne = (VOID *)((UINTN)MemOne + 1), MemTwo = (VOID *)((UINTN)MemTwo + 1)) {
    ReturnValue = (INTN)(*(INT8 *)MemOne - *(INT8 *)MemTwo);
    if (ReturnValue != 0) {
      return ReturnValue;
    }
  }

  return 0;
}
