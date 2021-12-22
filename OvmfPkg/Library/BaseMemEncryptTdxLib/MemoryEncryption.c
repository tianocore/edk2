/** @file

  Virtual Memory Management Services to set or clear the memory encryption

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Code is derived from MdeModulePkg/Core/DxeIplPeim/X64/VirtualMemory.c

**/

#include <Uefi.h>
#include <Uefi/UefiBaseType.h>
#include <Library/CpuLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/Tdx.h>
#include <Library/TdxLib.h>
#include <ConfidentialComputingGuestAttr.h>

#include <Library/PageTablesLib.h>
#include <Library/MemEncryptTdxLib.h>

typedef enum {
  SetSharedBit,
  ClearSharedBit
} TDX_PAGETABLE_MODE;

TDX_PAGETABLE_MODE  mMode = SetSharedBit;

PAGE_TABLES_PCD_SETTINGS  mPageTablesPcdSettings = {
  TRUE,                                               // PcdSetNxForStack
  FALSE,                                              // PcdIa32EferChangeAllowed
  FixedPcdGetBool (PcdCpuStackGuard),                 // PcdCpuStackGuard
  FixedPcdGetBool (PcdUse1GPageTable),                // PcdUse1GPageTable
  FALSE,                                              // PcdUse5LevelPageTable
  FixedPcdGet8 (PcdNullPointerDetectionPropertyMask), // PcdNullPointerDetectionPropertyMask
  FixedPcdGet32 (PcdImageProtectionPolicy),           // PcdImageProtectionPolicy
  FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy),     // PcdDxeNxMemoryProtectionPolicy
  0,                                                  // PcdPteMemoryEncryptionAddressOrMask
  0                                                   // PgTableMask
};

/**
  Returns boolean to indicate whether to indicate which, if any, memory encryption is enabled

  @param[in]  Type          Bitmask of encryption technologies to check is enabled

  @retval TRUE              The encryption type(s) are enabled
  @retval FALSE             The encryption type(s) are not enabled
**/
BOOLEAN
EFIAPI
MemEncryptTdxIsEnabled (
  VOID
  )
{
  return CC_GUEST_IS_TDX (PcdGet64 (PcdConfidentialComputingGuestAttr));
}

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
  return TdSharedPageMask ();
}

/**
  Set or Clear the memory encryption bit

  @param[in,out]  PageTablePtr          Page table entry pointer (PTE).
  @param[in]      PhysicalAddress       Physical address of the memory region
  @param[in]      Length                Length of the memory region
  @param[in]      Rsvd                  Not used parameter

**/
EFI_STATUS
EFIAPI
SetOrClearSharedBit (
  IN   OUT     UINT64            PageTablePtr,
  IN           PHYSICAL_ADDRESS  PhysicalAddress,
  IN           UINT64            Length,
  IN           UINT64            Rsvd
  )
{
  UINT64  *PageTablePointer;
  UINT64  AddressEncMask;
  UINT64  Status;

  PageTablePointer = (UINT64 *)(UINTN)PageTablePtr;
  AddressEncMask   = GetMemEncryptionAddressMask ();

  //
  // Set or clear page table entry. Also, set shared bit in physical address, before calling MapGPA
  //
  if (mMode == SetSharedBit) {
    *PageTablePointer |= AddressEncMask;
    PhysicalAddress   |= AddressEncMask;
  } else {
    *PageTablePointer &= ~AddressEncMask;
    PhysicalAddress   &= ~AddressEncMask;
  }

  Status = TdVmCall (TDVMCALL_MAPGPA, PhysicalAddress, Length, 0, 0, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If changing shared to private, must accept-page again
  //
  if (mMode == ClearSharedBit) {
    Status = TdAcceptPages (PhysicalAddress, Length / EFI_PAGE_SIZE, EFI_PAGE_SIZE);
  }

  return Status;
}

/**
  This function either sets or clears memory encryption for the memory
  region specified by PhysicalAddress and Length from the current page table
  context.

  The function iterates through the PhysicalAddress one page at a time, and set
  or clears the memory encryption in the page table. If it encounters
  that a given physical address range is part of large page then it attempts to
  change the attribute at one go (based on size), otherwise it splits the
  large pages into smaller (e.g 2M page into 4K pages) and then try to set or
  clear the encryption bit on the smallest page size.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region
  @param[in]  Mode                    Set or Clear mode

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encyrption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
SetMemorySharedOrPrivate (
  IN    PHYSICAL_ADDRESS    Cr3BaseAddress,
  IN    PHYSICAL_ADDRESS    PhysicalAddress,
  IN    UINTN               Length,
  IN    TDX_PAGETABLE_MODE  Mode
  )
{
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageMapLevel4Entry;
  UINT64                          PgTableMask;
  BOOLEAN                         IsWpEnabled;
  RETURN_STATUS                   Status;

  mMode              = Mode;
  Status             = EFI_SUCCESS;
  PageMapLevel4Entry = NULL;

  if (Length == 0) {
    return EFI_INVALID_PARAMETER;
  }

  PgTableMask = GetMemEncryptionAddressMask () | EFI_PAGE_MASK;

  //
  // Make sure that the page table is changeable.
  //
  IsWpEnabled = IsReadOnlyPageWriteProtected ();
  if (IsWpEnabled) {
    DisableReadOnlyPageWriteProtect ();
  }

  if (Cr3BaseAddress == 0) {
    Cr3BaseAddress = AsmReadCr3 ();
  }

  PageMapLevel4Entry  = (VOID *)(Cr3BaseAddress & ~PgTableMask);
  PageMapLevel4Entry += PML4_OFFSET (PhysicalAddress);

  //
  //  Before call the PageTablesLib function, set the PCDs first
  //
  mPageTablesPcdSettings.PgTableMask = PgTableMask;
  SetPageTablesPcdSettings (&mPageTablesPcdSettings);

  //
  // Call SetClearCcBits
  //
  Status = SetClearCcBits (Cr3BaseAddress, PhysicalAddress, Length, SetOrClearSharedBit);

  //
  // Protect the page table by marking the memory used for page table to be
  // read-only.
  //
  if (IsWpEnabled) {
    EnablePageTableProtection ((UINTN)PageMapLevel4Entry, TRUE);
  }

  //
  // Flush TLB
  //
  CpuFlushTlb ();

  //
  // Restore page table write protection, if any.
  //
  if (IsWpEnabled) {
    EnableReadOnlyPageWriteProtect ();
  }

  return Status;
}

/**
  This function clears memory encryption bit for the memory region specified by
  BaseAddress and NumPages from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a memory region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptTdxSetPageSharedBit (
  IN PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN             NumPages
  )
{
  return SetMemorySharedOrPrivate (
           Cr3BaseAddress,
           BaseAddress,
           EFI_PAGES_TO_SIZE (NumPages),
           SetSharedBit
           );
}

/**
  This function sets memory encryption bit for the memory region specified by
  BaseAddress and NumPages from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a memory region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.

  @retval RETURN_SUCCESS              The attributes were set for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptTdxClearPageSharedBit (
  IN PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN             NumPages
  )
{
  return SetMemorySharedOrPrivate (
           Cr3BaseAddress,
           BaseAddress,
           EFI_PAGES_TO_SIZE (NumPages),
           ClearSharedBit
           );
}
