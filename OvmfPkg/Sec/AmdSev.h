/** @file
  File defines the Sec routines for the AMD SEV

  Copyright (c) 2021, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _AMD_SEV_SEC_INTERNAL_H__
#define _AMD_SEV_SEC_INTERNAL_H__

/**
  Handle an SEV-ES/GHCB protocol check failure.

  Notify the hypervisor using the VMGEXIT instruction that the SEV-ES guest
  wishes to be terminated.

  @param[in] ReasonCode  Reason code to provide to the hypervisor for the
                         termination request.

**/
VOID
SevEsProtocolFailure (
  IN UINT8  ReasonCode
  );

/**
  Validate the SEV-ES/GHCB protocol level.

  Verify that the level of SEV-ES/GHCB protocol supported by the hypervisor
  and the guest intersect. If they don't intersect, request termination.

**/
VOID
SevEsProtocolCheck (
  VOID
  );

/**
 Determine if the SEV is active.

 During the early booting, GuestType is set in the work area. Verify that it
 is an SEV guest.

 @retval TRUE   SEV is enabled
 @retval FALSE  SEV is not enabled

**/
BOOLEAN
IsSevGuest (
  VOID
  );

/**
  Determine if SEV-ES is active.

  During early booting, SEV-ES support code will set a flag to indicate that
  SEV-ES is enabled. Return the value of this flag as an indicator that SEV-ES
  is enabled.

  @retval TRUE   SEV-ES is enabled
  @retval FALSE  SEV-ES is not enabled

**/
BOOLEAN
SevEsIsEnabled (
  VOID
  );

/**
 Validate System RAM used for decompressing the PEI and DXE firmware volumes
 when SEV-SNP is active. The PCDs SecValidatedStart and SecValidatedEnd are
 set in OvmfPkg/Include/Fdf/FvmainCompactScratchEnd.fdf.inc.

**/
VOID
SecValidateSystemRam (
  VOID
  );

/**
  Determine if SEV-SNP is active.

  @retval TRUE   SEV-SNP is enabled
  @retval FALSE  SEV-SNP is not enabled

**/
BOOLEAN
SevSnpIsEnabled (
  VOID
  );

/**
  Map MMIO regions unencrypted if SEV-ES is active.

  During early booting, page table entries default to having the encryption bit
  set for SEV-ES/SEV-SNP guests. In cases where there is MMIO to an address, the
  encryption bit should be cleared. Clear it here for any known MMIO accesses
  during SEC, which is currently just the APIC base address.

**/
VOID
SecMapApicBaseUnencrypted (
  VOID
  );

#endif
