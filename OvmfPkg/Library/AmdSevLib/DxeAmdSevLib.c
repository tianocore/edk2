/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2017 - 2026, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/AmdSevLib.h>
#include <Library/PcdLib.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Amd/Msr.h>
#include <Register/Cpuid.h>
#include <Uefi/UefiBaseType.h>

STATIC BOOLEAN  SevIsEnabled           = FALSE;
STATIC BOOLEAN  SevEsIsEnabled         = FALSE;
STATIC BOOLEAN  SevSnpIsEnabled        = FALSE;
STATIC BOOLEAN  SevDebugVirtualization = FALSE;
STATIC BOOLEAN  SevSnpCoherencySfwNo   = FALSE;

RETURN_STATUS
EFIAPI
DxeAmdSevLibConstructor (
  VOID
  )
{
  SEC_SEV_ES_WORK_AREA     *SevEsWorkArea;
  MSR_SEV_STATUS_REGISTER  Msr;

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
  // and store them in variables so the AmdSev*() functions do not need
  // to access the work area.
  //
  Msr.Uint32             = (UINT32)(UINTN)SevEsWorkArea->SevStatusMsrValue;
  SevIsEnabled           = Msr.Bits.SevBit ? TRUE : FALSE;
  SevEsIsEnabled         = Msr.Bits.SevEsBit ? TRUE : FALSE;
  SevSnpIsEnabled        = Msr.Bits.SevSnpBit ? TRUE : FALSE;
  SevDebugVirtualization = Msr.Bits.DebugVirtualization ? TRUE : FALSE;
  SevSnpCoherencySfwNo   = (SevEsWorkArea->Flags & SEV_ES_WORK_AREA_FLAG_CSFW_NO) != 0;

  return RETURN_SUCCESS;
}

/**
  Returns a boolean to indicate whether SEV-SNP is enabled.

  @retval TRUE           SEV-SNP is enabled
  @retval FALSE          SEV-SNP is not enabled
**/
BOOLEAN
EFIAPI
AmdSevSnpIsEnabled (
  VOID
  )
{
  return SevSnpIsEnabled;
}

/**
  Returns a boolean to indicate whether SEV-ES is enabled.

  @retval TRUE           SEV-ES is enabled
  @retval FALSE          SEV-ES is not enabled
**/
BOOLEAN
EFIAPI
AmdSevEsIsEnabled (
  VOID
  )
{
  return SevEsIsEnabled;
}

/**
  Returns a boolean to indicate whether SEV is enabled.

  @retval TRUE           SEV is enabled
  @retval FALSE          SEV is not enabled
**/
BOOLEAN
EFIAPI
AmdSevIsEnabled (
  VOID
  )
{
  return SevIsEnabled;
}

/**
  Returns a boolean to indicate whether DebugVirtualization is enabled.

  @retval TRUE           DebugVirtualization is enabled
  @retval FALSE          DebugVirtualization is not enabled
**/
BOOLEAN
EFIAPI
AmdSevEsDebugVirtualizationIsEnabled (
  VOID
  )
{
  return SevDebugVirtualization;
}

/**
  Returns a boolean to indicate whether the SEV-SNP cache line eviction
  mitigation is needed.

  @retval TRUE           Cache line eviction mitigation required
  @retval FALSE          Cache line eviction migigation not required

**/
BOOLEAN
EFIAPI
AmdSevSnpDoCoherencyMitigation (
  VOID
  )
{
  return AmdSevSnpIsEnabled () && !SevSnpCoherencySfwNo;
}
