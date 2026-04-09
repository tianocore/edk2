/** @file

  Virtual Memory Management Services to test an address range encryption state

  Copyright (c) 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/CpuLib.h>
#include <Library/MemEncryptSevLib.h>

#include "VirtualMemory.h"

/**
  Returns the (updated) address range state based upon the page table
  entry.

  @param[in]  CurrentState            The current address range state
  @param[in]  PageDirectoryEntry      The page table entry to check
  @param[in]  AddressEncMask          The encryption mask

  @retval MemEncryptSevAddressRangeUnencrypted  Address range is mapped
                                                unencrypted
  @retval MemEncryptSevAddressRangeEncrypted    Address range is mapped
                                                encrypted
  @retval MemEncryptSevAddressRangeMixed        Address range is mapped mixed
**/
STATIC
MEM_ENCRYPT_SEV_ADDRESS_RANGE_STATE
UpdateAddressState (
  IN MEM_ENCRYPT_SEV_ADDRESS_RANGE_STATE  CurrentState,
  IN UINT64                               PageDirectoryEntry,
  IN UINT64                               AddressEncMask
  )
{
  if (CurrentState == MemEncryptSevAddressRangeEncrypted) {
    if ((PageDirectoryEntry & AddressEncMask) == 0) {
      CurrentState = MemEncryptSevAddressRangeMixed;
    }
  } else if (CurrentState == MemEncryptSevAddressRangeUnencrypted) {
    if ((PageDirectoryEntry & AddressEncMask) != 0) {
      CurrentState = MemEncryptSevAddressRangeMixed;
    }
  } else if (CurrentState == MemEncryptSevAddressRangeError) {
    //
    // First address check, set initial state
    //
    if ((PageDirectoryEntry & AddressEncMask) == 0) {
      CurrentState = MemEncryptSevAddressRangeUnencrypted;
    } else {
      CurrentState = MemEncryptSevAddressRangeEncrypted;
    }
  }

  return CurrentState;
}

/**
  Returns the encryption state of the specified virtual address range.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             Base address to check
  @param[in]  Length                  Length of virtual address range

  @retval MemEncryptSevAddressRangeUnencrypted  Address range is mapped
                                                unencrypted
  @retval MemEncryptSevAddressRangeEncrypted    Address range is mapped
                                                encrypted
  @retval MemEncryptSevAddressRangeMixed        Address range is mapped mixed
  @retval MemEncryptSevAddressRangeError        Address range is not mapped
**/
MEM_ENCRYPT_SEV_ADDRESS_RANGE_STATE
EFIAPI
InternalMemEncryptSevGetAddressRangeState (
  IN PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN             Length
  )
{
  PAGE_MAP_AND_DIRECTORY_POINTER       *PageMapLevel4Entry;
  PAGE_MAP_AND_DIRECTORY_POINTER       *PageUpperDirectoryPointerEntry;
  PAGE_MAP_AND_DIRECTORY_POINTER       *PageDirectoryPointerEntry;
  PAGE_TABLE_1G_ENTRY                  *PageDirectory1GEntry;
  PAGE_TABLE_ENTRY                     *PageDirectory2MEntry;
  PAGE_TABLE_4K_ENTRY                  *PageTableEntry;
  UINT64                               AddressEncMask;
  UINT64                               PgTableMask;
  PHYSICAL_ADDRESS                     Address;
  PHYSICAL_ADDRESS                     AddressEnd;
  MEM_ENCRYPT_SEV_ADDRESS_RANGE_STATE  State;

  //
  // If Cr3BaseAddress is not specified then read the current CR3
  //
  if (Cr3BaseAddress == 0) {
    Cr3BaseAddress = AsmReadCr3 ();
  }

  AddressEncMask  = MemEncryptSevGetEncryptionMask ();
  AddressEncMask &= PAGING_1G_ADDRESS_MASK_64;

  PgTableMask = AddressEncMask | EFI_PAGE_MASK;

  State = MemEncryptSevAddressRangeError;

  //
  // Encryption is on a page basis, so start at the beginning of the
  // virtual address page boundary and walk page-by-page.
  //
  Address    = (PHYSICAL_ADDRESS)(UINTN)BaseAddress & ~EFI_PAGE_MASK;
  AddressEnd = (PHYSICAL_ADDRESS)
               (UINTN)(BaseAddress + Length);

  while (Address < AddressEnd) {
    PageMapLevel4Entry  = (VOID *)(Cr3BaseAddress & ~PgTableMask);
    PageMapLevel4Entry += PML4_OFFSET (Address);
    if (!PageMapLevel4Entry->Bits.Present) {
      return MemEncryptSevAddressRangeError;
    }

    PageDirectory1GEntry = (VOID *)(
                                    (PageMapLevel4Entry->Bits.PageTableBaseAddress <<
                                     12) & ~PgTableMask
                                    );
    PageDirectory1GEntry += PDP_OFFSET (Address);
    if (!PageDirectory1GEntry->Bits.Present) {
      return MemEncryptSevAddressRangeError;
    }

    //
    // If the MustBe1 bit is not 1, it's not actually a 1GB entry
    //
    if (PageDirectory1GEntry->Bits.MustBe1) {
      //
      // Valid 1GB page
      //
      State = UpdateAddressState (
                State,
                PageDirectory1GEntry->Uint64,
                AddressEncMask
                );

      Address += BIT30;
      continue;
    }

    //
    // Actually a PDP
    //
    PageUpperDirectoryPointerEntry =
      (PAGE_MAP_AND_DIRECTORY_POINTER *)PageDirectory1GEntry;
    PageDirectory2MEntry =
      (VOID *)(
               (PageUpperDirectoryPointerEntry->Bits.PageTableBaseAddress <<
                12) & ~PgTableMask
               );
    PageDirectory2MEntry += PDE_OFFSET (Address);
    if (!PageDirectory2MEntry->Bits.Present) {
      return MemEncryptSevAddressRangeError;
    }

    //
    // If the MustBe1 bit is not a 1, it's not a 2MB entry
    //
    if (PageDirectory2MEntry->Bits.MustBe1) {
      //
      // Valid 2MB page
      //
      State = UpdateAddressState (
                State,
                PageDirectory2MEntry->Uint64,
                AddressEncMask
                );

      Address += BIT21;
      continue;
    }

    //
    // Actually a PMD
    //
    PageDirectoryPointerEntry =
      (PAGE_MAP_AND_DIRECTORY_POINTER *)PageDirectory2MEntry;
    PageTableEntry =
      (VOID *)(
               (PageDirectoryPointerEntry->Bits.PageTableBaseAddress <<
                12) & ~PgTableMask
               );
    PageTableEntry += PTE_OFFSET (Address);
    if (!PageTableEntry->Bits.Present) {
      return MemEncryptSevAddressRangeError;
    }

    State = UpdateAddressState (
              State,
              PageTableEntry->Uint64,
              AddressEncMask
              );

    Address += EFI_PAGE_SIZE;
  }

  return State;
}
