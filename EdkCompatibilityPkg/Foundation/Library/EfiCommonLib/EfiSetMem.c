/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiSetMem.c

Abstract:

  Implementation of the EfiSetMem routine. This function is broken
  out into its own source file so that it can be excluded from a
  build for a particular platform easily if an optimized version 
  is desired.

--*/

#include "Tiano.h"
#include "EfiCommonLib.h"


VOID
EfiCommonLibSetMem (
  IN VOID   *Buffer,
  IN UINTN  Size,
  IN UINT8  Value
  )
/*++

Routine Description:

  Set Buffer to Value for Size bytes.

Arguments:

  Buffer  - Memory to set.

  Size    - Number of bytes to set

  Value   - Value of the set operation.

Returns:

  None

--*/
{
  INT8  *Ptr;

  if (Value == 0) {
    EfiCommonLibZeroMem (Buffer, Size);
  } else {
    Ptr = Buffer;
    while (Size--) {
      *(Ptr++) = Value;
    }
  }
}
