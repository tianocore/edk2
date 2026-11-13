/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2020 - 2026, AMD Incorporated. All rights reserved.<BR>

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

/**
  Read the SEV Status MSR value from the workarea

  **/
STATIC
UINT32
EFIAPI
InternalAmdSevStatus (
  VOID
  )
{
  SEC_SEV_ES_WORK_AREA  *SevEsWorkArea;

  SevEsWorkArea = AmdSevGetSevEsWorkArea ();
  if (SevEsWorkArea == NULL) {
    return 0;
  }

  return (UINT32)(UINTN)SevEsWorkArea->SevStatusMsrValue;
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
  MSR_SEV_STATUS_REGISTER  Msr;

  Msr.Uint32 = InternalAmdSevStatus ();

  return Msr.Bits.SevSnpBit ? TRUE : FALSE;
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
  MSR_SEV_STATUS_REGISTER  Msr;

  Msr.Uint32 = InternalAmdSevStatus ();

  return Msr.Bits.SevEsBit ? TRUE : FALSE;
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
  MSR_SEV_STATUS_REGISTER  Msr;

  Msr.Uint32 = InternalAmdSevStatus ();

  return Msr.Bits.SevBit ? TRUE : FALSE;
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
  MSR_SEV_STATUS_REGISTER  Msr;

  Msr.Uint32 = InternalAmdSevStatus ();

  return Msr.Bits.DebugVirtualization ? TRUE : FALSE;
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
  SEC_SEV_ES_WORK_AREA  *SevEsWorkArea;

  SevEsWorkArea = AmdSevGetSevEsWorkArea ();
  if (SevEsWorkArea == NULL) {
    return FALSE;
  }

  return AmdSevSnpIsEnabled () &&
         ((SevEsWorkArea->Flags & SEV_ES_WORK_AREA_FLAG_CSFW_NO) == 0);
}

/**
  Returns a boolean to indicate whether Alternate Injection is enabled.

  @retval TRUE           AlternateInjection is enabled
  @retval FALSE          AlternateInjection is not enabled
**/
BOOLEAN
EFIAPI
AmdSevSnpAlternateInjectionIsEnabled (
  VOID
  )
{
  MSR_SEV_STATUS_REGISTER  Msr;

  Msr.Uint32 = InternalAmdSevStatus ();
  return Msr.Bits.AlternateInjection ? TRUE : FALSE;
}
