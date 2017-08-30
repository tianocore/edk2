/** @file

  Virtual Memory Management Services to set or clear the memory encryption bit

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Code is derived from MdeModulePkg/Core/DxeIplPeim/X64/VirtualMemory.c

**/

#include <Library/CpuLib.h>
#include <Register/Cpuid.h>
#include <Register/Amd/Cpuid.h>

#include "VirtualMemory.h"

STATIC BOOLEAN mAddressEncMaskChecked = FALSE;
STATIC UINT64  mAddressEncMask;

typedef enum {
   SetCBit,
   ClearCBit
} MAP_RANGE_MODE;

/**
  Get the memory encryption mask

  @param[out]      EncryptionMask        contains the pte mask.

**/
STATIC
UINT64
GetMemEncryptionAddressMask (
  VOID
  )
{
  UINT64                            EncryptionMask;
  CPUID_MEMORY_ENCRYPTION_INFO_EBX  Ebx;

  if (mAddressEncMaskChecked) {
    return mAddressEncMask;
  }

  //
  // CPUID Fn8000_001F[EBX] Bit 0:5 (memory encryption bit position)
  //
  AsmCpuid (CPUID_MEMORY_ENCRYPTION_INFO, NULL, &Ebx.Uint32, NULL, NULL);
  EncryptionMask = LShiftU64 (1, Ebx.Bits.PtePosBits);

  mAddressEncMask = EncryptionMask & PAGING_1G_ADDRESS_MASK_64;
  mAddressEncMaskChecked = TRUE;

  return mAddressEncMask;
}

/**
  Split 2M page to 4K.

  @param[in]      PhysicalAddress       Start physical address the 2M page covered.
  @param[in, out] PageEntry2M           Pointer to 2M page entry.
  @param[in]      StackBase             Stack base address.
  @param[in]      StackSize             Stack size.

**/
STATIC
VOID
Split2MPageTo4K (
  IN        PHYSICAL_ADDRESS               PhysicalAddress,
  IN  OUT   UINT64                        *PageEntry2M,
  IN        PHYSICAL_ADDRESS               StackBase,
  IN        UINTN                          StackSize
  )
{
  PHYSICAL_ADDRESS                  PhysicalAddress4K;
  UINTN                             IndexOfPageTableEntries;
  PAGE_TABLE_4K_ENTRY               *PageTableEntry, *PageTableEntry1;
  UINT64                            AddressEncMask;

  PageTableEntry = AllocatePages(1);

  PageTableEntry1 = PageTableEntry;

  AddressEncMask = GetMemEncryptionAddressMask ();

  ASSERT (PageTableEntry != NULL);
  ASSERT (*PageEntry2M & AddressEncMask);

  PhysicalAddress4K = PhysicalAddress;
  for (IndexOfPageTableEntries = 0; IndexOfPageTableEntries < 512; IndexOfPageTableEntries++, PageTableEntry++, PhysicalAddress4K += SIZE_4KB) {
    //
    // Fill in the Page Table entries
    //
    PageTableEntry->Uint64 = (UINT64) PhysicalAddress4K | AddressEncMask;
    PageTableEntry->Bits.ReadWrite = 1;
    PageTableEntry->Bits.Present = 1;
    if ((PhysicalAddress4K >= StackBase) && (PhysicalAddress4K < StackBase + StackSize)) {
      //
      // Set Nx bit for stack.
      //
      PageTableEntry->Bits.Nx = 1;
    }
  }

  //
  // Fill in 2M page entry.
  //
  *PageEntry2M = (UINT64) (UINTN) PageTableEntry1 | IA32_PG_P | IA32_PG_RW | AddressEncMask;
}

