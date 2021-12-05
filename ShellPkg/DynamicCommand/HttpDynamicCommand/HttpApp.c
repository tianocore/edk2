/** @file
  Entrypoint of "http" shell standalone application.

  Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2020, Broadcom. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "Http.h"

/*
 * String token ID of help message text.
 * Shell supports to find help message in the resource section of an
 * application image if * .MAN file is not found.
 * This global variable is added to make build tool recognizes
 * that the help string is consumed by user and then build tool will
 * add the string into the resource section.
 * Thus the application can use '-?' option to show help message in Shell.
 */
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_STRING_ID  mStringHelpTokenId = STRING_TOKEN (STR_GET_HELP_HTTP);

/**
  Entry point of Http standalone application.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           Http command is executed sucessfully.
  @retval EFI_ABORTED           HII package was failed to initialize.
  @retval others                Other errors when executing http command.
**/
EFI_STATUS
EFIAPI
HttpAppInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  SHELL_STATUS  ShellStatus;

  mHttpHiiHandle = InitializeHiiPackage (ImageHandle);
  if (mHttpHiiHandle == NULL) {
    return EFI_ABORTED;
  }

  Status = EFI_SUCCESS;

  ShellStatus = RunHttp (ImageHandle, SystemTable);

  HiiRemovePackages (mHttpHiiHandle);

  if (Status != SHELL_SUCCESS) {
    Status = ENCODE_ERROR (ShellStatus);
  }

  return Status;
}
