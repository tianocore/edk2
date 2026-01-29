/** @file
  Functionality specific for dynamic UEFI shell command support.

  This command can provide detailed UEFI variable policy configuration
  information in the UEFI shell.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariablePolicy.h"

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/ShellDynamicCommand.h>

extern EFI_HII_HANDLE  mVarPolicyShellCommandHiiHandle;

/**
  This is the shell command handler function pointer callback type.

  This function handles the command when it is invoked in the shell.

  @param[in] This                   The instance of the
                                    EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] SystemTable            The pointer to the system table.
  @param[in] ShellParameters        The parameters associated with the command.
  @param[in] Shell                  The instance of the shell protocol used in
                                    the context of processing this command.

  @return EFI_SUCCESS               the operation was successful
  @return other                     the operation failed.

**/
SHELL_STATUS
EFIAPI
VarPolicyCommandHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters,
  IN EFI_SHELL_PROTOCOL                  *Shell
  )
{
  gEfiShellParametersProtocol = ShellParameters;
  gEfiShellProtocol           = Shell;

  return RunVarPolicy (gImageHandle, SystemTable);
}

/**
  This is the command help handler function pointer callback type.  This
  function is responsible for displaying help information for the associated
  command.

  @param[in] This                   The instance of the
                                    EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] Language               The pointer to the language string to use.

  @return string                    Pool allocated help string, must be freed
                                    by caller.

**/
STATIC
CHAR16 *
EFIAPI
VarPolicyCommandGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  )
{
  return HiiGetString (
           mVarPolicyShellCommandHiiHandle,
           STRING_TOKEN (STR_GET_HELP_VAR_POLICY),
           Language
           );
}

STATIC EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  mVarPolicyDynamicCommand = {
  VAR_POLICY_COMMAND_NAME,
  VarPolicyCommandHandler,
  VarPolicyCommandGetHelp
};

/**
  Entry point of the UEFI variable policy dynamic shell command.

  Produce the Dynamic Command Protocol to handle the "varpolicy" command.

  @param[in] ImageHandle        The image handle of the process.
  @param[in] SystemTable        The EFI System Table pointer.

  @retval EFI_SUCCESS           The "varpolicy" command executed successfully.
  @retval EFI_ABORTED           HII package failed to initialize.
  @retval others                Other errors when executing "varpolicy" command.

**/
EFI_STATUS
EFIAPI
VariablePolicyDynamicCommandEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mVarPolicyShellCommandHiiHandle = InitializeHiiPackage (ImageHandle);
  if (mVarPolicyShellCommandHiiHandle == NULL) {
    return EFI_ABORTED;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mVarPolicyDynamicCommand
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Unload the dynamic "varpolicy" UEFI Shell command.

  @param[in] ImageHandle        The image handle of the process.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
VariablePolicyDynamicCommandUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallProtocolInterface (
                  ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  &mVarPolicyDynamicCommand
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiRemovePackages (mVarPolicyShellCommandHiiHandle);

  return EFI_SUCCESS;
}
