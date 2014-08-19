/** @file
  Generic Monotonic Counter services

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/


//
// The current Monotonic count value
//
UINT64      mEfiMtc = 0;


//
// Event to use to update the Mtc's high part when wrapping
//
EFI_EVENT   mEfiMtcEvent;

//
// EfiMtcName - Variable name of the MTC value
//
CHAR16      *mEfiMtcName = L"MTC";

//
// EfiMtcGuid - Guid of the MTC value
//
EFI_GUID    mEfiMtcGuid = { 0xeb704011, 0x1402, 0x11d3, { 0x8e, 0x77, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };



//
// Worker functions
//


VOID
EFIAPI
EfiMtcEventHandler (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
/*++

Routine Description:

  Monotonic count event handler.  This handler updates the high monotonic count.

Arguments:

  Event         The event to handle
  Context       The event context

Returns:

  EFI_SUCCESS       The event has been handled properly
  EFI_NOT_FOUND     An error occurred updating the variable.

--*/
{
  UINT32  HighCount;

  EfiGetNextHighMonotonicCount (&HighCount);
  return;
}



VOID
LibMtcVirtualAddressChangeEvent (VOID)
{
}


EFI_STATUS
EFIAPI
LibMtcGetNextHighMonotonicCount (
  OUT UINT32  *HighCount
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  //
  // Check input parameters
  //
  if (HighCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  if (!EfiAtRuntime ()) {
    // Use a lock if called before ExitBootServices()
    OldTpl      = gBS->RaiseTPL (EFI_TPL_HIGH_LEVEL);
  }

  *HighCount  = (UINT32) RShiftU64 (mEfiMtc, 32) + 1;
  mEfiMtc     = LShiftU64 (*HighCount, 32);

  if (!EfiAtRuntime ()) {
    gBS->RestoreTPL (OldTpl);
  }

  //
  // Update the NvRam store to match the new high part
  //
  Status = EfiSetVariable (
              mEfiMtcName,
              &mEfiMtcGuid,
              EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
              sizeof (UINT32),
              HighCount
              );

  return Status;
}


EFI_STATUS
LibMtcGetNextMonotonicCount (
  OUT UINT64  *Count
  )
{
  EFI_STATUS    Status;
  EFI_TPL       OldTpl;
  UINT32        HighCount;
  UINTN         BufferSize;

  //
  // Can not be called after ExitBootServices()
  //
  if (EfiAtRuntime ()) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check input parameters
  //
  if (Count == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mEfiMtc == 0) {
    //
    // If the MTC has not been initialized read the variable
    //

    //
    // Read the last high part
    //
    BufferSize = sizeof (UINT32);
    Status = EfiGetVariable (
                mEfiMtcName,
                &mEfiMtcGuid,
                NULL,
                &BufferSize,
                &HighCount
                );
    if (EFI_ERROR (Status)) {
      HighCount = 0;
    }

    //
    // Set the current value
    //
    mEfiMtc = LShiftU64 (HighCount, 32);
    //
    // Increment the upper 32 bits for this boot
    // Continue even if it fails.  It will only fail if the variable services are
    // not functional.
    //
    Status = EfiGetNextHighMonotonicCount (&HighCount);
  }


  //
  // Update the monotonic counter with a lock
  //
  OldTpl  = gBS->RaiseTPL (EFI_TPL_HIGH_LEVEL);
  *Count  = mEfiMtc;
  mEfiMtc++;
  gBS->RestoreTPL (OldTpl);

  //
  // If the MSB bit of the low part toggled, then signal that the high
  // part needs updated now
  //
  if ((((UINT32) mEfiMtc) ^ ((UINT32) *Count)) & 0x80000000) {
    gBS->SignalEvent (mEfiMtcEvent);
  }

  return EFI_SUCCESS;
}



VOID
LibMtcInitialize (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Initialize event to handle overflows
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  EFI_TPL_CALLBACK,
                  EfiMtcEventHandler,
                  NULL,
                  &mEfiMtcEvent
                  );
  ASSERT_EFI_ERROR (Status);
}

