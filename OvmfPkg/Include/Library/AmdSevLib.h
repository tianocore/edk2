/** @file

  Define Secure Encrypted Virtualization (SEV) base library helper function

  Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _AMD_SEV_LIB_H_
#define _AMD_SEV_LIB_H_

#include <Base.h>
#include <WorkArea.h>

/**
   Read the workarea to determine whether SEV is enabled. If enabled,
   then return the SevEsWorkArea pointer.

  **/
SEC_SEV_ES_WORK_AREA *
EFIAPI
AmdSevGetSevEsWorkArea (
  VOID
  );

/**
  Returns a boolean to indicate whether SEV-SNP is enabled

  @retval TRUE           SEV-SNP is enabled
  @retval FALSE          SEV-SNP is not enabled
**/
BOOLEAN
EFIAPI
AmdSevSnpIsEnabled (
  VOID
  );

/**
  Returns a boolean to indicate whether SEV-ES is enabled.

  @retval TRUE           SEV-ES is enabled
  @retval FALSE          SEV-ES is not enabled
**/
BOOLEAN
EFIAPI
AmdSevEsIsEnabled (
  VOID
  );

/**
  Returns a boolean to indicate whether SEV is enabled

  @retval TRUE           SEV is enabled
  @retval FALSE          SEV is not enabled
**/
BOOLEAN
EFIAPI
AmdSevIsEnabled (
  VOID
  );

/**
  Returns a boolean to indicate whether DebugVirtualization is enabled.

  @retval TRUE           DebugVirtualization is enabled
  @retval FALSE          DebugVirtualization is not enabled
**/
BOOLEAN
EFIAPI
AmdSevEsDebugVirtualizationIsEnabled (
  VOID
  );

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
  );

#endif // _AMD_SEV_LIB_H_
