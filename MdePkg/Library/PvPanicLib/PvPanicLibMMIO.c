/** @file

  Provide services to PvPanic QEMU MMIO Device

  Copyright (C) 2025, Yandex. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PvPanicLib.h>

/**
  Send the PVPANIC event using a MMIO range.

  @param[in] Event          8-bit mask with event type
**/
VOID
EFIAPI
PvPanicLibSendEvent (
  IN UINT8  Event
  )
{
  UINT16  Value;

  // Adjust size for MMIO region
  Value = Event;
  DEBUG ((DEBUG_INFO, "[%a] Sending Event: 0x%x\n", __func__, Value));

  MmioWrite16 (PcdGet32 (PcdPvPanicMmioAddr), Value);
}
