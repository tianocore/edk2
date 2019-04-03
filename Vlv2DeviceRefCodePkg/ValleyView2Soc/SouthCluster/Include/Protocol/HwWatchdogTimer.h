/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



Module Name:

  HwWatchdogTimer.h

Abstract:


--*/

#ifndef __EFI_WATCHDOG_TIMER_DRIVER_PROTOCOL_H__
#define __EFI_WATCHDOG_TIMER_DRIVER_PROTOCOL_H__

#define EFI_WATCHDOG_TIMER_DRIVER_PROTOCOL_GUID \
  { 0xd5b06d16, 0x2ea1, 0x4def, 0x98, 0xd0, 0xa0, 0x5d, 0x40, 0x72, 0x84, 0x17 }

#define EFI_WATCHDOG_TIMER_NOT_SUPPORTED_PROTOCOL_GUID \
  { 0xe9e156ac, 0x3203, 0x4572, 0xac, 0xdf, 0x84, 0x4f, 0xdc, 0xdb, 0x6, 0xbf }


#include <Guid/HwWatchdogTimerHob.h>

//
// General Purpose Constants
//
#define ICH_INSTAFLUSH_GPIO      BIT16 // BIT 16 in GPIO Level 2 is GPIO 48.
#define B_INSTAFLUSH             BIT4
//
// Other Watchdog timer values
//
#define WDT_COUNTDOWN_VALUE                 0x14
#define BDS_WDT_COUNTDOWN_VALUE             0x35


//
// Prototypes for the Watchdog Timer Driver Protocol
//

typedef
EFI_STATUS
(EFIAPI *EFI_WATCHDOG_START_TIMER) (
  VOID
  );
/*++

  Routine Description:
    This service begins the Watchdog Timer countdown.  If the countdown completes prior to
    Stop Timer or Restart Timer the system will reset.

  Arguments:
    None

  Returns:
    EFI_SUCCESS       - Operation completed successfully
    EFI_DEVICE_ERROR  - The command was unsuccessful

--*/



typedef
EFI_STATUS
(EFIAPI *PEI_WATCHDOG_RESET_TIMER) (
  VOID
  );
/*++

  Routine Description:
    This service resets the Watchdog Timer countdown and should only be called after the
    Start Timer function.

  Arguments:
    None

  Returns:
    EFI_SUCCESS       - Operation completed successfully
    EFI_DEVICE_ERROR  - The command was unsuccessful

--*/




typedef
EFI_STATUS
(EFIAPI *EFI_WATCHDOG_RESTART_TIMER) (
  VOID
  );
/*++

  Routine Description:
    This service restarts the Watchdog Timer countdown and should only be called after the
    Start Timer function.

  Arguments:
    None

  Returns:
    EFI_SUCCESS       - Operation completed successfully
    EFI_DEVICE_ERROR  - The command was unsuccessful

--*/




typedef
EFI_STATUS
(EFIAPI *EFI_WATCHDOG_STOP_TIMER) (
  VOID
  );
/*++

  Routine Description:
    This service disables the Watchdog Timer countdown.

  Arguments:
    None

  Returns:
    EFI_SUCCESS       - Operation completed successfully
    EFI_DEVICE_ERROR  - The command was unsuccessful

--*/



typedef
EFI_STATUS
(EFIAPI *EFI_WATCHDOG_CHECK_TIMEOUT) (
  OUT HW_WATCHDOG_TIMEOUT       *WatchdogTimeout
  );
/*++

  Routine Description:
    This service disables the Watchdog Timer countdown.

  Arguments:
    None

  Returns:
    EFI_SUCCESS       - Operation completed successfully
    EFI_DEVICE_ERROR  - The command was unsuccessful

--*/



typedef
EFI_STATUS
(EFIAPI *EFI_WATCHDOG_FORCE_REBOOT) (
  IN BOOLEAN                    ForceTimeout,
  IN UINT8                      ResetType
  );
