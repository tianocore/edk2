/** @file

  Virtual Memory Management Services to set or clear the memory encryption bit

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017 - 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Code is derived from MdeModulePkg/Core/DxeIplPeim/X64/VirtualMemory.h

**/

#ifndef __VIRTUAL_MEMORY__
#define __VIRTUAL_MEMORY__

#include <IndustryStandard/PageTable.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi.h>

#define SYS_CODE64_SEL  0x38

#define PAGE_TABLE_POOL_ALIGNMENT  BASE_2MB
#define PAGE_TABLE_POOL_UNIT_SIZE  SIZE_2MB
#define PAGE_TABLE_POOL_UNIT_PAGES  \
  EFI_SIZE_TO_PAGES (PAGE_TABLE_POOL_UNIT_SIZE)
#define PAGE_TABLE_POOL_ALIGN_MASK  \
  (~(EFI_PHYSICAL_ADDRESS)(PAGE_TABLE_POOL_ALIGNMENT - 1))

typedef struct {
  VOID     *NextPool;
  UINTN    Offset;
  UINTN    FreePages;
} PAGE_TABLE_POOL;

/**
  Return the pagetable memory encryption mask.

  @return  The pagetable memory encryption mask.

**/
UINT64
EFIAPI
InternalGetMemEncryptionAddressMask (
  VOID
  );

/**
  This function clears memory encryption bit for the memory region specified by
  PhysicalAddress and Length from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encyrption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevSetMemoryDecrypted (
  IN  PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN  PHYSICAL_ADDRESS  PhysicalAddress,
  IN  UINTN             Length
  );

/**
  This function sets memory encryption bit for the memory region specified by
  PhysicalAddress and Length from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region

  @retval RETURN_SUCCESS              The attributes were set for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encyrption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevSetMemoryEncrypted (
  IN  PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN  PHYSICAL_ADDRESS  PhysicalAddress,
  IN  UINTN             Length
  );

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
  );

/**
  This function clears memory encryption bit for the MMIO region specified by
  PhysicalAddress and Length.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a MMIO region.
  @param[in]  Length                  The length of memory region

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Length is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encyrption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevClearMmioPageEncMask (
  IN  PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN  PHYSICAL_ADDRESS  PhysicalAddress,
  IN  UINTN             Length
  );

/**
  Create 1GB identity mapping for the specified virtual address range.

  The function is preliminary used by the SEV-SNP page state change
  APIs to build the page table required before issuing the PVALIDATE
  instruction. The function must be removed after the EDK2 core is
  enhanced to do the lazy validation.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  VirtualAddress          Virtual address
  @param[in]  Length                  Length of virtual address range

  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.

**/
RETURN_STATUS
EFIAPI
InternalMemEncryptSevCreateIdentityMap1G (
  IN    PHYSICAL_ADDRESS  Cr3BaseAddress,
  IN    PHYSICAL_ADDRESS  PhysicalAddress,
  IN    UINTN             Length
  );

#endif