/**
  Split 1G page to 2M.

  @param[in]      PhysicalAddress       Start physical address the 1G page covered.
  @param[in, out] PageEntry1G           Pointer to 1G page entry.
  @param[in]      StackBase             Stack base address.
  @param[in]      StackSize             Stack size.

**/
STATIC
VOID
Split1GPageTo2M (
  IN          PHYSICAL_ADDRESS               PhysicalAddress,
  IN  OUT     UINT64                         *PageEntry1G,
  IN          PHYSICAL_ADDRESS               StackBase,
  IN          UINTN                          StackSize
  )
{
  PHYSICAL_ADDRESS                  PhysicalAddress2M;
  UINTN                             IndexOfPageDirectoryEntries;
  PAGE_TABLE_ENTRY                  *PageDirectoryEntry;
  UINT64                            AddressEncMask;

  PageDirectoryEntry = AllocatePages(1);

  AddressEncMask = GetMemEncryptionAddressMask ();
  ASSERT (PageDirectoryEntry != NULL);
  ASSERT (*PageEntry1G & GetMemEncryptionAddressMask ());
  //
  // Fill in 1G page entry.
  //
  *PageEntry1G = (UINT64) (UINTN) PageDirectoryEntry | IA32_PG_P | IA32_PG_RW | AddressEncMask;

  PhysicalAddress2M = PhysicalAddress;
  for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectoryEntry++, PhysicalAddress2M += SIZE_2MB) {
    if ((PhysicalAddress2M < StackBase + StackSize) && ((PhysicalAddress2M + SIZE_2MB) > StackBase)) {
      //
      // Need to split this 2M page that covers stack range.
      //
      Split2MPageTo4K (PhysicalAddress2M, (UINT64 *) PageDirectoryEntry, StackBase, StackSize);
    } else {
      //
      // Fill in the Page Directory entries
      //
      PageDirectoryEntry->Uint64 = (UINT64) PhysicalAddress2M | AddressEncMask;
      PageDirectoryEntry->Bits.ReadWrite = 1;
      PageDirectoryEntry->Bits.Present = 1;
      PageDirectoryEntry->Bits.MustBe1 = 1;
    }
  }
}


/**
  Set or Clear the memory encryption bit

  @param[in]      PagetablePoint        Page table entry pointer (PTE).
  @param[in]      Mode                  Set or Clear encryption bit

**/
STATIC VOID
SetOrClearCBit(
  IN   OUT     UINT64*            PageTablePointer,
  IN           MAP_RANGE_MODE     Mode
  )
{
  UINT64      AddressEncMask;

  AddressEncMask = GetMemEncryptionAddressMask ();

  if (Mode == SetCBit) {
    *PageTablePointer |= AddressEncMask;
  } else {
    *PageTablePointer &= ~AddressEncMask;
  }

}

/**
  This function either sets or clears memory encryption bit for the memory region
  specified by PhysicalAddress and length from the current page table context.

  The function iterates through the physicalAddress one page at a time, and set
  or clears the memory encryption mask in the page table. If it encounters
  that a given physical address range is part of large page then it attempts to
  change the attribute at one go (based on size), otherwise it splits the
  large pages into smaller (e.g 2M page into 4K pages) and then try to set or
  clear the encryption bit on the smallest page size.

  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region
  @param[in]  Mode                    Set or Clear mode
  @param[in]  Flush                   Flush the caches before applying the
                                      encryption mask

  @retval RETURN_SUCCESS              The attributes were cleared for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encyrption attribute is
                                      not supported
**/

