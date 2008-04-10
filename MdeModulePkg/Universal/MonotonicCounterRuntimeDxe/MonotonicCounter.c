/** @file
  Produced the Monotonic Counter Services as defined in the DXE CIS.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "MonotonicCounter.h"

//
// The Monotonic Counter Handle
//
EFI_HANDLE  mMonotonicCounterHandle = NULL;

//
// The current Monotonic count value
//
UINT64      mEfiMtc;

//
// Event to use to update the Mtc's high part when wrapping
//
EFI_EVENT   mEfiMtcEvent;

//
// EfiMtcName - Variable name of the MTC value
//
CHAR16      *mEfiMtcName = (CHAR16 *) L"MTC";

//
// EfiMtcGuid - Guid of the MTC value
//
EFI_GUID    mEfiMtcGuid = { 0xeb704011, 0x1402, 0x11d3, { 0x8e, 0x77, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

//
// Worker functions
//
STATIC
EFI_STATUS
EFIAPI
MonotonicCounterDriverGetNextMonotonicCount (
  OUT UINT64  *Count
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  EFI_TPL OldTpl;

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
  //
  // Update the monotonic counter with a lock
  //
  OldTpl  = gBS->RaiseTPL (TPL_HIGH_LEVEL);
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


/**
  Returns the next high 32 bits of the platform's monotonic counter.

  The GetNextHighMonotonicCount() function returns the next high 32 bits
  of the platform's monotonic counter. The platform's monotonic counter is
  comprised of two 32 bit quantities:  the high 32 bits and the low 32 bits.
  During boot service time the low 32 bit value is volatile:  it is reset to
  zero on every system reset and is increased by 1 on every call to GetNextMonotonicCount().
  The high 32 bit value is non-volatile and is increased by 1 whenever the system resets
  or whenever the low 32 bit count [returned by GetNextMonoticCount()] overflows.
  The GetNextMonotonicCount() function is only available at boot services time.
  If the operating system wishes to extend the platform monotonic counter to runtime,
  it may do so by utilizing GetNextHighMonotonicCount().  To do this, before calling
  ExitBootServices() the operating system would call GetNextMonotonicCount() to obtain
  the current platform monotonic count.  The operating system would then provide an
  interface that returns the next count by:
    Adding 1 to the last count.
    Before the lower 32 bits of the count overflows, call GetNextHighMonotonicCount().
    This will increase the high 32 bits of the platform's non-volatile portion of the monotonic
    count by 1.

  This function may only be called at Runtime.

  @param[out]   HighCount	Pointer to returned value.

  @retval EFI_INVALID_PARAMETER If HighCount is NULL.
  @retval EFI_SUCCESS           Operation is successful.
  @retval EFI_OUT_OF_RESOURCES  If variable service reports that not enough storage
                                is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR      The variable could not be saved due to a hardware failure.

**/
STATIC
EFI_STATUS
EFIAPI
MonotonicCounterDriverGetNextHighMonotonicCount (
  OUT UINT32  *HighCount
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  EFI_TPL     OldTpl;

  //
  // Check input parameters
  //
  if (HighCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!EfiAtRuntime ()) {
    //
    // Use a lock if called before ExitBootServices()
    //
    OldTpl      = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    *HighCount  = (UINT32) RShiftU64 (mEfiMtc, 32) + 1;
    mEfiMtc     = LShiftU64 (*HighCount, 32);
    gBS->RestoreTPL (OldTpl);
  } else {
    *HighCount  = (UINT32) RShiftU64 (mEfiMtc, 32) + 1;
    mEfiMtc     = LShiftU64 (*HighCount, 32);
  }
  //
  // Update the NvRam store to match the new high part
  //
  return EfiSetVariable (
           mEfiMtcName,
           &mEfiMtcGuid,
           EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
           sizeof (UINT32),
           HighCount
           );

}

STATIC
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

  MonotonicCounterDriverGetNextHighMonotonicCount (&HighCount);
}

EFI_STATUS
EFIAPI
MonotonicCounterDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

--*/
{
  EFI_STATUS  Status;
  UINT32      HighCount;
  UINTN       BufferSize;

  //
  // Make sure the Monotonic Counter Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiMonotonicCounterArchProtocolGuid);

  //
  // Initialize event to handle overflows
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  EfiMtcEventHandler,
                  NULL,
                  &mEfiMtcEvent
                  );

  ASSERT_EFI_ERROR (Status);

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
  Status = MonotonicCounterDriverGetNextHighMonotonicCount (&HighCount);

  //
  // Fill in the EFI Boot Services and EFI Runtime Services Monotonic Counter Fields
  //
  gBS->GetNextMonotonicCount      = MonotonicCounterDriverGetNextMonotonicCount;
  gRT->GetNextHighMonotonicCount  = MonotonicCounterDriverGetNextHighMonotonicCount;

  //
  // Install the Monotonic Counter Architctural Protocol onto a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mMonotonicCounterHandle,
                  &gEfiMonotonicCounterArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
