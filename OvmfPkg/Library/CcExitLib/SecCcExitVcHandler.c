/** @file
  X64 #VC Exception Handler functon.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/CcExitLib.h>
#include <Register/Amd/Msr.h>

#include "CcExitVcHandler.h"

/**
  Handle a #VC exception.

  Performs the necessary processing to handle a #VC exception.

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
CcExitHandleVc (
  IN OUT EFI_EXCEPTION_TYPE  *ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  GHCB                      *Ghcb;
  GHCB                      *GhcbBackup;
  EFI_STATUS                VcRet;
  BOOLEAN                   InterruptState;
  SEV_ES_PER_CPU_DATA       *SevEsData;

  InterruptState = GetInterruptState ();
  if (InterruptState) {
    DisableInterrupts ();
  }

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
  ASSERT (Msr.GhcbInfo.Function == 0);
  ASSERT (Msr.Ghcb != 0);

  Ghcb       = Msr.Ghcb;
  GhcbBackup = NULL;

  SevEsData = (SEV_ES_PER_CPU_DATA *)(Ghcb + 1);
  SevEsData->VcCount++;

  //
  // Check for maximum SEC #VC nesting.
  //
  if (SevEsData->VcCount > VMGEXIT_MAXIMUM_VC_COUNT) {
    VmgExitIssueAssert (SevEsData);
  } else if (SevEsData->VcCount > 1) {
    UINTN  GhcbBackupSize;

    //
    // Be sure that the proper amount of pages are allocated
    //
    GhcbBackupSize = (VMGEXIT_MAXIMUM_VC_COUNT - 1) * sizeof (*Ghcb);
    if (GhcbBackupSize > FixedPcdGet32 (PcdOvmfSecGhcbBackupSize)) {
      //
      // Not enough SEC backup pages allocated.
      //
      VmgExitIssueAssert (SevEsData);
    }

    //
    // Save the active GHCB to a backup page.
    //   To access the correct backup page, increment the backup page pointer
    //   based on the current VcCount.
    //
    GhcbBackup  = (GHCB *)FixedPcdGet32 (PcdOvmfSecGhcbBackupBase);
    GhcbBackup += (SevEsData->VcCount - 2);

    CopyMem (GhcbBackup, Ghcb, sizeof (*Ghcb));
  }

  VcRet = InternalVmgExitHandleVc (Ghcb, ExceptionType, SystemContext);

  if (GhcbBackup != NULL) {
    //
    // Restore the active GHCB from the backup page.
    //
    CopyMem (Ghcb, GhcbBackup, sizeof (*Ghcb));
  }

  SevEsData->VcCount--;

  if (InterruptState) {
    EnableInterrupts ();
  }

  return VcRet;
}
