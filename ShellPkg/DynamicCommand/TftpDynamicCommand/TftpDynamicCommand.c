/** @file
  Produce "tftp" shell dynamic command.

  Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "Tftp.h"
#include <Protocol/ShellDynamicCommand.h>

/**
  This is the shell command handler function pointer callback type.  This
  function handles the command when it is invoked in the shell.

  @param[in] This                   The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] SystemTable            The pointer to the system table.
  @param[in] ShellParameters        The parameters associated with the command.
  @param[in] Shell                  The instance of the shell protocol used in the context
                                    of processing this command.

  @return EFI_SUCCESS               the operation was sucessful
  @return other                     the operation failed.
**/
SHELL_STATUS
EFIAPI
TftpCommandHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL    *This,
  IN EFI_SYSTEM_TABLE                      *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL         *ShellParameters,
  IN EFI_SHELL_PROTOCOL                    *Shell
  )
{
  gEfiShellParametersProtocol = ShellParameters;
  gEfiShellProtocol           = Shell;
  return RunTftp (gImageHandle, SystemTable);
}

/**
  This is the command help handler function pointer callback type.  This
  function is responsible for displaying help information for the associated
  command.

  @param[in] This                   The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] Language               The pointer to the language string to use.

  @return string                    Pool allocated help string, must be freed by caller
**/
CHAR16 *
EFIAPI
TftpCommandGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL    *This,
  IN CONST CHAR8                           *Language
  )
{
  return HiiGetString (mTftpHiiHandle, STRING_TOKEN (STR_GET_HELP_TFTP), Language);
}

EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL mTftpDynamicCommand = {
  L"tftp",
  TftpCommandHandler,
  TftpCommandGetHelp
};

/**
  Entry point of Tftp Dynamic Command.

  Produce the DynamicCommand protocol to handle "tftp" command.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           Tftp command is executed sucessfully.
  @retval EFI_ABORTED           HII package was failed to initialize.
  @retval others                Other errors when executing tftp command.
**/
EFI_STATUS
EFIAPI
TftpCommandInitialize (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
  )
{
  EFI_STATUS                  Status;
  mTftpHiiHandle = InitializeHiiPackage (ImageHandle);
  if (mTftpHiiHandle == NULL) {
    return EFI_ABORTED;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mTftpDynamicCommand
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Tftp driver unload handler.

  @param ImageHandle            The image handle of the process.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.
**/
EFI_STATUS
EFIAPI
TftpUnload (
  IN EFI_HANDLE               ImageHandle
)
{
  EFI_STATUS                  Status;
  Status = gBS->UninstallProtocolInterface (
                  ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  &mTftpDynamicCommand
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  HiiRemovePackages (mTftpHiiHandle);
  return EFI_SUCCESS;
}
