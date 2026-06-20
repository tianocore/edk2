/** @file File providing a NULL implementation of the MMU helpers for Arm CCA.

  Copyright (c) 2011-2026, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)

**/

#include <Uefi.h>

/**
  Return the Realm CCA protection attribute mask encoded for AArch64
  translation table descriptors.

  The Realm CCA protection attribute is the most significant bit of the Realm
  IPA space, bit (IPA_WIDTH - 1). For non-LPA2 descriptors, the returned mask
  is the raw IPA protection bit. When LPA2 is enabled and the protection bit is
  encoded in the descriptor upper address attribute field, this function returns
  the page-table descriptor encoding, not the raw IPA address bit.

  The returned mask is suitable for use as a page-table attribute and for the
  CCA protection attribute value derived from VirtualBase ^ PhysicalBase by
  ArmConfigureMmu(). Callers that need the raw IPA address bit must not use
  this function.

  If the current execution context is not a Realm, the returned mask is zero.

  @param[out] CcaProtectionAttributeMask  The CCA protection attribute mask.
                                          Zero if not running in a Realm.

  @retval EFI_SUCCESS            The mask was returned successfully.
  @retval EFI_INVALID_PARAMETER  CcaProtectionAttributeMask is NULL.
  @retval Others                 The Realm IPA width could not be queried.
**/
EFI_STATUS
ArmCcaGetMemoryProtectionAttribute (
  OUT UINT64  *CcaProtectionAttributeMask
  )
{
  if (CcaProtectionAttributeMask == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *CcaProtectionAttributeMask = 0;
  return EFI_SUCCESS;
}

/**
  Configure the protection attribute for the page tables
  describing the memory region.

  The IPA space of a Realm is divided into two halves:
    - Protected IPA space and
    - Unprotected IPA space.

  Software in a Realm should treat the most significant bit of an
  IPA as a protection attribute.

  A Protected IPA is an address in the lower half of a Realms IPA
  space. The most significant bit of a Protected IPA is 0.

  An Unprotected IPA is an address in the upper half of a Realms
  IPA space. The most significant bit of an Unprotected IPA is 1.

  Note:
  - Configuring the memory region as Unprotected IPA enables the
    Realm to share the memory region with the Host.
  - This function updates the page table entries to reflect the
    protection attribute.
  - A separate call to transition the memory range using the Realm
    Service Interface (RSI) RSI_IPA_STATE_SET command is additionally
    required and is expected to be done outside this function.
  - The caller must ensure that this function call is invoked by code
    executing within the Realm.

    @param [in]  BaseAddress  Base address of the memory region.
    @param [in]  Length       Length of the memory region.
    @param [in]  IpaWidth     IPA width of the Realm.
    @param [in]  Share        If TRUE, set the most significant
                              bit of the IPA to configure the memory
                              region as Unprotected IPA.
                              If FALSE, clear the most significant
                              bit of the IPA to configure the memory
                              region as Protected IPA.

    @retval EFI_SUCCESS            IPA protection attribute updated.
    @retval EFI_INVALID_PARAMETER  A parameter is invalid.
    @retval EFI_UNSUPPORTED        RME is not supported.
**/
EFI_STATUS
EFIAPI
ArmCcaSetMemoryProtectionAttribute (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                IpaWidth,
  IN  BOOLEAN               Share
  )
{
  // Nothing to do here, return EFI_UNSUPPORTED.
  return EFI_UNSUPPORTED;
}
