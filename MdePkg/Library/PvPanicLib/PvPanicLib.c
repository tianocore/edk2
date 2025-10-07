/** @file

  Provide services to PvPanic QEMU ISA/MMIO Device

  Copyright (C) 2025, Yandex. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PvPanicLib.h>

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
  PvPanicLibSendEvent (PVPANIC_GUEST_PANICKED);
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
  PvPanicLibSendEvent (PVPANIC_GUEST_CRASHLOADED);
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
  PvPanicLibSendEvent (PVPANIC_GUEST_SHUTDOWN);
}
