/** @file
    Constructor and Deconstructor functions for <wchar.h>.

    Unless explicitly stated otherwise, the functions defined in this file order
    two wide characters the same way as two integers of the underlying integer
    type designated by wchar_t.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/BaseLib.h>
#include  <Library/BaseMemoryLib.h>
#include  <Library/MemoryAllocationLib.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <wchar.h>

/* Data initialized by the library constructor */
UINT8 *__wchar_bitmap       = NULL;
UINTN  __wchar_bitmap_size;
UINTN  __wchar_bitmap_64;

EFI_STATUS
EFIAPI
__wchar_construct(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if( __wchar_bitmap == NULL) {
    __wchar_bitmap_size = (WCHAR_MAX + 8) / 8U;
    __wchar_bitmap = AllocatePool(__wchar_bitmap_size);
    if( __wchar_bitmap == NULL) {
      EFIerrno = RETURN_OUT_OF_RESOURCES;
      errno = ENOMEM;
      return EFIerrno;
    }
    return  RETURN_SUCCESS;
  }
  return RETURN_ALREADY_STARTED;
}

EFI_STATUS
EFIAPI
__wchar_deconstruct(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if( __wchar_bitmap != NULL) {
    FreePool( __wchar_bitmap);
    __wchar_bitmap = NULL;
  }
  return RETURN_SUCCESS;
}
