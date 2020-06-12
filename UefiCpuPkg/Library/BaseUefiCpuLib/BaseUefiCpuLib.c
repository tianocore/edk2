/** @file
  This library defines some routines that are generic for IA32 family CPU.

  The library routines are UEFI specification compliant.

  Copyright (c) 2020, AMD Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Register/Intel/Cpuid.h>
#include <Register/Amd/Cpuid.h>

#include <Library/BaseLib.h>
#include <Library/UefiCpuLib.h>

/**
  Determine if the standard CPU signature is "AuthenticAMD".

  @retval TRUE  The CPU signature matches.
  @retval FALSE The CPU signature does not match.

**/
BOOLEAN
EFIAPI
StandardSignatureIsAuthenticAMD (
  VOID
  )
{
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;

  AsmCpuid (CPUID_SIGNATURE, NULL, &RegEbx, &RegEcx, &RegEdx);
  return (RegEbx == CPUID_SIGNATURE_AUTHENTIC_AMD_EBX &&
          RegEcx == CPUID_SIGNATURE_AUTHENTIC_AMD_ECX &&
          RegEdx == CPUID_SIGNATURE_AUTHENTIC_AMD_EDX);
}
