/** @file
    IPMI Fault Resilient Booting Driver.

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/IpmiCommandLib.h>
#include <IndustryStandard/Ipmi.h>

#include <Library/ManageabilityTransportHelperLib.h>

/**
  This routine disables the specified FRB timer.

  @retval EFI_STATUS  EFI_SUCCESS      FRB timer was disabled
                      EFI_ABORTED      Timer was already stopped
                      EFI_UNSUPPORTED  This type of FRB timer is not supported.

**/
EFI_STATUS
EfiDisableFrb (
  VOID
  )
{
  EFI_STATUS                        Status;
  IPMI_SET_WATCHDOG_TIMER_REQUEST   SetWatchdogTimer;
  UINT8                             CompletionCode;
  IPMI_GET_WATCHDOG_TIMER_RESPONSE  GetWatchdogTimer;

  Status = IpmiGetWatchdogTimer (&GetWatchdogTimer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if timer is still running, if not abort disable routine.
  //
  if (GetWatchdogTimer.TimerUse.Bits.TimerRunning == 0) {
    return EFI_ABORTED;
  }

  ZeroMem (&SetWatchdogTimer, sizeof (SetWatchdogTimer));
  //
  // Just flip the Timer Use bit. This should release the timer.
  //
  SetWatchdogTimer.TimerUse.Bits.TimerRunning    = 0;
  SetWatchdogTimer.TimerUse.Bits.TimerUse        = IPMI_WATCHDOG_TIMER_BIOS_FRB2;
  SetWatchdogTimer.TimerUseExpirationFlagsClear &= ~BIT2;
  SetWatchdogTimer.TimerUseExpirationFlagsClear |= BIT1 | BIT4;

  Status = IpmiSetWatchdogTimer (&SetWatchdogTimer, &CompletionCode);
  return Status;
}

/**
  This function disables FRB2. This function gets called each time the
  EFI_EVENT_SIGNAL_READY_TO_BOOT gets signaled

  @param[in] Event    The handle of callback event.
  @param[in] Context  This should be NULL means no context associated
                      with this event.

**/
VOID
EFIAPI
DisableFRB2Handler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DEBUG ((DEBUG_ERROR, "!!! enter DisableFRB2Handler()!!!\n"));

  EfiDisableFrb ();
}

/**
  This function checks the Watchdog timer expiration flags and
  report the kind of watchdog timeout occurred to the Error
  Manager.

  @retval EFI_STATUS  EFI_SUCCESS  Timeout status is checked and cleared.
                      EFI_ERROR    There was an error when check and clear
                                   timeout status.

**/
EFI_STATUS
CheckForAndReportErrors (
  VOID
  )
{
  EFI_STATUS                        Status;
  IPMI_GET_WATCHDOG_TIMER_RESPONSE  GetWatchdogTimer;
  IPMI_SET_WATCHDOG_TIMER_REQUEST   SetWatchdogTimer;
  UINT8                             CompletionCode;

  //
  // Get the Watchdog timer info to find out what kind of timer expiration occurred.
  //
  Status = IpmiGetWatchdogTimer (&GetWatchdogTimer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If FRB2 Failure occurred, report it to the error manager and log a SEL.
  //
  if ((GetWatchdogTimer.TimerUseExpirationFlagsClear & BIT1) != 0) {
    //
    // Report the FRB2 time-out error
    //
  } else if ((GetWatchdogTimer.TimerUseExpirationFlagsClear & BIT3) != 0) {
    //
    // Report the OS Watchdog timer failure
    //
  }

  //
  // Need to clear Timer expiration flags after checking.
  //
  ZeroMem (&SetWatchdogTimer, sizeof (SetWatchdogTimer));
  SetWatchdogTimer.TimerUse                      = GetWatchdogTimer.TimerUse;
  SetWatchdogTimer.TimerActions                  = GetWatchdogTimer.TimerActions;
  SetWatchdogTimer.PretimeoutInterval            = GetWatchdogTimer.PretimeoutInterval;
  SetWatchdogTimer.TimerUseExpirationFlagsClear  = GetWatchdogTimer.TimerUseExpirationFlagsClear;
  SetWatchdogTimer.InitialCountdownValue         = GetWatchdogTimer.InitialCountdownValue;
  SetWatchdogTimer.TimerUse.Bits.TimerRunning    = 1;
  SetWatchdogTimer.TimerUseExpirationFlagsClear |= BIT1 | BIT2 | BIT3;

  Status = IpmiSetWatchdogTimer (&SetWatchdogTimer, &CompletionCode);

  return Status;
}

/**
  This routine is built only when DEBUG_MODE is enabled.  It is used
  to report the status of FRB2 when the FRB2 driver is installed.

  @retval EFI_STATUS  EFI_SUCCESS  All info was retrieved and reported
                      EFI_ERROR    There was an error during info retrieval

**/
EFI_STATUS
ReportFrb2Status (
  VOID
  )
{
  EFI_STATUS                        Status;
  IPMI_GET_WATCHDOG_TIMER_RESPONSE  GetWatchdogTimer;

  //
  // Get the Watchdog timer info to find out what kind of timer expiration occurred.
  //
  Status = IpmiGetWatchdogTimer (&GetWatchdogTimer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get Watchdog Timer info from BMC.\n"));
    return Status;
  }

  //
  // Check if timer is running, report status to DEBUG_MODE output.
  //
  if (GetWatchdogTimer.TimerUse.Bits.TimerRunning == 1) {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "FRB2 Timer is running.\n"));
  } else {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "FRB2 Timer is not running.\n"));
  }

  return EFI_SUCCESS;
}

/**
  The entry point of the Ipmi Fault Resilient Booting DXE driver.

  @param[in] ImageHandle - Handle of this driver image
  @param[in] SystemTable - Table containing standard EFI services

  @retval EFI_SUCCESS    - IPMI Protocol is installed successfully.
  @retval Otherwise      - Other errors.

**/
EFI_STATUS
EFIAPI
FrbDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT   ReadyToBootEvent;
  EFI_STATUS  Status;

  CheckForAndReportErrors ();
  ReportFrb2Status ();

  //
  // Register the event to Disable FRB2 before Boot.
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_NOTIFY,
             DisableFRB2Handler,
             NULL,
             &ReadyToBootEvent
             );

  return Status;
}
