/** @file
  X64 #VC Exception Handler functon.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/VmgExitLib.h>
#include <Register/Amd/Msr.h>

/**
  #VC exception handling routine.

  Called by the UefiCpuPkg exception handling support when a #VC is encountered.

  This base library handler will always request propagation of a GP_EXCEPTION.

  @param[in, out] Context  Pointer to EFI_SYSTEM_CONTEXT.

  @retval 0                Exception handled
  @retval Others           New exception value to propagate

**/
UINTN
VmgExitHandleVc (
  IN OUT EFI_SYSTEM_CONTEXT  Context
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  EFI_SYSTEM_CONTEXT_X64    *Regs;
  GHCB                      *Ghcb;
  UINT64                    Status;
  UINTN                     ExitCode, VcRet;

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
  ASSERT (Msr.GhcbInfo.Function == 0);
  ASSERT (Msr.Ghcb != 0);

  Regs = Context.SystemContextX64;
  Ghcb = Msr.Ghcb;

  VmgInit (Ghcb);

  ExitCode = Regs->ExceptionData;
  switch (ExitCode) {
  default:
    Status = VmgExit (Ghcb, SVM_EXIT_UNSUPPORTED, ExitCode, 0);
    if (Status == 0) {
      Regs->ExceptionData = 0;
      VcRet = GP_EXCEPTION;
    } else {
      GHCB_EVENT_INJECTION  Event;

      Event.Uint64 = Status;
      if (Event.Elements.ErrorCodeValid) {
        Regs->ExceptionData = Event.Elements.ErrorCode;
      } else {
        Regs->ExceptionData = 0;
      }

      VcRet = Event.Elements.Vector;
    }
  }

  VmgDone (Ghcb);

  return VcRet;
}
