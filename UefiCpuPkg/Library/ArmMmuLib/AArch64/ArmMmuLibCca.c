/** @file File providing an implementation of the MMU helpers for Arm CCA.

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
#include <Library/ArmMmuLib.h>
#include <Library/BaseLib.h>
#include "ArmMmuLibInternal.h"

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
  UINT64  Attributes;
  UINT64  Mask;
  UINT64  CcaProtectionAttribute;
  UINT64  TopBits;
  UINT64  MaxAddressBits;

  if ((Length == 0) || (IpaWidth == 0) ||
      !IS_ALIGNED (Length, EFI_PAGE_SIZE) ||
      !IS_ALIGNED (BaseAddress, EFI_PAGE_SIZE))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (!ArmHasRme ()) {
    return EFI_UNSUPPORTED;
  }

  if (ArmHas52BitTgran4 ()) {
    MaxAddressBits = MIN (ArmGetPhysicalAddressBits (), MAX_VA_BITS_LPA2);
  } else {
    MaxAddressBits = MIN (ArmGetPhysicalAddressBits (), MAX_VA_BITS_48);
  }

  if (IpaWidth > MaxAddressBits) {
    return EFI_INVALID_PARAMETER;
  }

  CcaProtectionAttribute = 1ULL << (IpaWidth - 1);

  if (ArmLpa2Enabled ()) {
    /*
     * For LPA2, the top IPA bits (e.g., bits 50..51) map into upper page table
     * attributes in a packed form. Extract those top IPA bits and move them to
     * the encoding expected in the Attributes field.
     *
     * TopBits calculation: use BIT51|BIT50 to detect whether the protection bit
     * sits in the top IPA range, then translate to the TTBR/LPA2 attribute
     * position. (See ARM ARM: translation-table entry formats for LPA2).
     */
    TopBits = CcaProtectionAttribute & (BIT51 | BIT50);
    if (TopBits != 0) {
      CcaProtectionAttribute = ((TopBits >> 0x32) & 0x3) << 8;
    }
  }

  if (Share) {
    Attributes = CcaProtectionAttribute;
  } else {
    Attributes = 0;
  }

  if (ArmLpa2Enabled ()) {
    Mask = ~(TT_ADDRESS_MASK_BLOCK_ENTRY_LPA2 | TT_UPPER_ADDRESS_MASK |
             CcaProtectionAttribute);
  } else {
    Mask = ~(TT_ADDRESS_MASK_BLOCK_ENTRY | CcaProtectionAttribute);
  }

  return UpdateRegionMapping (
           BaseAddress,
           Length,
           Attributes,
           Mask,
           ArmGetTTBR0BaseAddress (),
           TRUE,
           ArmLpa2Enabled (),
           CcaProtectionAttribute
           );
}
