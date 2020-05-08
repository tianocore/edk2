/** @file
  VMGEXIT Base Support Library.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/VmgExitLib.h>

/**
  Create a VMGEXIT error representing a GP_EXCEPTION to be propogated.

  @retval  Other  Exception event representing the exception to be propagated.

**/
STATIC
UINT64
VmgExitError (
  VOID
  )
{
  GHCB_EVENT_INJECTION  Event;

  Event.Uint64 = 0;
  Event.Elements.Vector = GP_EXCEPTION;
  Event.Elements.Type   = GHCB_EVENT_INJECTION_TYPE_EXCEPTION;
  Event.Elements.Valid  = 1;

  return Event.Uint64;
}

/**
  Perform VMGEXIT.

  Sets the necessary fields of the GHCB, invokes the VMGEXIT instruction and
  then handles the return actions.

  The base library function returns an error in the form of a
  GHCB_EVENT_INJECTION representing a GP_EXCEPTION.

  @param[in, out]  Ghcb       A pointer to the GHCB
  @param[in]       ExitCode   VMGEXIT code to be assigned to the SwExitCode
                              field of the GHCB.
  @param[in]       ExitInfo1  VMGEXIT information to be assigned to the
                              SwExitInfo1 field of the GHCB.
  @param[in]       ExitInfo2  VMGEXIT information to be assigned to the
                              SwExitInfo2 field of the GHCB.

  @retval  0                  VMGEXIT succeeded.
  @retval  Others             VMGEXIT processing did not succeed. Exception
                              event to be propagated.

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
  return VmgExitError ();
}

/**
  Perform pre-VMGEXIT initialization/preparation.

  Performs the necessary steps in preparation for invoking VMGEXIT. Must be
  called before setting any fields within the GHCB.

  The base library function does nothing.

  @param[in, out]  Ghcb       A pointer to the GHCB

**/
VOID
EFIAPI
VmgInit (
  IN OUT GHCB                *Ghcb
  )
{
}

/**
  Perform post-VMGEXIT cleanup.

  Performs the necessary steps to cleanup after invoking VMGEXIT. Must be
  called after obtaining needed fields within the GHCB.

  The base library function does nothing.

  @param[in, out]  Ghcb       A pointer to the GHCB

**/
VOID
EFIAPI
VmgDone (
  IN OUT GHCB                *Ghcb
  )
{
}

/**
  Perform MMIO write of a buffer to a non-MMIO marked range.

  Performs an MMIO write without taking a #VC. This is useful
  for Flash devices, which are marked read-only.

  The base library function does nothing.

  @param[in, out]  Dest       A pointer to the destination buffer
  @param[in]       Src        A pointer to the source data to be written
  @param[in]       Bytes      Number of bytes to write

**/
VOID
EFIAPI
VmgMmioWrite (
  IN OUT UINT8               *Dest,
  IN     UINT8               *Src,
  IN     UINTN                Bytes
  )
{
}

/**
  Issue the GHCB set AP Jump Table VMGEXIT.

  Performs a VMGEXIT using the GHCB AP Jump Table exit code to save the
  AP Jump Table address with the hypervisor for retrieval at a later time.

  The base library function returns an error in the form of a
  GHCB_EVENT_INJECTION representing a GP_EXCEPTION.

  @param[in]  Address  Physical address of the AP Jump Table

  @retval  0           VMGEXIT succeeded.
  @retval  Others      VMGEXIT processing did not succeed. Exception
                       number to be propagated.

**/
UINT64
EFIAPI
VmgExitSetAPJumpTable (
  IN EFI_PHYSICAL_ADDRESS  Address
  )
{
  return VmgExitError ();
}

/**
  Handle a #VC exception.

  Performs the necessary processing to handle a #VC exception.

  The base library function returns an error equal to VC_EXCEPTION,
  to be propagated to the standard exception handling stack.

  @param[in, out]  SystemContext  Pointer to EFI_SYSTEM_CONTEXT

  @retval  0                      Exception handled
  @retval  Others                 New exception value to propagate

**/
UINTN
VmgExitHandleVc (
  IN OUT EFI_SYSTEM_CONTEXT  Context
  )
{
  return VC_EXCEPTION;
}
