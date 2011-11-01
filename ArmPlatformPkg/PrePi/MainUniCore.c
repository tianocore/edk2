/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "PrePi.h"

VOID
PrimaryMain (
  IN  UINTN                     UefiMemoryBase,
  IN  UINTN                     StacksBase,
  IN  UINTN                     GlobalVariableBase,
  IN  UINT64                    StartTimeStamp
  )
{
  DEBUG_CODE_BEGIN();
    // On MPCore system, PeiMpCore.inf should be used instead of PeiUniCore.inf
    ASSERT(ArmIsMpCore() == 0);
  DEBUG_CODE_END();

  PrePiMain (UefiMemoryBase, StacksBase, GlobalVariableBase, StartTimeStamp);

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

