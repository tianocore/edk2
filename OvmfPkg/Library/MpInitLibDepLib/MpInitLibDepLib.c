/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

/**
This is null constructor which always return EFI_SUCCESS.
@param  ImageHandle   The firmware allocated handle for the EFI image.
@param  SystemTable   A pointer to the EFI System Table.
@retval EFI_SUCCESS   Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
MpInitLibDepContructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
