/** @file

  SEV-SNP Page Validation functions.

  Copyright (c) 2021 AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>

#include "SnpPageStateChange.h"
#include "VirtualMemory.h"

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
  if (!MemEncryptSevSnpIsEnabled ()) {
    return;
  }

  //
  // All the pre-validation must be completed in the PEI phase.
  //
  ASSERT (FALSE);
}

/**
  Accept pages system RAM when SEV-SNP is enabled in the guest VM.

  @param[in]  BaseAddress             Base address
  @param[in]  NumPages                Number of pages starting from the base address

**/
VOID
EFIAPI
MemEncryptSnpAcceptPages (
  IN PHYSICAL_ADDRESS           BaseAddress,
  IN UINTN                      NumPages
  )
{
  EFI_STATUS Status;

  if (!MemEncryptSevSnpIsEnabled ()) {
    return;
  }
  if (BaseAddress >= SIZE_4GB) {
    Status = InternalMemEncryptSevCreateIdentityMap1G (
               0,
               BaseAddress,
               EFI_PAGES_TO_SIZE (NumPages)
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      CpuDeadLoop ();
    }
  }

  InternalSetPageState (BaseAddress, NumPages, SevSnpPagePrivate, TRUE);
}
