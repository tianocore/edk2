/** @file
  Produce "MpProtocolUnitTest" shell dynamic command.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ShellDynamicCommand.h>
#include "EfiMpServicesUnitTestCommom.h"

CHAR16  *mMpProtocolUnitTestCommandHelp = L".TH MpProtocolUnitTest 0\r\n.SH NAME\r\nDisplay unit test results of EFI MP services protocol.\r\n";

EFI_STATUS
EFIAPI
EfiMpServiceProtocolUnitTest (
  VOID
  );

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
MpProtocolUnitTestCommandHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters,
  IN EFI_SHELL_PROTOCOL                  *Shell
  )
{
  return EfiMpServiceProtocolUnitTest ();
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
MpProtocolUnitTestCommandGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  )
{
  return AllocateCopyPool (StrSize (mMpProtocolUnitTestCommandHelp), mMpProtocolUnitTestCommandHelp);
}

EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  mMpProtocolUnitTestDynamicCommand = {
  L"MpProtocolUnitTest",
  MpProtocolUnitTestCommandHandler,
  MpProtocolUnitTestCommandGetHelp
};

/**
  Entry point of MpProtocolUnitTest Dynamic Command.

  Produce the DynamicCommand protocol to handle "MpProtocolUnitTest" command.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           Tftp command is executed successfully.
  @retval EFI_ABORTED           HII package was failed to initialize.
  @retval others                Other errors when executing MpProtocolUnitTest command.
**/
EFI_STATUS
EFIAPI
MpProtocolUnitTestCommandInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMpProtocolUnitTestDynamicCommand
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Driver unload handler.

  @param ImageHandle            The image handle of the process.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.
**/
EFI_STATUS
EFIAPI
MpProtocolUnitTestUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallProtocolInterface (
                  ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  &mMpProtocolUnitTestDynamicCommand
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
