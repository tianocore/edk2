/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2026, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/AmdSevLib.h>
#include <Uefi/UefiBaseType.h>

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
  return FALSE;
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
  return FALSE;
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
  return FALSE;
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
  return FALSE;
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
  return FALSE;
}