STATIC
RETURN_STATUS
EFIAPI
SetMemoryEncDec (
  IN    PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN    PHYSICAL_ADDRESS         PhysicalAddress,
  IN    UINTN                    Length,
  IN    MAP_RANGE_MODE           Mode,
  IN    BOOLEAN                  CacheFlush
  )
{
  PAGE_MAP_AND_DIRECTORY_POINTER *PageMapLevel4Entry;
  PAGE_MAP_AND_DIRECTORY_POINTER *PageUpperDirectoryPointerEntry;
  PAGE_MAP_AND_DIRECTORY_POINTER *PageDirectoryPointerEntry;
  PAGE_TABLE_1G_ENTRY            *PageDirectory1GEntry;
  PAGE_TABLE_ENTRY               *PageDirectory2MEntry;
  PAGE_TABLE_4K_ENTRY            *PageTableEntry;
  UINT64                         PgTableMask;
  UINT64                         AddressEncMask;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: Cr3Base=0x%Lx Physical=0x%Lx Length=0x%Lx Mode=%a CacheFlush=%u\n",
    gEfiCallerBaseName,
    __FUNCTION__,
    Cr3BaseAddress,
    PhysicalAddress,
    (UINT64)Length,
    (Mode == SetCBit) ? "Encrypt" : "Decrypt",
    (UINT32)CacheFlush
    ));

  //
  // Check if we have a valid memory encryption mask
  //
  AddressEncMask = GetMemEncryptionAddressMask ();
  if (!AddressEncMask) {
    return RETURN_ACCESS_DENIED;
  }

  PgTableMask = AddressEncMask | EFI_PAGE_MASK;

  if (Length == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // We are going to change the memory encryption attribute from C=0 -> C=1 or
  // vice versa Flush the caches to ensure that data is written into memory with
  // correct C-bit
  //
  if (CacheFlush) {
    WriteBackInvalidateDataCacheRange((VOID*) (UINTN)PhysicalAddress, Length);
  }

  while (Length)
  {
    //
    // If Cr3BaseAddress is not specified then read the current CR3
    //
    if (Cr3BaseAddress == 0) {
      Cr3BaseAddress = AsmReadCr3();
    }

    PageMapLevel4Entry = (VOID*) (Cr3BaseAddress & ~PgTableMask);
    PageMapLevel4Entry += PML4_OFFSET(PhysicalAddress);
    if (!PageMapLevel4Entry->Bits.Present) {
      DEBUG ((DEBUG_WARN,
        "%a:%a ERROR bad PML4 for %lx\n", gEfiCallerBaseName, __FUNCTION__,
        PhysicalAddress));
      return RETURN_NO_MAPPING;
    }

    PageDirectory1GEntry = (VOID*) ((PageMapLevel4Entry->Bits.PageTableBaseAddress<<12) & ~PgTableMask);
    PageDirectory1GEntry += PDP_OFFSET(PhysicalAddress);
    if (!PageDirectory1GEntry->Bits.Present) {
      DEBUG ((DEBUG_WARN,
        "%a:%a ERROR bad PDPE for %lx\n", gEfiCallerBaseName,
         __FUNCTION__, PhysicalAddress));
      return RETURN_NO_MAPPING;
    }

    //
    // If the MustBe1 bit is not 1, it's not actually a 1GB entry
    //
    if (PageDirectory1GEntry->Bits.MustBe1) {
      //
      // Valid 1GB page
      // If we have at least 1GB to go, we can just update this entry
      //
      if (!(PhysicalAddress & (BIT30 - 1)) && Length >= BIT30) {
        SetOrClearCBit(&PageDirectory1GEntry->Uint64, Mode);
        DEBUG ((DEBUG_VERBOSE,
          "%a:%a Updated 1GB entry for %lx\n", gEfiCallerBaseName,
          __FUNCTION__, PhysicalAddress));
        PhysicalAddress += BIT30;
        Length -= BIT30;
      } else {
        //
        // We must split the page
        //
        DEBUG ((DEBUG_VERBOSE,
          "%a:%a Spliting 1GB page\n", gEfiCallerBaseName, __FUNCTION__));
        Split1GPageTo2M(((UINT64)PageDirectory1GEntry->Bits.PageTableBaseAddress)<<30, (UINT64*) PageDirectory1GEntry, 0, 0);
        continue;
      }
    } else {
      //
      // Actually a PDP
      //
      PageUpperDirectoryPointerEntry = (PAGE_MAP_AND_DIRECTORY_POINTER*) PageDirectory1GEntry;
      PageDirectory2MEntry = (VOID*) ((PageUpperDirectoryPointerEntry->Bits.PageTableBaseAddress<<12) & ~PgTableMask);
      PageDirectory2MEntry += PDE_OFFSET(PhysicalAddress);
      if (!PageDirectory2MEntry->Bits.Present) {
        DEBUG ((DEBUG_WARN,
          "%a:%a ERROR bad PDE for %lx\n", gEfiCallerBaseName, __FUNCTION__,
          PhysicalAddress));
        return RETURN_NO_MAPPING;
      }
      //
      // If the MustBe1 bit is not a 1, it's not a 2MB entry
      //
      if (PageDirectory2MEntry->Bits.MustBe1) {
        //
        // Valid 2MB page
        // If we have at least 2MB left to go, we can just update this entry
        //
        if (!(PhysicalAddress & (BIT21-1)) && Length >= BIT21) {
          SetOrClearCBit (&PageDirectory2MEntry->Uint64, Mode);
          PhysicalAddress += BIT21;
          Length -= BIT21;
        } else {
          //
          // We must split up this page into 4K pages
          //
          DEBUG ((DEBUG_VERBOSE,
            "%a:%a Spliting 2MB page at %lx\n", gEfiCallerBaseName,__FUNCTION__,
            PhysicalAddress));
          Split2MPageTo4K (((UINT64)PageDirectory2MEntry->Bits.PageTableBaseAddress) << 21, (UINT64*) PageDirectory2MEntry, 0, 0);
          continue;
        }
      } else {
        PageDirectoryPointerEntry = (PAGE_MAP_AND_DIRECTORY_POINTER*) PageDirectory2MEntry;
        PageTableEntry = (VOID*) (PageDirectoryPointerEntry->Bits.PageTableBaseAddress<<12 & ~PgTableMask);
        PageTableEntry += PTE_OFFSET(PhysicalAddress);
        if (!PageTableEntry->Bits.Present) {
          DEBUG ((DEBUG_WARN,
            "%a:%a ERROR bad PTE for %lx\n", gEfiCallerBaseName,
            __FUNCTION__, PhysicalAddress));
          return RETURN_NO_MAPPING;
        }
        SetOrClearCBit (&PageTableEntry->Uint64, Mode);
        PhysicalAddress += EFI_PAGE_SIZE;
        Length -= EFI_PAGE_SIZE;
      }
    }
  }

  //
  // Flush TLB
  //
  CpuFlushTlb();

  return RETURN_SUCCESS;
}

