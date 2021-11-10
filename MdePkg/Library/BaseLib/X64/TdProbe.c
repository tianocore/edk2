/** @file

  Copyright (c) 2020-2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Register/Intel/Cpuid.h>

/**
  Probe if TD is enabled.

  @return TRUE    TD is enabled.
  @return FALSE   TD is not enabled.
**/
BOOLEAN
EFIAPI
TdIsEnabled (
  )
{
  UINT32    Eax;
  UINT32    Ebx;
  UINT32    Ecx;
  UINT32    Edx;
  UINT32    LargestEax;
  BOOLEAN   TdEnabled;

  TdEnabled = FALSE;

  do {
    AsmCpuid (CPUID_SIGNATURE, &LargestEax, &Ebx, &Ecx, &Edx);

    if (Ebx != CPUID_SIGNATURE_GENUINE_INTEL_EBX
      || Edx != CPUID_SIGNATURE_GENUINE_INTEL_EDX
      || Ecx != CPUID_SIGNATURE_GENUINE_INTEL_ECX) {
      break;
    }

    AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, &Ecx, NULL);
    if ((Ecx & BIT31) == 0) {
      break;
    }

    if (LargestEax < 0x21) {
      break;
    }

    AsmCpuidEx (0x21, 0, &Eax, &Ebx, &Ecx, &Edx);
    if (Ebx != SIGNATURE_32 ('I', 'n', 't', 'e')
      || Edx != SIGNATURE_32 ('l', 'T', 'D', 'X')
      || Ecx != SIGNATURE_32 (' ', ' ', ' ', ' ')) {
      break;
    }

    TdEnabled = TRUE;
  }while (FALSE);

  return TdEnabled;
}
