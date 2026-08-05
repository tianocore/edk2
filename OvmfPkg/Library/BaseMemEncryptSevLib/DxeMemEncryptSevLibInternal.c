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
#include "PeiDxeMemEncryptSevLibInternal.h"

STATIC UINT64   mSevEncryptionMask      = 0;
STATIC BOOLEAN  mSevIsEnabled           = FALSE;
STATIC BOOLEAN  mSevEsIsEnabled         = FALSE;
STATIC BOOLEAN  mSevSnpIsEnabled        = FALSE;
STATIC BOOLEAN  mSevDebugVirtualization = FALSE;
STATIC BOOLEAN  mSevSnpCoherencySfwNo   = FALSE;

RETURN_STATUS
EFIAPI
DxeMemEncryptSevLibConstructor (
  VOID
  )
{
  SEC_SEV_ES_WORK_AREA     *SevEsWorkArea;
  MSR_SEV_STATUS_REGISTER  Msr;

  //
  // The work area should at least be EfiBootServicesData since the work area
  // is also used for AP bring up to setup the AP jump table.
  //
  SevEsWorkArea = GetSevEsWorkArea ();
  if (SevEsWorkArea == NULL) {
    return RETURN_SUCCESS;
  }

  //
  // The work area will not be mapped at the expected physical address once
  // SetVirtualAddressMap() is called. So fetch the values from the work area
  // and store them in variables so the MemEncryptSev*() functions do not need
  // to access the work area.
  //
  Msr.Uint32              = (UINT32)(UINTN)SevEsWorkArea->SevStatusMsrValue;
  mSevEncryptionMask      = SevEsWorkArea->EncryptionMask;
  mSevIsEnabled           = Msr.Bits.SevBit ? TRUE : FALSE;
  mSevEsIsEnabled         = Msr.Bits.SevEsBit ? TRUE : FALSE;
  mSevSnpIsEnabled        = Msr.Bits.SevSnpBit ? TRUE : FALSE;
  mSevDebugVirtualization = Msr.Bits.DebugVirtualization ? TRUE : FALSE;
  mSevSnpCoherencySfwNo   = (SevEsWorkArea->Flags & SEV_ES_WORK_AREA_FLAG_CSFW_NO) != 0;

  return RETURN_SUCCESS;
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
  return mSevSnpIsEnabled;
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
  return mSevEsIsEnabled;
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
  return mSevIsEnabled;
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
  return mSevDebugVirtualization;
}

/**
  Returns a boolean to indicate whether the SEV-SNP cache line eviction
  mitigation is needed.

  @retval TRUE           Cache line eviction mitigation required
  @retval FALSE          Cache line eviction migigation not required

**/
BOOLEAN
EFIAPI
MemEncryptSevSnpDoCoherencyMitigation (
  VOID
  )
{
  return MemEncryptSevSnpIsEnabled () && !mSevSnpCoherencySfwNo;
}
