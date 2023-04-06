/** @file
*
*  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <PiPei.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseRiscVSbiLib.h>

/**
  Returns the pointer to the HOB list.

  This function returns the pointer to first HOB in the list.

  @return The pointer to the HOB list.

**/
VOID *
EFIAPI
PrePeiGetHobList (
  VOID
  )
{
  EFI_RISCV_FIRMWARE_CONTEXT  *FirmwareContext;

  FirmwareContext = NULL;
  GetFirmwareContextPointer (&FirmwareContext);

  if (FirmwareContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Firmware Context is NULL\n", __func__));
    return NULL;
  }

  return (VOID *)FirmwareContext->PrePiHobList;
}

/**
  Updates the pointer to the HOB list.

  @param  HobList       Hob list pointer to store

**/
EFI_STATUS
EFIAPI
PrePeiSetHobList (
  IN  VOID  *HobList
  )
{
  EFI_RISCV_FIRMWARE_CONTEXT  *FirmwareContext;

  FirmwareContext = NULL;
  GetFirmwareContextPointer (&FirmwareContext);

  if (FirmwareContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Firmware Context is NULL\n", __func__));
    return EFI_NOT_READY;
  }

  FirmwareContext->PrePiHobList = HobList;
  return EFI_SUCCESS;
}
