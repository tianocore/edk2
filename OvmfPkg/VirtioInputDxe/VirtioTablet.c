/** @file

  EFI_ABSOLUTE_POINTER_PROTOCOL implementation for virtio tablet.

  Copyright (C) 2026, Advanced Micro Devices, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/VirtioLib.h>

#include <IndustryStandard/VirtioInput.h>

#include "VirtioInput.h"
#include "VirtioKeyCodes.h"

BOOLEAN
VirtioTabletProbe (
  IN VIRTIO_INPUT_DEV  *Dev
  )
{
  EFI_STATUS  Status;
  UINT8       Size;
  UINT8       Bitmap;

  // A tablet (absolute pointer) reports ABS_X and ABS_Y in the EV_ABS bitmap
  Status = VirtioInputConfigQuerySize (Dev, VirtioInputCfgEvBits, EV_ABS, &Size);
  if (EFI_ERROR (Status) || (Size == 0)) {
    return FALSE;
  }

  Status = Dev->VirtIo->ReadDevice (Dev->VirtIo, OFFSET_OF_VINPUT (Data), 1, 1, &Bitmap);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return (Bitmap & (1 << ABS_X)) && (Bitmap & (1 << ABS_Y));
}

// -----------------------------------------------------------------------------
// Function handling VirtIO tablet events
VOID
VirtioTabletHandleEvent (
  IN OUT VIRTIO_INPUT_DEV  *Dev,
  IN VIRTIO_INPUT_EVENT    *Event
  )
{
  switch (Event->Type) {
    case EV_KEY:
      switch (Event->Code) {
        case BTN_TOUCH:
        case BTN_LEFT:
          if (Event->Value == KEY_PRESSED) {
            Dev->AbsPointerState.ActiveButtons |= (UINT32)EFI_ABSP_TouchActive;
          } else {
            Dev->AbsPointerState.ActiveButtons &= ~(UINT32)EFI_ABSP_TouchActive;
          }

          break;

        case BTN_RIGHT:
          if (Event->Value == KEY_PRESSED) {
            Dev->AbsPointerState.ActiveButtons |= (UINT32)EFI_ABS_AltActive;
          } else {
            Dev->AbsPointerState.ActiveButtons &= ~(UINT32)EFI_ABS_AltActive;
          }

          break;

        default:
          break;
      }

      Dev->AbsPointerReady = TRUE;
      break;

    case EV_ABS:
      switch (Event->Code) {
        case ABS_X:
          Dev->AbsPointerState.CurrentX = Event->Value;
          break;

        case ABS_Y:
          Dev->AbsPointerState.CurrentY = Event->Value;
          break;

        default:
          break;
      }

      Dev->AbsPointerReady = TRUE;
      break;

    default:
      break;
  }
}

// -----------------------------------------------------------------------------
// EFI_ABSOLUTE_POINTER_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioTabletReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
  VIRTIO_INPUT_DEV  *Dev;
  EFI_TPL           OldTpl;

  Dev = VIRTIO_INPUT_FROM_ABS_POINTER_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  ZeroMem (&Dev->AbsPointerState, sizeof (Dev->AbsPointerState));
  Dev->AbsPointerReady = FALSE;
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// EFI_ABSOLUTE_POINTER_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioTabletGetState (
  IN  EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  OUT EFI_ABSOLUTE_POINTER_STATE     *State
  )
{
  VIRTIO_INPUT_DEV  *Dev;
  EFI_TPL           OldTpl;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_INPUT_FROM_ABS_POINTER_THIS (This);

  if (!Dev->AbsPointerReady) {
    return EFI_NOT_READY;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  CopyMem (State, &Dev->AbsPointerState, sizeof (*State));

  //
  // The reported position is absolute, so it persists; only clear the "new
  // data" flag so the next GetState () returns EFI_NOT_READY until a fresh
  // event arrives.
  //
  Dev->AbsPointerReady = FALSE;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// EFI_ABSOLUTE_POINTER_PROTOCOL WaitForInput event handler
STATIC
VOID
EFIAPI
VirtioTabletWaitForInput (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  VIRTIO_INPUT_DEV  *Dev = Context;

  //
  // Stall 1ms to give other timer-driven drivers a chance to run while this
  // routine is recursively invoked from WaitForEvent ().
  //
  gBS->Stall (1000);

  // Drain pending events from the device.
  VirtioInputTimer (NULL, Dev);

  // If there is new pointer activity - send signal
  if (Dev->AbsPointerReady) {
    gBS->SignalEvent (Event);
  }
}

STATIC
EFI_STATUS
VirtioTabletGetAbsMinMax (
  IN  VIRTIO_INPUT_DEV  *Dev,
  IN  UINT8             Axis,
  OUT UINT32            *Min,
  OUT UINT32            *Max
  )
{
  EFI_STATUS  Status;
  UINT8       Size;

  Status = VirtioInputConfigQuerySize (Dev, VirtioInputCfgAbsInfo, Axis, &Size);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Size < sizeof (VIRTIO_INPUT_ABS_INFO)) {
    return EFI_UNSUPPORTED;
  }

  Status = Dev->VirtIo->ReadDevice (
                          Dev->VirtIo,
                          OFFSET_OF_VINPUT (Data.Abs.Min),
                          SIZE_OF_VINPUT (Data.Abs.Min),
                          sizeof (*Min),
                          Min
                          );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Dev->VirtIo->ReadDevice (
                          Dev->VirtIo,
                          OFFSET_OF_VINPUT (Data.Abs.Max),
                          SIZE_OF_VINPUT (Data.Abs.Max),
                          sizeof (*Max),
                          Max
                          );
  return Status;
}

EFI_STATUS
VirtioTabletInit (
  IN OUT VIRTIO_INPUT_DEV  *Dev
  )
{
  EFI_STATUS  Status;
  UINT32      AbsMinX;
  UINT32      AbsMaxX;
  UINT32      AbsMinY;
  UINT32      AbsMaxY;

  Dev->AbsolutePointer.Reset    = VirtioTabletReset;
  Dev->AbsolutePointer.GetState = VirtioTabletGetState;
  Dev->AbsolutePointer.Mode     = &Dev->AbsPointerMode;

  ZeroMem (&Dev->AbsPointerMode, sizeof (Dev->AbsPointerMode));

  Status = VirtioTabletGetAbsMinMax (Dev, ABS_X, &AbsMinX, &AbsMaxX);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = VirtioTabletGetAbsMinMax (Dev, ABS_Y, &AbsMinY, &AbsMaxY);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev->AbsPointerMode.AbsoluteMinX = AbsMinX;
  Dev->AbsPointerMode.AbsoluteMaxX = AbsMaxX;
  Dev->AbsPointerMode.AbsoluteMinY = AbsMinY;
  Dev->AbsPointerMode.AbsoluteMaxY = AbsMaxY;
  Dev->AbsPointerMode.Attributes   = EFI_ABSP_SupportsAltActive;

  ZeroMem (&Dev->AbsPointerState, sizeof (Dev->AbsPointerState));
  Dev->AbsPointerReady = FALSE;

  //
  // Setup the WaitForInput event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  VirtioTabletWaitForInput,
                  Dev,
                  &Dev->AbsolutePointer.WaitForInput
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

VOID
VirtioTabletUninit (
  IN OUT VIRTIO_INPUT_DEV  *Dev
  )
{
  gBS->CloseEvent (Dev->AbsolutePointer.WaitForInput);
}
