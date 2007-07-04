/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    Stall.c

Abstract:

    Tiano Miscellaneous Services Stall service implementation

--*/

//
// Include statements
//

#include <DxeMain.h>


EFI_STATUS
EFIAPI
CoreStall (
  IN UINTN            Microseconds
  )
/*++

Routine Description:

  Introduces a fine-grained stall.

Arguments:

  Microseconds      The number of microseconds to stall execution

Returns:

  EFI_SUCCESS            - Execution was stalled for at least the requested amount
                           of microseconds.

  EFI_NOT_AVAILABLE_YET  - gMetronome is not available yet

--*/
{
  UINT32  Counter;
  UINT32  Remainder;

  if (gMetronome == NULL) {
    return EFI_NOT_AVAILABLE_YET;
  }

  //
  // Calculate the number of ticks by dividing the number of microseconds by
  // the TickPeriod.
  // Calcullation is based on 100ns unit.
  //
  Counter = (UINT32) DivU64x32Remainder (
                       Microseconds * 10,
                       gMetronome->TickPeriod,
                       &Remainder
                       );

  //
  // Call WaitForTick for Counter + 1 ticks to try to guarantee Counter tick
  // periods, thus attempting to ensure Microseconds of stall time.
  //
  if (Remainder != 0) {
    Counter++;
  }

  gMetronome->WaitForTick (gMetronome, Counter);

  return EFI_SUCCESS;
}
