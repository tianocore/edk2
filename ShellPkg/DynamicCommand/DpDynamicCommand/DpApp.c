/** @file
  Entrypoint of "dp" shell standalone application.

  Copyright (c) 2017, Intel Corporation. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "Dp.h"

//
// String token ID of help message text.
// Shell supports to find help message in the resource section of an application image if
// .MAN file is not found. This global variable is added to make build tool recognizes
// that the help string is consumed by user and then build tool will add the string into
// the resource section. Thus the application can use '-?' option to show help message in
// Shell.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_STRING_ID  mStringHelpTokenId = STRING_TOKEN (STR_GET_HELP_DP);

/**
  Entry point of Tftp standalone application.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           Tftp command is executed successfully.
  @retval EFI_ABORTED           HII package was failed to initialize.
  @retval others                Other errors when executing tftp command.
**/
EFI_STATUS
EFIAPI
DpAppInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mDpHiiHandle = InitializeHiiPackage (ImageHandle);
  if (mDpHiiHandle == NULL) {
    return EFI_ABORTED;
  }

  Status = (EFI_STATUS)RunDp (ImageHandle, SystemTable);
  HiiRemovePackages (mDpHiiHandle);
  return Status;
}
