/** @file
  aarch64-specifc functionality for DxeLoad.

  Copyright (c) 2024, NewFW Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UefiPayloadEntry.h"

/**
  Entry point to the C language phase of UEFI payload.
  @param[in]   FdtBaseAddr      Pointer to the FDT
  @param[in]   AdditionalPara   Not used yet
  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN  FdtBaseAddr,
  IN UINTN  AddtionalPara
  )
{
  return FitUplEntryPoint (FdtBaseAddr);
}
