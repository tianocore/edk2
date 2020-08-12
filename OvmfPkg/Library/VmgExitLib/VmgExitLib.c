/** @file
  VMGEXIT Support Library.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/VmgExitLib.h>
#include <Register/Amd/Msr.h>

/**
  Check for VMGEXIT error

  Check if the hypervisor has returned an error after completion of the VMGEXIT
  by examining the SwExitInfo1 field of the GHCB.

  @param[in]  Ghcb       A pointer to the GHCB

  @retval  0             VMGEXIT succeeded.
  @return                Exception number to be propagated, VMGEXIT processing
                         did not succeed.

**/
STATIC
UINT64
VmgExitErrorCheck (
  IN GHCB                *Ghcb
  )
{
  GHCB_EVENT_INJECTION  Event;
  GHCB_EXIT_INFO        ExitInfo;
  UINT64                Status;

  ExitInfo.Uint64 = Ghcb->SaveArea.SwExitInfo1;
  ASSERT ((ExitInfo.Elements.Lower32Bits == 0) ||
          (ExitInfo.Elements.Lower32Bits == 1));

  Status = 0;
  if (ExitInfo.Elements.Lower32Bits == 0) {
    return Status;
  }

  if (ExitInfo.Elements.Lower32Bits == 1) {
    ASSERT (Ghcb->SaveArea.SwExitInfo2 != 0);

    //
    // Check that the return event is valid
    //
    Event.Uint64 = Ghcb->SaveArea.SwExitInfo2;
    if (Event.Elements.Valid &&
        Event.Elements.Type == GHCB_EVENT_INJECTION_TYPE_EXCEPTION) {
      switch (Event.Elements.Vector) {
      case GP_EXCEPTION:
      case UD_EXCEPTION:
        //
        // Use returned event as return code
        //
        Status = Event.Uint64;
      }
    }
  }

  if (Status == 0) {
    GHCB_EVENT_INJECTION  GpEvent;

    GpEvent.Uint64 = 0;
    GpEvent.Elements.Vector = GP_EXCEPTION;
    GpEvent.Elements.Type   = GHCB_EVENT_INJECTION_TYPE_EXCEPTION;
    GpEvent.Elements.Valid  = 1;

    Status = GpEvent.Uint64;
  }

  return Status;
}

/**
  Perform VMGEXIT.

  Sets the necessary fields of the GHCB, invokes the VMGEXIT instruction and
  then handles the return actions.

  @param[in, out]  Ghcb       A pointer to the GHCB
  @param[in]       ExitCode   VMGEXIT code to be assigned to the SwExitCode
                              field of the GHCB.
  @param[in]       ExitInfo1  VMGEXIT information to be assigned to the
                              SwExitInfo1 field of the GHCB.
  @param[in]       ExitInfo2  VMGEXIT information to be assigned to the
                              SwExitInfo2 field of the GHCB.

  @retval  0                  VMGEXIT succeeded.
  @return                     Exception number to be propagated, VMGEXIT
                              processing did not succeed.

**/
UINT64
EFIAPI
VmgExit (
  IN OUT GHCB                *Ghcb,
  IN     UINT64              ExitCode,
  IN     UINT64              ExitInfo1,
  IN     UINT64              ExitInfo2
  )
{
  Ghcb->SaveArea.SwExitCode = ExitCode;
  Ghcb->SaveArea.SwExitInfo1 = ExitInfo1;
  Ghcb->SaveArea.SwExitInfo2 = ExitInfo2;

  //
  // Guest memory is used for the guest-hypervisor communication, so fence
  // the invocation of the VMGEXIT instruction to ensure GHCB accesses are
  // synchronized properly.
  //
  MemoryFence ();
  AsmVmgExit ();
  MemoryFence ();

  return VmgExitErrorCheck (Ghcb);
}

/**
  Perform pre-VMGEXIT initialization/preparation.

  Performs the necessary steps in preparation for invoking VMGEXIT. Must be
  called before setting any fields within the GHCB.

  @param[in, out]  Ghcb       A pointer to the GHCB

**/
VOID
EFIAPI
VmgInit (
  IN OUT GHCB                *Ghcb
  )
{
  SetMem (&Ghcb->SaveArea, sizeof (Ghcb->SaveArea), 0);
}

/**
  Perform post-VMGEXIT cleanup.

  Performs the necessary steps to cleanup after invoking VMGEXIT. Must be
  called after obtaining needed fields within the GHCB.

  @param[in, out]  Ghcb       A pointer to the GHCB

**/
VOID
EFIAPI
VmgDone (
  IN OUT GHCB                *Ghcb
  )
{
}

