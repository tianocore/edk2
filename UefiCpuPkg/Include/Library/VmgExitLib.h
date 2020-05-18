/** @file
  Public header file for the VMGEXIT Support library class.

  This library class defines some routines used when invoking the VMGEXIT
  instruction in support of SEV-ES and to handle #VC exceptions.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VMG_EXIT_LIB_H__
#define __VMG_EXIT_LIB_H__

#include <Protocol/DebugSupport.h>
#include <Register/Amd/Ghcb.h>


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
  @retval  Others             VMGEXIT processing did not succeed. Exception
                              number to be propagated.

**/
UINT64
EFIAPI
VmgExit (
  IN OUT GHCB                *Ghcb,
  IN     UINT64              ExitCode,
  IN     UINT64              ExitInfo1,
  IN     UINT64              ExitInfo2
  );

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
  );

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
  );

/**
  Handle a #VC exception.

  Performs the necessary processing to handle a #VC exception.

  The base library function returns an error equal to VC_EXCEPTION,
  to be propagated to the standard exception handling stack.

  @param[in, out]  ExceptionType  Pointer to an EFI_EXCEPTION_TYPE to be set
                                  as value to use on error.
  @param[in, out]  SystemContext  Pointer to EFI_SYSTEM_CONTEXT

  @retval  EFI_SUCCESS            Exception handled
  @retval  EFI_UNSUPPORTED        #VC not supported, (new) exception value to
                                  propagate provided
  @retval  EFI_PROTOCOL_ERROR     #VC handling failed, (new) exception value to
                                  propagate provided

**/
EFI_STATUS
EFIAPI
VmgExitHandleVc (
  IN OUT EFI_EXCEPTION_TYPE  *ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  );

#endif
