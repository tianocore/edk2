/** @file
  X64 #VC Exception Handler functon header file.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CC_EXIT_VC_HANDLER_H_
#define CC_EXIT_VC_HANDLER_H_

#include <Base.h>
#include <Uefi.h>
#include <Library/CcExitLib.h>

/**
  Handle a #VC exception.

  Performs the necessary processing to handle a #VC exception.

  @param[in, out]  Ghcb           Pointer to the GHCB
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
InternalVmgExitHandleVc (
  IN OUT GHCB                *Ghcb,
  IN OUT EFI_EXCEPTION_TYPE  *ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
  Routine to allow ASSERT from within #VC.

  @param[in, out]  SevEsData  Pointer to the per-CPU data

**/
VOID
EFIAPI
VmgExitIssueAssert (
  IN OUT SEV_ES_PER_CPU_DATA  *SevEsData
  );

#endif
