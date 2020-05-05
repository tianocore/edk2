/** @file
  IA32 SEV-ES #VC Exception Handler functons.

  Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include "AMDSevVcCommon.h"

/**
  Common #VC exception handling routine.

  Used to bridge different phases of UEFI execution.

  @param[in, out] Ghcb     Pointer to the Guest-Hypervisor Communication Block
  @param[in, out] Context  Pointer to EFI_SYSTEM_CONTEXT.

  @retval 0                Exception handled
  @retval Others           New exception value to propagate

**/
UINTN
DoVcCommon (
  IN OUT GHCB                *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT  Context
  )
{
  EFI_SYSTEM_CONTEXT_IA32  *Regs;

  Regs = Context.SystemContextIa32;

  Regs->ExceptionData = 0;

  return GP_EXCEPTION;
}
