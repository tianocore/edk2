/** @file
  Variable Storage Router Base Library

Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/MmServicesTableLib.h>

#include <Protocol/VariableStorage.h>

extern EDKII_VARIABLE_STORAGE_PROTOCOL  *mVariableStorage;

/**
  The constructor function for VariableStorageRouterLib.

  @param  ImageHandle     The firmware allocated handle for the EFI image.
  @param  MmSystemTable   A pointer to the MM System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterBaseLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  return gMmst->MmLocateProtocol (
                  &gEdkiiVariableStorageProtocolGuid,
                  NULL,
                  (VOID **)&mVariableStorage
                  );
}
