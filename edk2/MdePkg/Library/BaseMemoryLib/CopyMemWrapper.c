/** @file
  CopyMem() implementation.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  CopyMemWrapper.c

  The following BaseMemoryLib instances share the same version of this file:

    BaseMemoryLib
    BaseMemoryLibMmx
    BaseMemoryLibSse2
    BaseMemoryLibRepStr
    PeiMemoryLib
    UefiMemoryLib

**/

#include "MemLibWrappers.h"

/**
  Copy Length bytes from Source to Destination.

  This function copies Length bytes from SourceBuffer to DestinationBuffer, and
  returns DestinationBuffer. The implementation must be reentrant, and it must
  handle the case where SourceBuffer overlaps DestinationBuffer.

  If Length is greater than (MAX_ADDRESS - DestinationBuffer + 1), then
  ASSERT().
  If Length is greater than (MAX_ADDRESS - SourceBuffer + 1), then ASSERT().

  @param  Destination Target of copy
  @param  Source Place to copy from
  @param  Length Number of bytes to copy

  @return Destination

**/
VOID *
EFIAPI
CopyMem (
  OUT     VOID                      *Destination,
  IN      CONST VOID                *Source,
  IN      UINTN                     Length
  )
{
  ASSERT (Length <= MAX_ADDRESS - (UINTN)Destination + 1);
  ASSERT (Length <= MAX_ADDRESS - (UINTN)Source + 1);
  if (Destination == Source || Length == 0) {
    return Destination;
  }
  return InternalMemCopyMem (Destination, Source, Length);
}
