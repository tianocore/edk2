/** @file
  Produce "dp" shell dynamic command.

  Copyright (c) 2017, Intel Corporation. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "Dp.h"
#include <Protocol/ShellDynamicCommand.h>

/**
  This is the shell command handler function pointer callback type.  This
  function handles the command when it is invoked in the shell.

  @param[in] This                   The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] SystemTable            The pointer to the system table.
  @param[in] ShellParameters        The parameters associated with the command.
  @param[in] Shell                  The instance of the shell protocol used in the context
                                    of processing this command.

  @return EFI_SUCCESS               the operation was successful
  @return other                     the operation failed.
**/
SHELL_STATUS
EFIAPI
DpCommandHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters,
  IN EFI_SHELL_PROTOCOL                  *Shell
  )
{
  gEfiShellParametersProtocol = ShellParameters;
  gEfiShellProtocol           = Shell;
  return RunDp (gImageHandle, SystemTable);
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
DpCommandGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  )
{
  return HiiGetString (mDpHiiHandle, STRING_TOKEN (STR_GET_HELP_DP), Language);
}

EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  mDpDynamicCommand = {
  L"dp",
  DpCommandHandler,
  DpCommandGetHelp
};

/**
  Entry point of Tftp Dynamic Command.

  Produce the DynamicCommand protocol to handle "tftp" command.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           Tftp command is executed successfully.
  @retval EFI_ABORTED           HII package was failed to initialize.
  @retval others                Other errors when executing tftp command.
**/
EFI_STATUS
EFIAPI
DpCommandInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mDpHiiHandle = InitializeHiiPackage (ImageHandle);
  if (mDpHiiHandle == NULL) {
    return EFI_ABORTED;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mDpDynamicCommand
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
DpUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallProtocolInterface (
                  ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  &mDpDynamicCommand
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiRemovePackages (mDpHiiHandle);
  return EFI_SUCCESS;
}
