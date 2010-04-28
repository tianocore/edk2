/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiZeroMemRep1.c

Abstract:

  This is the code that uses rep stosb ZeroMem service

--*/

#include "Tiano.h"

VOID
EfiCommonLibZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Count
  )
/*++

Input:  VOID   *Buffer - Pointer to buffer to clear
        UINTN  Count  - Number of bytes to clear

Output: None.

Saves:

Modifies:

Description:  This function uses rep stosb to zero memory.

--*/
{
  __asm {
    mov         ecx, Count
    xor         eax, eax
    mov         edi, Buffer
    rep stosb
Exit:
  }
}

