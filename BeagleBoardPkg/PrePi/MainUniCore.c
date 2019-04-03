/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  Copyright (c) 2017, Linaro, Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "PrePi.h"

VOID
PrimaryMain (
  IN  UINTN                     UefiMemoryBase,
  IN  UINTN                     StacksBase,
  IN  UINT64                    StartTimeStamp
  )
{
  PrePiMain (UefiMemoryBase, StacksBase, StartTimeStamp);

  // We must never return
  ASSERT(FALSE);
}

VOID
SecondaryMain (
  IN  UINTN                     MpId
  )
{
  // We must never get into this function on UniCore system
  ASSERT(FALSE);
}

