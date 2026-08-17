/** @file

  EFI_SIMPLE_POINTER_PROTOCOL implementation for virtio mouse.

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
VirtioMouseProbe (
  IN VIRTIO_INPUT_DEV  *Dev
  )
{
  EFI_STATUS  Status;
  UINT8       Size;
  UINT8       Bitmap;

  // Check if REL_X and REL_Y are set in the EV_REL bitmap
  Status = VirtioInputConfigQuerySize (Dev, VirtioInputCfgEvBits, EV_REL, &Size);
  if (EFI_ERROR (Status) || (Size == 0)) {
    return FALSE;
  }

  Status = Dev->VirtIo->ReadDevice (Dev->VirtIo, OFFSET_OF_VINPUT (Data), 1, 1, &Bitmap);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return (Bitmap & (1 << REL_X)) && (Bitmap & (1 << REL_Y));
}

// -----------------------------------------------------------------------------
// Function handling VirtIO mouse events
VOID
VirtioMouseHandleEvent (
  IN OUT VIRTIO_INPUT_DEV  *Dev,
  IN VIRTIO_INPUT_EVENT    *Event
  )
{
  switch (Event->Type) {
    case EV_KEY:
      switch (Event->Code) {
        case BTN_LEFT:
          Dev->PointerState.LeftButton = (BOOLEAN)(Event->Value == KEY_PRESSED);
          break;

        case BTN_RIGHT:
          Dev->PointerState.RightButton = (BOOLEAN)(Event->Value == KEY_PRESSED);
          break;

        default:
          break;
      }

      Dev->PointerReady = TRUE;
      break;

    case EV_REL:
      switch (Event->Code) {
        case REL_X:
          Dev->PointerState.RelativeMovementX += (INT32)Event->Value;
          break;

        case REL_Y:
          Dev->PointerState.RelativeMovementY += (INT32)Event->Value;
          break;

        default:
          break;
      }

      Dev->PointerReady = TRUE;
      break;

    default:
      break;
  }
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_POINTER_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioMouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL  *This,
  IN BOOLEAN                      ExtendedVerification
  )
{
  VIRTIO_INPUT_DEV  *Dev;
  EFI_TPL           OldTpl;

  Dev = VIRTIO_INPUT_FROM_POINTER_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  ZeroMem (&Dev->PointerState, sizeof (Dev->PointerState));
  Dev->PointerReady = FALSE;
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_POINTER_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioMouseGetState (
  IN  EFI_SIMPLE_POINTER_PROTOCOL  *This,
  OUT EFI_SIMPLE_POINTER_STATE     *State
  )
{
  VIRTIO_INPUT_DEV  *Dev;
  EFI_TPL           OldTpl;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_INPUT_FROM_POINTER_THIS (This);

  if (!Dev->PointerReady) {
    return EFI_NOT_READY;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  CopyMem (State, &Dev->PointerState, sizeof (*State));

  //
  // Clear mouse state
  //
  Dev->PointerState.RelativeMovementX = 0;
  Dev->PointerState.RelativeMovementY = 0;
  Dev->PointerReady                   = FALSE;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_POINTER_PROTOCOL WaitForInput event handler
STATIC
VOID
EFIAPI
VirtioMouseWaitForPointer (
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
  if (Dev->PointerReady) {
    gBS->SignalEvent (Event);
  }
}

EFI_STATUS
VirtioMouseInit (
  IN OUT VIRTIO_INPUT_DEV  *Dev
  )
{
  EFI_STATUS  Status;

  Dev->SimplePointer.Reset    = VirtioMouseReset;
  Dev->SimplePointer.GetState = VirtioMouseGetState;
  Dev->SimplePointer.Mode     = &Dev->PointerMode;

  Dev->PointerMode.ResolutionX = 8;
  Dev->PointerMode.ResolutionY = 8;
  Dev->PointerMode.LeftButton  = TRUE;
  Dev->PointerMode.RightButton = TRUE;

  ZeroMem (&Dev->PointerState, sizeof (Dev->PointerState));
  Dev->PointerReady = FALSE;

  //
  // Setup the WaitForInput event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  VirtioMouseWaitForPointer,
                  Dev,
                  &Dev->SimplePointer.WaitForInput
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

VOID
VirtioMouseUninit (
  IN OUT VIRTIO_INPUT_DEV  *Dev
  )
{
  gBS->CloseEvent (Dev->SimplePointer.WaitForInput);
}
