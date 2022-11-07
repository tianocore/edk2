/** @file
  SMM IPL that load the SMM Core into SMRAM

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <PiSmm.h>
#include <StandaloneMm.h>

/**
  Load SMM core to dispatch other Standalone MM drivers.

  @param  Entry                     Entry of Standalone MM Foundation.
  @param  Context1                  A pointer to the context to pass into the EntryPoint
                                    function.
  @retval EFI_SUCCESS               Successfully loaded SMM core.
  @retval Others                    Failed to load SMM core.
**/
EFI_STATUS
LoadSmmCore (
  IN EFI_PHYSICAL_ADDRESS  Entry,
  IN VOID                  *Context1
  )
{
  STANDALONE_MM_FOUNDATION_ENTRY_POINT  EntryPoint;

  EntryPoint = (STANDALONE_MM_FOUNDATION_ENTRY_POINT)(UINTN)Entry;
  return EntryPoint (Context1);
}
