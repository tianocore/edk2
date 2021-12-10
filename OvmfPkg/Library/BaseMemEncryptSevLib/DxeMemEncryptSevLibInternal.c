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

STATIC BOOLEAN  mSevStatus        = FALSE;
STATIC BOOLEAN  mSevEsStatus      = FALSE;
STATIC BOOLEAN  mSevSnpStatus     = FALSE;
STATIC BOOLEAN  mSevStatusChecked = FALSE;

STATIC UINT64   mSevEncryptionMask      = 0;
STATIC BOOLEAN  mSevEncryptionMaskSaved = FALSE;

/**
  Reads and sets the status of SEV features.

  **/
STATIC
VOID
EFIAPI
InternalMemEncryptSevStatus (
  VOID
  )
{
  UINT32                            RegEax;
  MSR_SEV_STATUS_REGISTER           Msr;
  CPUID_MEMORY_ENCRYPTION_INFO_EAX  Eax;
  BOOLEAN                           ReadSevMsr;
  UINT64                            EncryptionMask;

  ReadSevMsr = FALSE;

  EncryptionMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask);
  if (EncryptionMask != 0) {
    //
    // The MSR has been read before, so it is safe to read it again and avoid
    // having to validate the CPUID information.
    //
    ReadSevMsr = TRUE;
  } else {
    //
    // Check if memory encryption leaf exist
    //
    AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
    if (RegEax >= CPUID_MEMORY_ENCRYPTION_INFO) {
      //
      // CPUID Fn8000_001F[EAX] Bit 1 (Sev supported)
      //
      AsmCpuid (CPUID_MEMORY_ENCRYPTION_INFO, &Eax.Uint32, NULL, NULL, NULL);

      if (Eax.Bits.SevBit) {
        ReadSevMsr = TRUE;
      }
    }
  }

  if (ReadSevMsr) {
    //
    // Check MSR_0xC0010131 Bit 0 (Sev Enabled)
    //
    Msr.Uint32 = AsmReadMsr32 (MSR_SEV_STATUS);
    if (Msr.Bits.SevBit) {
      mSevStatus = TRUE;
    }

    //
    // Check MSR_0xC0010131 Bit 1 (Sev-Es Enabled)
    //
    if (Msr.Bits.SevEsBit) {
      mSevEsStatus = TRUE;
    }

    //
    // Check MSR_0xC0010131 Bit 2 (Sev-Snp Enabled)
    //
    if (Msr.Bits.SevSnpBit) {
      mSevSnpStatus = TRUE;
    }
  }

  mSevStatusChecked = TRUE;
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
  if (!mSevStatusChecked) {
    InternalMemEncryptSevStatus ();
  }

  return mSevSnpStatus;
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
  if (!mSevStatusChecked) {
    InternalMemEncryptSevStatus ();
  }

  return mSevEsStatus;
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
  if (!mSevStatusChecked) {
    InternalMemEncryptSevStatus ();
  }

  return mSevStatus;
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
