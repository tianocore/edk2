/** @file
  Protocol that exposes alternatives for the RaiseTPL/RestoreTPL boot services
  that interrupt handlers can use to restore the TPL from TPL_HIGH_LEVEL back
  to the original TPL in a manner that is guaranteed to be free from the risk
  of stack exhaustion when many interrupts are delivered in a short time.

  Copyright (c) 2026 Google LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#define EDKII_INTERRUPT_HANDLER_TPL_CONTROL_PROTOCOL_GUID \
  { \
    0x643a2d7d, 0xa44d, 0x4ea9, { 0x82, 0x9c, 0x04, 0x76, 0x02, 0x9e, 0x5f, 0x6a } \
  }

/**
  Raises a task's priority level to TPL_HIGH_LEVEL and returns its previous
  level.

  @return Previous task priority level

**/
typedef
EFI_TPL
(EFIAPI *EDKII_INTERRUPT_HANDLER_RAISE_TPL)(
  VOID
  );

/**
  Restores a task's priority level from TPL_HIGH_LEVEL to its previous value
  without re-enabling interrupts.

  @param[in]  OldTpl          The previous task priority level to restore.

**/
typedef
VOID
(EFIAPI *EDKII_INTERRUPT_HANDLER_RESTORE_TPL)(
  IN EFI_TPL      OldTpl
  );

///
/// Variable Lock Protocol is related to EDK II-specific implementation of variables
/// and intended for use as a means to mark a variable read-only after the event
/// EFI_END_OF_DXE_EVENT_GUID is signaled.
///
typedef struct {
  EDKII_INTERRUPT_HANDLER_RAISE_TPL      RaiseTpl;
  EDKII_INTERRUPT_HANDLER_RESTORE_TPL    RestoreTpl;
} EDKII_INTERRUPT_HANDLER_TPL_CONTROL_PROTOCOL;

extern EFI_GUID  gEdkiiInterruptHandlerTplControlProtocolGuid;
