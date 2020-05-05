/** @file
  SEV-ES #VC Exception Handler functon.

  Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Register/Amd/Msr.h>
#include "CpuExceptionCommon.h"
#include "AMDSevVcCommon.h"

/**
  #VC exception handling routine.

  Called by the UefiCpuPkg exception handling support when a #VC is encountered.

  @param[in, out] Context  Pointer to EFI_SYSTEM_CONTEXT.

  @retval 0                Exception handled
  @retval Others           New exception value to propagate

**/
UINTN
DoVcException (
  IN OUT EFI_SYSTEM_CONTEXT  Context
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  GHCB                      *Ghcb;

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
  ASSERT (Msr.GhcbInfo.Function == 0);
  ASSERT (Msr.Ghcb != 0);

  Ghcb = Msr.Ghcb;

  return DoVcCommon (Ghcb, Context);
}
