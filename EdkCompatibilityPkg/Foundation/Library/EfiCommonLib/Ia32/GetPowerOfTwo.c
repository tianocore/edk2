/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GetPowerOfTwo.c

Abstract:

  Calculates the largest integer that is both 
  a power of two and less than Input

--*/

#include "Tiano.h"

UINT64
GetPowerOfTwo (
  IN UINT64   Input
  )
/*++

Routine Description:

  Calculates the largest integer that is both 
  a power of two and less than Input

Arguments:

  Input  - value to calculate power of two

Returns:

  the largest integer that is both  a power of 
  two and less than Input

--*/
{
  __asm {
    xor     eax, eax
    mov     edx, eax
    mov     ecx, dword ptr Input[4]
    jecxz   _F
    bsr     ecx, ecx
    bts     edx, ecx
    jmp     _Exit
_F:
    mov     ecx, dword ptr Input[0]
    jecxz   _Exit
    bsr     ecx, ecx
    bts     eax, ecx
_Exit:
  }
}
