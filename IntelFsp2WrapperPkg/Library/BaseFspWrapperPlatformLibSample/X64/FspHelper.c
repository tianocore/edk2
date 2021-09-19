/** @file
  Sample to provide FSP wrapper related function.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Library/PcdLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <Uefi/UefiBaseType.h>

/**
  Get the Fspm Upd Data Address from the PCD

  @return FSPM UPD Data Address
**/
UINTN
EFIAPI
GetFspmUpdDataAddress (
  VOID
  )
{
  return PcdGet64 (PcdFspmUpdDataAddress64);
}

/**
  Get the Fsps Upd Data Address from the PCD

  @return FSPS UPD Data Address
**/
UINTN
EFIAPI
GetFspsUpdDataAddress (
  VOID
  )
{
  return PcdGet64 (PcdFspsUpdDataAddress64);
}