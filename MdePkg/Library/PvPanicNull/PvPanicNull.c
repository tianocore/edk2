/** @file

  NULL instance of PvPanicLib.

  Copyright (C) 2025, Yandex. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PvPanicLib.h>

/**
  Send the PVPANIC event using a platform-specific implementation(IO Port/MMIO).

  @param[in] Event          8-bit mask with event type
**/
VOID
EFIAPI
PvPanicLibSendEvent (
  IN UINT8  Event
  )
{
}

/**
  Send a PVPANIC event that reports a guest panic and must
  be handled by the host.
**/
VOID
EFIAPI
PvPanicLibSendEventGuestPanicked (
  VOID
  )
{
}

/**
  Send a PVPANIC event that reports a guest panic that could
  be handled by the guest.
**/
VOID
EFIAPI
PvPanicLibSendEventGuestCrashLoaded (
  VOID
  )
{
}

/**
  Send the PVPANIC event that reports a regular guest shutdown.

**/
VOID
EFIAPI
PvPanicLibSendEventGuestShutdown (
  VOID
  )
{
}
