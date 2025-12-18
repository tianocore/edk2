/** @file
  IPMI Os watchdog timer Driver.

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IpmiCommandLib.h>
#include <IndustryStandard/Ipmi.h>

BOOLEAN  mOsWdtFlag = FALSE;

EFI_EVENT  mExitBootServicesEvent;

/*++

Routine Description:
  Enable the OS Boot Watchdog Timer.
  Is called only on legacy or EFI OS boot.

Arguments:
  Event    - Event type
  *Context - Context for the event

Returns:
  None

--*/
VOID
EFIAPI
EnableEfiOsBootWdtHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                        Status;
  IPMI_SET_WATCHDOG_TIMER_REQUEST   SetWatchdogTimer;
  UINT8                             CompletionCode;
  IPMI_GET_WATCHDOG_TIMER_RESPONSE  GetWatchdogTimer;
  static BOOLEAN                    OsWdtEventHandled = FALSE;

  DEBUG ((DEBUG_ERROR, "!!! EnableEfiOsBootWdtHandler()!!!\n"));

  //
  // Make sure it processes once only. And proceess it only if OsWdtFlag==TRUE;
  //
  if (OsWdtEventHandled || !mOsWdtFlag) {
    return;
  }

  OsWdtEventHandled = TRUE;

  Status = IpmiGetWatchdogTimer (&GetWatchdogTimer);
  if (EFI_ERROR (Status)) {
    return;
  }

  ZeroMem (&SetWatchdogTimer, sizeof (SetWatchdogTimer));
  //
  // Just flip the Timer Use bit. This should release the timer.
  //
  SetWatchdogTimer.TimerUse.Bits.TimerRunning    = 1;
  SetWatchdogTimer.TimerUse.Bits.TimerUse        = IPMI_WATCHDOG_TIMER_OS_LOADER;
  SetWatchdogTimer.TimerActions.Uint8            = IPMI_WATCHDOG_TIMER_ACTION_HARD_RESET;
  SetWatchdogTimer.TimerUseExpirationFlagsClear &= ~BIT4;
  SetWatchdogTimer.TimerUseExpirationFlagsClear |= BIT1 | BIT2;
  SetWatchdogTimer.InitialCountdownValue         = 600; // 100ms / count

  Status = IpmiSetWatchdogTimer (&SetWatchdogTimer, &CompletionCode);
  return;
}

/*++

Routine Description:
  This is the standard EFI driver point. This function intitializes
  the private data required for creating ASRR Driver.

Arguments:
  As required for DXE driver enrty routine.
  ImageHandle - ImageHandle of the loaded driver
  SystemTable - Pointer to the System Table

Returns:
  @retval EFI_SUCCESS           Protocol successfully started and installed.
  @retval EFI_OUT_OF_RESOURCES  The event could not be allocated.

--*/
EFI_STATUS
EFIAPI
DriverInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_NOTIFY,
                  EnableEfiOsBootWdtHandler,
                  NULL,
                  &mExitBootServicesEvent
                  );

  return Status;
}
