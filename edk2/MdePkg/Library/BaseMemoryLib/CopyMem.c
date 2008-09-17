/** @file
  Implementation of the EfiCopyMem routine. This function is broken
  out into its own source file so that it can be excluded from a
  build for a particular platform easily if an optimized version
  is desired.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/




#include "MemLibInternals.h"

/**
  Copy Length bytes from Source to Destination.

  @param  Destination Target of copy
  @param  Source Place to copy from
  @param  Length Number of bytes to copy

  @return Destination

**/
VOID *
EFIAPI
InternalMemCopyMem (
  OUT     VOID                      *Destination,
  IN      CONST VOID                *Source,
  IN      UINTN                     Length
  )
{
  //
  // Declare the local variables that actually move the data elements as
  // volatile to prevent the optimizer from replacing this function with
  // the intrinsic memcpy()
  //
  volatile UINT8                    *Destination8;
  CONST UINT8                       *Source8;

  if (Source > Destination) {
    Destination8 = (UINT8*)Destination;
    Source8 = (CONST UINT8*)Source;
    while (Length-- != 0) {
      *(Destination8++) = *(Source8++);
    }
  } else if (Source < Destination) {
    Destination8 = (UINT8*)Destination + Length;
    Source8 = (CONST UINT8*)Source + Length;
    while (Length-- != 0) {
      *(--Destination8) = *(--Source8);
    }
  }
  return Destination;
}