/**
  This function clears memory encryption bit for the memory region specified by
  PhysicalAddress and length from the current page table context.

  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region
  @param[in]  Flush                   Flush the caches before applying the
                                      encryption mask

  @retval RETURN_SUCCESS              The attributes were cleared for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encyrption attribute is
                                      not supported
**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevSetMemoryDecrypted (
  IN  PHYSICAL_ADDRESS        Cr3BaseAddress,
  IN  PHYSICAL_ADDRESS        PhysicalAddress,
  IN  UINTN                   Length,
  IN  BOOLEAN                 Flush
  )
{

  return SetMemoryEncDec (Cr3BaseAddress, PhysicalAddress, Length, ClearCBit, Flush);
}

/**
  This function sets memory encryption bit for the memory region specified by
  PhysicalAddress and length from the current page table context.

  @param[in]  PhysicalAddress         The physical address that is the start address
                                      of a memory region.
  @param[in]  Length                  The length of memory region
  @param[in]  Flush                   Flush the caches before applying the
                                      encryption mask

  @retval RETURN_SUCCESS              The attributes were cleared for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encyrption attribute is
                                      not supported
**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevSetMemoryEncrypted (
  IN  PHYSICAL_ADDRESS        Cr3BaseAddress,
  IN  PHYSICAL_ADDRESS        PhysicalAddress,
  IN  UINTN                   Length,
  IN  BOOLEAN                 Flush
  )
{
  return SetMemoryEncDec (Cr3BaseAddress, PhysicalAddress, Length, SetCBit, Flush);
}
