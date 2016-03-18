/** @file
  Implementation of the InternalMemCopyMem routine. This function is broken
  out into its own source file so that it can be excluded from a build for a
  particular platform easily if an optimized version is desired.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/




#include "MemLibInternals.h"

/**
  Copy Length bytes from Source to Destination.

  @param  DestinationBuffer The target of the copy request.
  @param  SourceBuffer      The place to copy from.
  @param  Length            The number of bytes to copy.

  @return Destination

**/
VOID *
EFIAPI
InternalMemCopyMem (
  OUT     VOID                      *DestinationBuffer,
  IN      CONST VOID                *SourceBuffer,
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

  if (SourceBuffer > DestinationBuffer) {
    Destination8 = (UINT8*)DestinationBuffer;
    Source8 = (CONST UINT8*)SourceBuffer;
    while (Length-- != 0) {
      *(Destination8++) = *(Source8++);
    }
  } else if (SourceBuffer < DestinationBuffer) {
    Destination8 = (UINT8*)DestinationBuffer + Length;
    Source8 = (CONST UINT8*)SourceBuffer + Length;
    while (Length-- != 0) {
      *(--Destination8) = *(--Source8);
    }
  }
  return DestinationBuffer;
}
