/** @file
  Produce "mput" shell dynamic command.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/HiiLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/ShellDynamicCommand.h>
#include "EfiMpServicesUnitTestCommom.h"

EFI_HII_HANDLE  mMpUtHiiHandle;

EFI_STATUS
EFIAPI
EfiMpServiceProtocolUnitTest (
  VOID
  );

/**
  Retrieve HII package list from ImageHandle and publish to HII database.

  @param ImageHandle            The image handle of the process.

  @return HII handle.
**/
EFI_HII_HANDLE
InitializeHiiPackage (
  EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageList;
  EFI_HII_HANDLE               HiiHandle;

  //
  // Retrieve HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Publish HII package list to HII Database.
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           NULL,
                           &HiiHandle
                           );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return HiiHandle;
}

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
MpUtCommandHandler (
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
MpUtCommandGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  )
{
  return HiiGetString (mMpUtHiiHandle, STRING_TOKEN (STR_GET_HELP_MPUT), Language);
}

EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  mMpUtDynamicCommand = {
  L"mput",
  MpUtCommandHandler,
  MpUtCommandGetHelp
};

/**
  Entry point of mput Dynamic Command.

  Produce the DynamicCommand protocol to handle "mput" command.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           Tftp command is executed successfully.
  @retval EFI_ABORTED           HII package was failed to initialize.
  @retval others                Other errors when executing mput command.
**/
EFI_STATUS
EFIAPI
MpUtCommandInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mMpUtHiiHandle = InitializeHiiPackage (ImageHandle);
  if (mMpUtHiiHandle == NULL) {
    return EFI_ABORTED;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMpUtDynamicCommand
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
MpUtUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallProtocolInterface (
                  ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  &mMpUtDynamicCommand
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiRemovePackages (mMpUtHiiHandle);

  return EFI_SUCCESS;
}
