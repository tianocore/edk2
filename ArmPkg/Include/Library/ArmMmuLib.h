/** @file

  Copyright (c) 2015 - 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_MMU_LIB_H_
#define ARM_MMU_LIB_H_

#include <Uefi/UefiBaseType.h>

#include <Library/ArmLib.h>

EFI_STATUS
EFIAPI
ArmConfigureMmu (
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable,
  OUT VOID                          **TranslationTableBase OPTIONAL,
  OUT UINTN                         *TranslationTableSize  OPTIONAL
  );

VOID
EFIAPI
ArmReplaceLiveTranslationEntry (
  IN  UINT64   *Entry,
  IN  UINT64   Value,
  IN  UINT64   RegionStart,
  IN  BOOLEAN  DisableMmu
  );

/**
  Set the requested memory permission attributes on a region of memory.

  BaseAddress and Length must be aligned to EFI_PAGE_SIZE.

  If Attributes contains a memory type attribute (EFI_MEMORY_UC/WC/WT/WB), the
  region is mapped according to this memory type, and additional memory
  permission attributes (EFI_MEMORY_RP/RO/XP) are taken into account as well,
  discarding any permission attributes that are currently set for the region.
  AttributeMask is ignored in this case, and must be set to 0x0.

  If Attributes contains only a combination of memory permission attributes
  (EFI_MEMORY_RP/RO/XP), each page in the region will retain its existing
  memory type, even if it is not uniformly set across the region. In this case,
  AttributesMask may be set to a mask of permission attributes, and memory
  permissions omitted from this mask will not be updated for any page in the
  region. All attributes appearing in Attributes must appear in AttributeMask
  as well. (Attributes & ~AttributeMask must produce 0x0)

  @param[in]  BaseAddress     The physical address that is the start address of
                              a memory region.
  @param[in]  Length          The size in bytes of the memory region.
  @param[in]  Attributes      Mask of memory attributes to set.
  @param[in]  AttributeMask   Mask of memory attributes to take into account.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_INVALID_PARAMETER BaseAddress or Length is not suitably aligned.
                                Invalid combination of Attributes and
                                AttributeMask.
  @retval EFI_OUT_OF_RESOURCES  Requested attributes cannot be applied due to
                                lack of system resources.

**/
EFI_STATUS
ArmSetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes,
  IN UINT64                AttributeMask
  );

#ifdef MDE_CPU_AARCH64

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
SetMemoryProtectionAttribute (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                IpaWidth,
  IN  BOOLEAN               Share
  );

#endif

#endif // ARM_MMU_LIB_H_
