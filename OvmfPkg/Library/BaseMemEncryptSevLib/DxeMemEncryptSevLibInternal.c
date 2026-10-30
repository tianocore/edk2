/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2017 - 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/AmdSevLib.h>
#include <Library/PcdLib.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Amd/Msr.h>
#include <Register/Cpuid.h>
#include <Uefi/UefiBaseType.h>

STATIC UINT64  mSevEncryptionMask = 0;

RETURN_STATUS
EFIAPI
DxeMemEncryptSevLibConstructor (
  VOID
  )
{
  SEC_SEV_ES_WORK_AREA  *SevEsWorkArea;

  //
  // The work area should at least be EfiBootServicesData since the work area
  // is also used for AP bring up to setup the AP jump table.
  //
  SevEsWorkArea = AmdSevGetSevEsWorkArea ();
  if (SevEsWorkArea == NULL) {
    return RETURN_SUCCESS;
  }

  //
  // The work area will not be mapped at the expected physical address once
  // SetVirtualAddressMap() is called. So fetch the values from the work area
  // and store them in variables so the MemEncryptSev*() functions do not need
  // to access the work area.
  //
  mSevEncryptionMask = SevEsWorkArea->EncryptionMask;

  return RETURN_SUCCESS;
}

/**
  Returns the SEV encryption mask.

  @return  The SEV pagtable encryption mask
**/
UINT64
EFIAPI
MemEncryptSevGetEncryptionMask (
  VOID
  )
{
  return mSevEncryptionMask;
}
