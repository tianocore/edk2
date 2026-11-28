/** @file
  Contructor for Timer Library functions

  Copyright (c) 2022, Citrix Systems, Inc.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

extern UINT32  mFSBClock;

/**
  The constructor cache the value of the PcdFSBClock. This might be
  necessary when the PCD is dynamic.

  @retval RETURN_SUCCESS   The constructor always returns success.
**/
RETURN_STATUS
EFIAPI
TimerLibConstruct (
  VOID
  )
{
  //
  // Cache current value of PcdFSBClock when it's a dynamic PCD.
  //
  mFSBClock = PcdGet32 (PcdFSBClock);
  return RETURN_SUCCESS;
}
