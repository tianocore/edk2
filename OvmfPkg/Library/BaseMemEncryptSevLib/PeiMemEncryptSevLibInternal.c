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

STATIC BOOLEAN mSevStatus = FALSE;
STATIC BOOLEAN mSevEsStatus = FALSE;
STATIC BOOLEAN mSevStatusChecked = FALSE;
STATIC BOOLEAN mSevLiveMigrationStatus = FALSE;
STATIC BOOLEAN mSevLiveMigrationStatusChecked = FALSE;

STATIC UINT64  mSevEncryptionMask = 0;
STATIC BOOLEAN mSevEncryptionMaskSaved = FALSE;

#define KVM_FEATURE_MIGRATION_CONTROL   BIT17

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
  SEC_SEV_ES_WORK_AREA              *SevEsWorkArea;

  ReadSevMsr = FALSE;

  SevEsWorkArea = (SEC_SEV_ES_WORK_AREA *) FixedPcdGet32 (PcdSevEsWorkAreaBase);
  if (SevEsWorkArea != NULL && SevEsWorkArea->EncryptionMask != 0) {
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
  }

  mSevStatusChecked = TRUE;
}

/**
  Figures out if we are running inside KVM HVM and
  KVM HVM supports SEV Live Migration feature.
**/
STATIC
VOID
EFIAPI
KvmDetectSevLiveMigrationFeature(
  VOID
  )
{
  UINT8 Signature[13];
  UINT32 mKvmLeaf = 0;
  UINT32 RegEax, RegEbx, RegEcx, RegEdx;

  Signature[12] = '\0';
  for (mKvmLeaf = 0x40000000; mKvmLeaf < 0x40010000; mKvmLeaf += 0x100) {
    AsmCpuid (mKvmLeaf,
              NULL,
              (UINT32 *) &Signature[0],
              (UINT32 *) &Signature[4],
              (UINT32 *) &Signature[8]);

    if (!AsciiStrCmp ((CHAR8 *) Signature, "KVMKVMKVM\0\0\0")) {
      DEBUG ((
        DEBUG_INFO,
        "%a: KVM Detected, signature = %s\n",
        __FUNCTION__,
        Signature
        ));

      RegEax = mKvmLeaf + 1;
      RegEcx = 0;
      AsmCpuid (mKvmLeaf + 1, &RegEax, &RegEbx, &RegEcx, &RegEdx);
      if ((RegEax & KVM_FEATURE_MIGRATION_CONTROL) != 0) {
        DEBUG ((
          DEBUG_INFO,
          "%a: Live Migration feature supported\n",
          __FUNCTION__
          ));
        mSevLiveMigrationStatus = TRUE;
      }
    }
  }

  mSevLiveMigrationStatusChecked = TRUE;
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
  Returns a boolean to indicate whether SEV live migration is enabled.

  @retval TRUE           SEV live migration is enabled
  @retval FALSE          SEV live migration is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevLiveMigrationIsEnabled (
  VOID
  )
{
  if (!mSevLiveMigrationStatusChecked) {
    KvmDetectSevLiveMigrationFeature ();
  }

  return mSevLiveMigrationStatus;
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
    SEC_SEV_ES_WORK_AREA  *SevEsWorkArea;

    SevEsWorkArea = (SEC_SEV_ES_WORK_AREA *) FixedPcdGet32 (PcdSevEsWorkAreaBase);
    if (SevEsWorkArea != NULL) {
      mSevEncryptionMask = SevEsWorkArea->EncryptionMask;
    } else {
      CPUID_MEMORY_ENCRYPTION_INFO_EBX  Ebx;

      //
      // CPUID Fn8000_001F[EBX] Bit 0:5 (memory encryption bit position)
      //
      AsmCpuid (CPUID_MEMORY_ENCRYPTION_INFO, NULL, &Ebx.Uint32, NULL, NULL);
      mSevEncryptionMask = LShiftU64 (1, Ebx.Bits.PtePosBits);
    }

    mSevEncryptionMaskSaved = TRUE;
  }

  return mSevEncryptionMask;
}
