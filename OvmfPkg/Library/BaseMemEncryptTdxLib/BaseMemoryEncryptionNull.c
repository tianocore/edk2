/** @file

  Virtual Memory Management Services to set or clear the memory encryption

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Code is derived from MdeModulePkg/Core/DxeIplPeim/X64/VirtualMemory.c

**/

#include <Uefi.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/MemEncryptTdxLib.h>

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
  return FALSE;
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
  return EFI_UNSUPPORTED;
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
  return EFI_UNSUPPORTED;
}
