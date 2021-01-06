/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>

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
  Reads and sets the status of SEV features.

  **/
STATIC
UINT32
EFIAPI
InternalMemEncryptSevStatus (
  VOID
  )
{
  UINT32                            RegEax;
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

  return ReadSevMsr ? AsmReadMsr32 (MSR_SEV_STATUS) : 0;
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
  MSR_SEV_STATUS_REGISTER           Msr;

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
  MSR_SEV_STATUS_REGISTER           Msr;

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
  CPUID_MEMORY_ENCRYPTION_INFO_EBX  Ebx;
  SEC_SEV_ES_WORK_AREA              *SevEsWorkArea;
  UINT64                            EncryptionMask;

  SevEsWorkArea = (SEC_SEV_ES_WORK_AREA *) FixedPcdGet32 (PcdSevEsWorkAreaBase);
  if (SevEsWorkArea != NULL) {
    EncryptionMask = SevEsWorkArea->EncryptionMask;
  } else {
    //
    // CPUID Fn8000_001F[EBX] Bit 0:5 (memory encryption bit position)
    //
    AsmCpuid (CPUID_MEMORY_ENCRYPTION_INFO, NULL, &Ebx.Uint32, NULL, NULL);
    EncryptionMask = LShiftU64 (1, Ebx.Bits.PtePosBits);
  }

  return EncryptionMask;
}

/**
  Locate the page range that covers the initial (pre-SMBASE-relocation) SMRAM
  Save State Map.

  @param[out] BaseAddress     The base address of the lowest-address page that
                              covers the initial SMRAM Save State Map.

  @param[out] NumberOfPages   The number of pages in the page range that covers
                              the initial SMRAM Save State Map.

  @retval RETURN_SUCCESS      BaseAddress and NumberOfPages have been set on
                              output.

  @retval RETURN_UNSUPPORTED  SMM is unavailable.
**/
RETURN_STATUS
EFIAPI
MemEncryptSevLocateInitialSmramSaveStateMapPages (
  OUT UINTN *BaseAddress,
  OUT UINTN *NumberOfPages
  )
{
  return RETURN_UNSUPPORTED;
}
