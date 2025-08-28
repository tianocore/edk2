/** @file

  Virtual Memory Management Services to test an address range encryption state

  Copyright (c) 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/CpuLib.h>
#include <Library/CpuPageTableLib.h>
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
  IA32_CR4                             Cr4;
  PAGING_MODE                          PagingMode;
  UINT64                               AddressEncMask;
  PHYSICAL_ADDRESS                     Address;
  PHYSICAL_ADDRESS                     AddressEnd;
  PAGE_TABLE_4K_ENTRY                  Entry;
  UINTN                                Level;
  MEM_ENCRYPT_SEV_ADDRESS_RANGE_STATE  State;
  RETURN_STATUS                        Status;

  //
  // If Cr3BaseAddress is not specified then read the current CR3
  //
  if (Cr3BaseAddress == 0) {
    Cr3BaseAddress = AsmReadCr3 ();
  }

  Cr4.UintN  = AsmReadCr4 ();
  PagingMode = Cr4.Bits.LA57 ? Paging5Level1GB : Paging4Level1GB;

  AddressEncMask  = MemEncryptSevGetEncryptionMask ();
  AddressEncMask &= PAGING_1G_ADDRESS_MASK_64;

  State = MemEncryptSevAddressRangeError;

  //
  // Encryption is on a page basis, so start at the beginning of the
  // virtual address page boundary and walk page-by-page.
  //
  Address    = (PHYSICAL_ADDRESS)(UINTN)BaseAddress & ~EFI_PAGE_MASK;
  AddressEnd = (PHYSICAL_ADDRESS)
               (UINTN)(BaseAddress + Length);

  while (Address < AddressEnd) {
    //
    // The present bit and encryption bit is valid for all leaf entries, so use
    // a 4K PTE entry as a base to hold the pagetable entry.
    //
    Status = PageTableGetEntry (Cr3BaseAddress, PagingMode, Address, &Entry.Uint64, &Level);
    if (Status != RETURN_SUCCESS) {
      return MemEncryptSevAddressRangeError;
    }

    if (Entry.Bits.Present == 0) {
      return MemEncryptSevAddressRangeError;
    }

    State = UpdateAddressState (State, Entry.Uint64, AddressEncMask);

    switch (Level) {
      case 1:
        Address += EFI_PAGE_SIZE;
        break;

      case 2:
        Address += BIT21;
        break;

      case 3:
        Address += BIT30;
        break;

      default:
        return MemEncryptSevAddressRangeError;
    }
  }

  return State;
}
