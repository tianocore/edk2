/** @file

  SEV-SNP Page Validation functions.

  Copyright (c) 2021 - 2024, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/AmdSvsmLib.h>

#include "SnpPageStateChange.h"

//
// The variable used for the VMPL check.
//
STATIC UINT8  gVmpl0Data[4096];

/**
 The function checks whether SEV-SNP guest is booted under VMPL0.

 @retval  TRUE      The guest is booted under VMPL0
 @retval  FALSE     The guest is not booted under VMPL0
 **/
STATIC
BOOLEAN
SevSnpIsVmpl0 (
  VOID
  )
{
  UINT64  Rdx;
  UINT32  Status;

  //
  // There is no straightforward way to query the current VMPL level.
  // The simplest method is to use the RMPADJUST instruction to change
  // a page permission to a VMPL level-1, and if the guest kernel is
  // launched at a level <= 1, then RMPADJUST instruction will return
  // an error.
  //
  Rdx = 1;

  Status = AsmRmpAdjust ((UINT64)gVmpl0Data, 0, Rdx);
  if (Status != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Pre-validate the system RAM when SEV-SNP is enabled in the guest VM.

  @param[in]  BaseAddress             Base address
  @param[in]  NumPages                Number of pages starting from the base address

**/
VOID
EFIAPI
MemEncryptSevSnpPreValidateSystemRam (
  IN PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN             NumPages
  )
{
  SEC_SEV_ES_WORK_AREA  *SevEsWorkArea;

  if (!MemEncryptSevSnpIsEnabled ()) {
    return;
  }

  //
  // The page state change uses the PVALIDATE instruction. The instruction
  // can be run at VMPL-0 only. If its not a VMPL-0 guest, then an SVSM must
  // be present to perform the operation on behalf of the guest. If the guest
  // is not running at VMPL-0 and an SVSM is not present, then terminate the
  // boot.
  //
  if (!SevSnpIsVmpl0 () && !AmdSvsmIsSvsmPresent ()) {
    SnpPageStateFailureTerminate ();
  }

  SevEsWorkArea = (SEC_SEV_ES_WORK_AREA *)FixedPcdGet32 (PcdSevEsWorkAreaBase);

  InternalSetPageState (
    BaseAddress,
    NumPages,
    SevSnpPagePrivate,
    TRUE,
    SevEsWorkArea->WorkBuffer,
    sizeof (SevEsWorkArea->WorkBuffer)
    );
}
