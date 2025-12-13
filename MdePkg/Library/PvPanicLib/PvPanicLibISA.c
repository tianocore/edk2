/** @file

  Provide services to PvPanic QEMU ISA Device

  Copyright (C) 2025, Yandex. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PvPanicLib.h>

/**
  Send the PVPANIC event using an ISA IO port.

  @param[in] Event          8-bit mask with event type
**/
VOID
EFIAPI
PvPanicLibSendEvent (
  IN UINT8  Event
  )
{
  DEBUG ((DEBUG_INFO, "[%a] Sending Event: 0x%x\n", __func__, Event));

  IoWrite8 (PcdGet32 (PcdPvPanicIoPort), Event);
}
