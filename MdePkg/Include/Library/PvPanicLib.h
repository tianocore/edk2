/** @file

  Provide services to PvPanic QEMU ISA Device

  Copyright (C) 2025, Yandex. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PV_PANIC_LIB_H__
#define __PV_PANIC_LIB_H__

/**
  See more detailed information about bit definition
  https://www.qemu.org/docs/master/specs/pvpanic.html
**/

// a guest panic has happened and should be processed by the host.
#define PVPANIC_GUEST_PANICKED  (1 << 0)
// a guest panic has happened and will be handled by the guest; the host should
// record it or report it, but should not affect the execution of the guest.
#define PVPANIC_GUEST_CRASHLOADED  (1 << 1)
// a regular guest shutdown has happened and should be processed by the host.
#define PVPANIC_GUEST_SHUTDOWN  (1 << 2)

/**
  Send the PVPANIC event using a platform-specific implementation(IO Port/MMIO).

  @param[in] Event          8-bit mask with event type
**/
VOID
EFIAPI
PvPanicLibSendEvent (
  IN UINT8  Event
  );

/**
  Send a PVPANIC event that reports a guest panic and must
  be handled by the host.
**/
VOID
EFIAPI
PvPanicLibSendEventGuestPanicked (
  VOID
  );

/**
  Send a PVPANIC event that reports a guest panic that could
  be handled by the guest.
**/
VOID
EFIAPI
PvPanicLibSendEventGuestCrashLoaded (
  VOID
  );

/**
  Send the PVPANIC event that reports a regular guest shutdown.

**/
VOID
EFIAPI
PvPanicLibSendEventGuestShutdown (
  VOID
  );

#endif //__PV_PANIC_LIB_H__
