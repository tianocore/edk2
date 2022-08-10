/** @file
  FSP Wrapper to handle platform specific actions for
  FSP MultiPhase (SeparatePhase) Initialization.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

/**
  FSP Wrapper Platform MultiPhase Handler

  @param[in] FspHobListPtr        - Pointer to FSP HobList (valid after FSP-M completed)
  @param[in] ComponentIndex       - FSP Component which executing MultiPhase initialization.
  @param[in] PhaseIndex           - Indicates current execution phase of FSP MultiPhase initialization.

  @retval EFI_STATUS        Always return EFI_SUCCESS

**/
VOID
EFIAPI
FspWrapperPlatformMultiPhaseHandler (
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex,
  IN UINT32    PhaseIndex
  )
{
  /* Example platform actions as below
  switch (ComponentIndex) {
  case FspMultiPhaseMemInitApiIndex:
    switch (PhaseIndex) {
      case 1:
        PlatformAction1 ();
      break;
    }
    break;
  case FspMultiPhaseSiInitApiIndex:
    switch (PhaseIndex) {
      case 1:
        PlatformAction2 ();
      break;
    }
    break;
  }
  */
}
