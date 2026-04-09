/** @file

  Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Amd/SevSnpMsr.h>
#include <Register/Intel/Cpuid.h>

/**
  Probe if running as some kind of SEV guest.

  @return FALSE   Not running as a guest under any kind of SEV
  @return TRUE    Running as a guest under any kind of SEV
**/
BOOLEAN
EFIAPI
SevGuestIsEnabled (
  VOID
  )
{
  UINT32                            MaxExtendedLeaf;
  MSR_SEV_STATUS_REGISTER           SevStatus;
  CPUID_MEMORY_ENCRYPTION_INFO_EAX  MemoryEncryptionInfoEax;

  // Check max extended CPUID leaf
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &MaxExtendedLeaf, NULL, NULL, NULL);
  if (MaxExtendedLeaf < CPUID_MEMORY_ENCRYPTION_INFO) {
    return FALSE;
  }

  // Check SEV support
  AsmCpuid (CPUID_MEMORY_ENCRYPTION_INFO, &MemoryEncryptionInfoEax.Uint32, NULL, NULL, NULL);
  if (MemoryEncryptionInfoEax.Bits.SevBit == 0) {
    return FALSE;
  }

  // Read SEV_STATUS MSR
  SevStatus.Uint64 = AsmReadMsr64 (MSR_SEV_STATUS);

  return (SevStatus.Bits.SevBit | SevStatus.Bits.SevEsBit | SevStatus.Bits.SevSnpBit) ? TRUE : FALSE;
}
