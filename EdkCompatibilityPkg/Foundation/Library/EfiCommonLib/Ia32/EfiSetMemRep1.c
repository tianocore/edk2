/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiSetMemRep1.c

Abstract:

  This is the code that uses rep stosb SetMem service

--*/

#include "Tiano.h"

VOID
EfiCommonLibSetMem (
  IN VOID   *Buffer,
  IN UINTN  Count,
  IN UINT8  Value
  )
/*++

Input:  VOID   *Buffer - Pointer to buffer to write
        UINTN  Count   - Number of bytes to write
        UINT8  Value   - Value to write

Output: None.

Saves:

Modifies:

Description:  This function uses rep stosb to set memory.

--*/
{
  __asm {
    mov         ecx, Count
    test        ecx, ecx
    je          Exit
    mov         al,  Value
    mov         edi, Buffer
    rep stosb
Exit:
  }
}

