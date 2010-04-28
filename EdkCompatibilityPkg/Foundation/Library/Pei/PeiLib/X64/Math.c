/*++

Copyright (c) 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Math.c

Abstract:

  64-bit Math worker functions for x64

--*/

#include "Efi.h"
#include "Pei.h"
#include "PeiLib.h"


UINT64
LShiftU64 (
  IN UINT64   Operand,
  IN UINTN    Count
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be left shifted by 32 bits and returns the 
  shifted value.
  Count is valid up 63. (Only Bits 0-5 is valid for Count)

Arguments:

  Operand - Value to be shifted
  
  Count   - Number of times to shift left.
  
Returns:

  Value shifted left identified by the Count.
  
--*/
{
  return Operand << Count;
}


UINT64
MultU64x32 (
  IN UINT64   Multiplicand,
  IN UINTN    Multiplier
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be multiplied with a 32 bit value returns 
  64bit result.
  No checking if the result is greater than 64bits

Arguments:

  Multiplicand  - 
  
  Multiplier    - 
  
Returns:

  Multiplicand * Multiplier
  
--*/
{
  return Multiplicand * Multiplier;
}

UINT64
DivU64x32 (
  IN UINT64   Dividend,
  IN UINTN    Divisor,
  OUT UINTN   *Remainder OPTIONAL
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be divided with a 32 bit value returns 
  64bit result and the Remainder.

Arguments:

  Dividend  - 
  
  Divisor   - 
  
  Remainder -
  
Returns:

  Dividend  / Divisor
  Remainder = Dividend mod Divisor
  
N.B. only works for 31bit divisors!!
    
--*/
{
  return Dividend/Divisor;
}

