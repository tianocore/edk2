/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2017 - 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/PcdLib.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Amd/Msr.h>
#include <Register/Cpuid.h>
#include <Uefi/UefiBaseType.h>
#include <ConfidentialComputingGuestAttr.h>

STATIC UINT64   mSevEncryptionMask      = 0;
STATIC BOOLEAN  mSevEncryptionMaskSaved = FALSE;

/**
  The function check if the specified Attr is set.

  @param[in]  CurrentAttr   The current attribute.
  @param[in]  Attr          The attribute to check.

  @retval  TRUE      The specified Attr is set.
  @retval  FALSE     The specified Attr is not set.

**/
STATIC
BOOLEAN
AmdMemEncryptionAttrCheck (
  IN  UINT64                             CurrentAttr,
  IN  CONFIDENTIAL_COMPUTING_GUEST_ATTR  Attr
  )
{
  switch (Attr) {
    case CCAttrAmdSev:
      //
      // SEV is automatically enabled if SEV-ES or SEV-SNP is active.
      //
      return CurrentAttr >= CCAttrAmdSev;
    case CCAttrAmdSevEs:
      //
      // SEV-ES is automatically enabled if SEV-SNP is active.
      //
      return CurrentAttr >= CCAttrAmdSevEs;
    case CCAttrAmdSevSnp:
      return CurrentAttr == CCAttrAmdSevSnp;
    default:
      return FALSE;
  }
}

/**
  Check if the specified confidential computing attribute is active.

  @param[in]  Attr          The attribute to check.

  @retval TRUE   The specified Attr is active.
  @retval FALSE  The specified Attr is not active.

**/
STATIC
BOOLEAN
EFIAPI
ConfidentialComputingGuestHas (
  IN  CONFIDENTIAL_COMPUTING_GUEST_ATTR  Attr
  )
{
  UINT64  CurrentAttr;

  //
  // Get the current CC attribute.
  //
  CurrentAttr = PcdGet64 (PcdConfidentialComputingGuestAttr);

  //
  // If attr is for the AMD group then call AMD specific checks.
  //
  if (((RShiftU64 (CurrentAttr, 8)) & 0xff) == 1) {
    return AmdMemEncryptionAttrCheck (CurrentAttr, Attr);
  }

  return (CurrentAttr == Attr);
}

/**
  Returns a boolean to indicate whether SEV-SNP is enabled.

  @retval TRUE           SEV-SNP is enabled
  @retval FALSE          SEV-SNP is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevSnpIsEnabled (
  VOID
  )
{
  return ConfidentialComputingGuestHas (CCAttrAmdSevSnp);
}

/**
  Returns a boolean to indicate whether SEV-ES is enabled.

  @retval TRUE           SEV-ES is enabled
  @retval FALSE          SEV-ES is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevEsIsEnabled (
  VOID
  )
{
  return ConfidentialComputingGuestHas (CCAttrAmdSevEs);
}

/**
  Returns a boolean to indicate whether SEV is enabled.

  @retval TRUE           SEV is enabled
  @retval FALSE          SEV is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevIsEnabled (
  VOID
  )
{
  return ConfidentialComputingGuestHas (CCAttrAmdSev);
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
  if (!mSevEncryptionMaskSaved) {
    mSevEncryptionMask      = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask);
    mSevEncryptionMaskSaved = TRUE;
  }

  return mSevEncryptionMask;
}