/*++

  Routine Description:
    This service forces a reboot of the system due to a reset of the POWERGOOD_PS,
    POWERGOOD_CLK, and the BSEL Override

  Arguments:
    None

  Returns:
    This function should not return!

    EFI_DEVICE_ERROR  - The command was unsuccessful and a reboot did not occur

--*/



typedef
EFI_STATUS
(EFIAPI *EFI_WATCHDOG_KNOWN_RESET) (
  IN BOOLEAN                    AllowReset
  );
/*++

  Routine Description:
    This service notifies the Watchdog Timer of the fact that a known reset is occuring.

  Arguments:
    AllowReset -  TRUE if a Reset is currently expected
                  FALSE if a Reset is not currently expected

  Returns:
    This function should not return!

    EFI_DEVICE_ERROR  - The command was unsuccessful and a reboot did not occur

--*/

typedef
EFI_STATUS
(EFIAPI *EFI_GET_TIMER_COUNT_DOWN_PERIOD)(
  OUT UINT32      *CountdownValue
  );
/*++

  Routine Description:
    This service reads the current Watchdog Timer countdown reload value.

  Arguments:
    CountdownValue - pointer to UINT32 to return the value of the reload register.

  Returns:
    EFI_SUCCESS       - Operation completed successfully
    EFI_DEVICE_ERROR  - The command was unsuccessful

--*/

typedef
EFI_STATUS
(EFIAPI *EFI_SET_TIMER_COUNT_DOWN_PERIOD)(
  OUT UINT32      CountdownValue
  );
/*++

  Routine Description:
    This service reads the current Watchdog Timer countdown reload value.

  Arguments:
    CountdownValue - Value to set the reload register.

  Returns:
    EFI_SUCCESS       - Operation completed successfully
    EFI_DEVICE_ERROR  - The command was unsuccessful

--*/

typedef
EFI_STATUS
(EFIAPI *PEI_WATCHDOG_CLEAR_TIMER_STATE) (
  );
/*++

  Routine Description:
    This service clears the state that indicates the Watchdog Timer fired.

  Arguments:

  Returns:
    EFI_SUCCESS       - Operation completed successfully
    EFI_DEVICE_ERROR  - The command was unsuccessful

--*/

typedef
EFI_STATUS
(EFIAPI *EFI_STALL_WATCHDOG_COUNTDOWN) (
  IN BOOLEAN Stall
  );
/*++

  Routine Description:
    This service disables the Watchdog Timer countdown.  It also closes the recurring restart event
    if the event exists.

  Arguments:
    Stall - TRUE = Stop the timer countdown
            FALSE = Start the timer countdown

  Returns:
    EFI_SUCCESS       - Operation completed successfully
    EFI_DEVICE_ERROR  - The command was unsuccessful

--*/

typedef struct _EFI_WATCHDOG_TIMER_DRIVER_PROTOCOL {
  EFI_WATCHDOG_START_TIMER                      StartWatchdogTimer;
  PEI_WATCHDOG_RESET_TIMER                      ResetWatchdogTimeout;
  EFI_WATCHDOG_RESTART_TIMER                    RestartWatchdogTimer;
  EFI_WATCHDOG_STOP_TIMER                       StopWatchdogTimer;
  EFI_WATCHDOG_CHECK_TIMEOUT                    CheckWatchdogTimeout;
  EFI_WATCHDOG_FORCE_REBOOT                     ForceReboot;
  EFI_WATCHDOG_KNOWN_RESET                      AllowKnownReset;
  EFI_GET_TIMER_COUNT_DOWN_PERIOD               GetCountdownPeriod;
  EFI_SET_TIMER_COUNT_DOWN_PERIOD               SetCountdownPeriod;
  PEI_WATCHDOG_CLEAR_TIMER_STATE                ClearTimerState;
  EFI_STALL_WATCHDOG_COUNTDOWN                  StallWatchdogCountdown;
} EFI_WATCHDOG_TIMER_DRIVER_PROTOCOL;

extern EFI_GUID gEfiWatchdogTimerDriverProtocolGuid;
extern EFI_GUID gEfiWatchdogTimerNotSupportedProtocolGuid;

#endif
