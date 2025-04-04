/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2017 - 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/HobLib.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Amd/Msr.h>
#include <Register/Cpuid.h>
#include <Guid/OvmfMemoryEncryptionConfig.h>
#include <ConfidentialComputingGuestAttr.h>

STATIC UINT64   mCurrentAttr            = 0;
STATIC BOOLEAN  mCurrentAttrRead        = FALSE;
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
  UINT64  CurrentLevel;

  CurrentLevel = CurrentAttr & CCAttrTypeMask;

  switch (Attr) {
    case CCAttrAmdSev:
      //
      // SEV is automatically enabled if SEV-ES or SEV-SNP is active.
      //
      return CurrentLevel >= CCAttrAmdSev;
    case CCAttrAmdSevEs:
      //
      // SEV-ES is automatically enabled if SEV-SNP is active.
      //
      return CurrentLevel >= CCAttrAmdSevEs;
    case CCAttrAmdSevSnp:
      return CurrentLevel == CCAttrAmdSevSnp;
    case CCAttrFeatureAmdSevEsDebugVirtualization:
      return !!(CurrentAttr & CCAttrFeatureAmdSevEsDebugVirtualization);
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
  OVMF_MEMORY_ENCRYPTION_CONFIG  *Hob;

  //
  // Get the current CC attribute.
  //
  // We avoid reading the PCD on every check because this routine could be indirectly
  // called during the virtual pointer conversion. And its not safe to access the
  // PCDs during the virtual pointer conversion.
  //
  if (!mCurrentAttrRead) {
    // Fetch the hobs to get the current CC attribute.
    Hob = GetFirstGuidHob (&gOvmfMemoryEncryptionConfigGuid);
    if (Hob == NULL) {
      ASSERT (FALSE);
      // If the hob is not found, then set the current attribute to 0.
      mCurrentAttr = 0;
      return FALSE;
    }

    // If the hob is found, then set the current attribute to the value in the hob.
    mCurrentAttr     = Hob->ConfidentialComputingGuestAttr;
    mCurrentAttrRead = TRUE;
  }

  //
  // If attr is for the AMD group then call AMD specific checks.
  //
  if (((RShiftU64 (mCurrentAttr, 8)) & 0xff) == 1) {
    return AmdMemEncryptionAttrCheck (mCurrentAttr, Attr);
  }

  return (mCurrentAttr == Attr);
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
  OVMF_MEMORY_ENCRYPTION_CONFIG  *Hob;

  if (!mSevEncryptionMaskSaved) {
    // Fetch the hobs to get the current SEV encryption mask.
    Hob = GetFirstGuidHob (&gOvmfMemoryEncryptionConfigGuid);
    if (Hob == NULL) {
      ASSERT (FALSE);
      // If the hob is not found, then set the SEV encryption mask to 0.
      mSevEncryptionMask = 0;
      return mSevEncryptionMask;
    }

    // If the hob is found, then set the SEV encryption mask to the value in the hob.
    mSevEncryptionMask      = Hob->PteMemoryEncryptionAddressOrMask;
    mSevEncryptionMaskSaved = TRUE;
  }

  return mSevEncryptionMask;
}

/**
  Returns a boolean to indicate whether DebugVirtualization is enabled.

  @retval TRUE           DebugVirtualization is enabled
  @retval FALSE          DebugVirtualization is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevEsDebugVirtualizationIsEnabled (
  VOID
  )
{
  return ConfidentialComputingGuestHas (CCAttrFeatureAmdSevEsDebugVirtualization);
}
