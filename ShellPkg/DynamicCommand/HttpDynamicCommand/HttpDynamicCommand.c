/** @file
  Produce "http" shell dynamic command.

  Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2020, Broadcom. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Protocol/ShellDynamicCommand.h>
#include "Http.h"

/**
  This is the shell command handler function pointer callback type.  This
  function handles the command when it is invoked in the shell.

  @param[in] This               The instance of the
                                EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] SystemTable        The pointer to the system table.
  @param[in] ShellParameters    The parameters associated with the command.
  @param[in] Shell              The instance of the shell protocol used in
                                the context of processing this command.

  @return EFI_SUCCESS           the operation was sucessful
  @return other                 the operation failed.
**/
SHELL_STATUS
EFIAPI
HttpCommandHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters,
  IN EFI_SHELL_PROTOCOL                  *Shell
  )
{
  gEfiShellParametersProtocol = ShellParameters;
  gEfiShellProtocol           = Shell;

  return RunHttp (gImageHandle, SystemTable);
}

/**
  This is the command help handler function pointer callback type.  This
  function is responsible for displaying help information for the associated
  command.

  @param[in] This        The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] Language    The pointer to the language string to use.

  @return string         Pool allocated help string, must be freed by caller
**/
CHAR16 *
EFIAPI
HttpCommandGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  )
{
  return HiiGetString (
           mHttpHiiHandle,
           STRING_TOKEN (STR_GET_HELP_HTTP),
           Language
           );
}

EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  mHttpDynamicCommand = {
  HTTP_APP_NAME,
  HttpCommandHandler,
  HttpCommandGetHelp
};

/**
  Entry point of Http Dynamic Command.

  Produce the DynamicCommand protocol to handle "http" command.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           Http command is executed sucessfully.
  @retval EFI_ABORTED           HII package was failed to initialize.
  @retval others                Other errors when executing http command.
**/
EFI_STATUS
EFIAPI
HttpCommandInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mHttpHiiHandle = InitializeHiiPackage (ImageHandle);
  if (mHttpHiiHandle == NULL) {
    return EFI_ABORTED;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mHttpDynamicCommand
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Http driver unload handler.

  @param ImageHandle            The image handle of the process.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.
**/
EFI_STATUS
EFIAPI
HttpUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallProtocolInterface (
                  ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  &mHttpDynamicCommand
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiRemovePackages (mHttpHiiHandle);

  return EFI_SUCCESS;
}
