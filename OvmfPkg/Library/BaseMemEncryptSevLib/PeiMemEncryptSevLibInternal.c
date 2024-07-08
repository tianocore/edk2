/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2020, AMD Incorporated. All rights reserved.<BR>

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

/**
   Read the workarea to determine whether SEV is enabled. If enabled,
   then return the SevEsWorkArea pointer.

  **/
STATIC
SEC_SEV_ES_WORK_AREA *
EFIAPI
GetSevEsWorkArea (
  VOID
  )
{
  OVMF_WORK_AREA  *WorkArea;

  WorkArea = (OVMF_WORK_AREA *)FixedPcdGet32 (PcdOvmfWorkAreaBase);

  //
  // If its not SEV guest then SevEsWorkArea is not valid.
  //
  if ((WorkArea == NULL) || (WorkArea->Header.GuestType != CcGuestTypeAmdSev)) {
    return NULL;
  }

  return (SEC_SEV_ES_WORK_AREA *)FixedPcdGet32 (PcdSevEsWorkAreaBase);
}

/**
  Read the SEV Status MSR value from the workarea

  **/
STATIC
UINT32
EFIAPI
InternalMemEncryptSevStatus (
  VOID
  )
{
  SEC_SEV_ES_WORK_AREA  *SevEsWorkArea;

  SevEsWorkArea = GetSevEsWorkArea ();
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
MemEncryptSevSnpIsEnabled (
  VOID
  )
{
  MSR_SEV_STATUS_REGISTER  Msr;

  Msr.Uint32 = InternalMemEncryptSevStatus ();

  return Msr.Bits.SevSnpBit ? TRUE : FALSE;
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
  MSR_SEV_STATUS_REGISTER  Msr;

  Msr.Uint32 = InternalMemEncryptSevStatus ();

  return Msr.Bits.SevEsBit ? TRUE : FALSE;
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
  MSR_SEV_STATUS_REGISTER  Msr;

  Msr.Uint32 = InternalMemEncryptSevStatus ();

  return Msr.Bits.SevBit ? TRUE : FALSE;
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
  SEC_SEV_ES_WORK_AREA  *SevEsWorkArea;

  SevEsWorkArea = GetSevEsWorkArea ();
  if (SevEsWorkArea == NULL) {
    return 0;
  }

  return SevEsWorkArea->EncryptionMask;
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
  MSR_SEV_STATUS_REGISTER  Msr;

  Msr.Uint32 = InternalMemEncryptSevStatus ();

  return Msr.Bits.DebugVirtualization ? TRUE : FALSE;
}
