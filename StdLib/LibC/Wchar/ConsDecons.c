/** @file
    Constructor and Deconstructor functions for <wchar.h>.

    Unless explicitly stated otherwise, the functions defined in this file order
    two wide characters the same way as two integers of the underlying integer
    type designated by wchar_t.

    Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/DebugLib.h>

#include  <LibConfig.h>

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
  EFI_STATUS  Status;

  if( __wchar_bitmap == NULL) {
    __wchar_bitmap_size = (WCHAR_MAX + 8) / 8U;

    Status  = SystemTable->BootServices->AllocatePool(
                EfiBootServicesData, __wchar_bitmap_size, (VOID **)&__wchar_bitmap);
    ASSERT(__wchar_bitmap != NULL);
    if (EFI_ERROR (Status)) {
      __wchar_bitmap = NULL;
      return Status;
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
  EFI_STATUS    Status  = RETURN_SUCCESS;

  if( __wchar_bitmap != NULL) {
    Status = SystemTable->BootServices->FreePool( __wchar_bitmap);
    ASSERT_EFI_ERROR (Status);
    __wchar_bitmap = NULL;
  }
  return Status;
}
