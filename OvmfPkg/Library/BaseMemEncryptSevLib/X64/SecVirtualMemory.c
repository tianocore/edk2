/** @file

  Virtual Memory Management Services to set or clear the memory encryption bit

  Copyright (c) 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/CpuLib.h>
#include <Library/MemEncryptSevLib.h>

#include "VirtualMemory.h"

/**
  Return the pagetable memory encryption mask.

  @return  The pagetable memory encryption mask.

**/
UINT64
EFIAPI
InternalGetMemEncryptionAddressMask (
  VOID
  )
{
  UINT64                            EncryptionMask;

  EncryptionMask = MemEncryptSevGetEncryptionMask ();
  EncryptionMask &= PAGING_1G_ADDRESS_MASK_64;

  return EncryptionMask;
}

/**
  This function clears memory encryption bit for the memory region specified by
  PhysicalAddress and Length from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region
  @param[in]  Flush                   Flush the caches before applying the
                                      encryption mask

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encyrption attribute
                                      is not supported
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
  //
  // This function is not available during SEC.
  //
  return RETURN_UNSUPPORTED;
}

/**
  This function sets memory encryption bit for the memory region specified by
  PhysicalAddress and Length from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  PhysicalAddress         The physical address that is the start
                                      address of a memory region.
  @param[in]  Length                  The length of memory region
  @param[in]  Flush                   Flush the caches before applying the
                                      encryption mask

  @retval RETURN_SUCCESS              The attributes were set for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encyrption attribute
                                      is not supported
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
  //
  // This function is not available during SEC.
  //
  return RETURN_UNSUPPORTED;
}
