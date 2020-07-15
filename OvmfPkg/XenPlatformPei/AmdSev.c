/**@file
  Initialize Secure Encrypted Virtualization (SEV) support

  Copyright (c) 2017, Advanced Micro Devices. All rights reserved.<BR>
  Copyright (c) 2019, Citrix Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
//
// The package level header files this module uses
//
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/PcdLib.h>
#include <PiPei.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Cpuid.h>

#include "Platform.h"

/**

  Function checks if SEV support is available, if present then it sets
  the dynamic PcdPteMemoryEncryptionAddressOrMask with memory encryption mask.

  **/
VOID
AmdSevInitialize (
  VOID
  )
{
  CPUID_MEMORY_ENCRYPTION_INFO_EBX  Ebx;
  UINT64                            EncryptionMask;
  RETURN_STATUS                     PcdStatus;

  //
  // Check if SEV is enabled
  //
  if (!MemEncryptSevIsEnabled ()) {
    return;
  }

  //
  // CPUID Fn8000_001F[EBX] Bit 0:5 (memory encryption bit position)
  //
  AsmCpuid (CPUID_MEMORY_ENCRYPTION_INFO, NULL, &Ebx.Uint32, NULL, NULL);
  EncryptionMask = LShiftU64 (1, Ebx.Bits.PtePosBits);

  //
  // Set Memory Encryption Mask PCD
  //
  PcdStatus = PcdSet64S (PcdPteMemoryEncryptionAddressOrMask, EncryptionMask);
  ASSERT_RETURN_ERROR (PcdStatus);

  DEBUG ((DEBUG_INFO, "SEV is enabled (mask 0x%lx)\n", EncryptionMask));

  //
  // Set Pcd to Deny the execution of option ROM when security
  // violation.
  //
  PcdStatus = PcdSet32S (PcdOptionRomImageVerificationPolicy, 0x4);
  ASSERT_RETURN_ERROR (PcdStatus);
}
