/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Register/Cpuid.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Amd/Msr.h>
#include <Library/MemEncryptSevLib.h>

#include "VirtualMemory.h"

/**
  This function clears memory encryption bit for the memory region specified by
  BaseAddress and NumPages from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a memory region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.
  @param[in]  Flush                   Flush the caches before clearing the bit
                                      (mostly TRUE except MMIO addresses)

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptSevClearPageEncMask (
  IN PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINTN                    NumPages,
  IN BOOLEAN                  Flush
  )
{
  return InternalMemEncryptSevSetMemoryDecrypted (
           Cr3BaseAddress,
           BaseAddress,
           EFI_PAGES_TO_SIZE (NumPages),
           Flush
           );
}

/**

  This function clears memory encryption bit for the memory region specified by
  BaseAddress and Number of pages from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a memory region.
  @param[in]  NumberOfPages           The number of pages from start memory
                                      region.
  @param[in]  Flush                   Flush the caches before clearing the bit
                                      (mostly TRUE except MMIO addresses)

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encryption attribute
                                      is not supported
  **/
RETURN_STATUS
EFIAPI
MemEncryptSevSetPageEncMask (
  IN PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINTN                    NumPages,
  IN BOOLEAN                  Flush
  )
{
  return InternalMemEncryptSevSetMemoryEncrypted (
           Cr3BaseAddress,
           BaseAddress,
           EFI_PAGES_TO_SIZE (NumPages),
           Flush
           );
}
