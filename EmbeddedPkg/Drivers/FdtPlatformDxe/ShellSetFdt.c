/** @file

  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FdtPlatform.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-i", TypeFlag },
  {NULL , TypeMax  }
};

/**
  Display FDT device paths.

  Display in text form the device paths used to install the FDT from the
  highest to the lowest priority.

**/
STATIC
VOID
DisplayFdtDevicePaths (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;
  CHAR16      *TextDevicePath;
  CHAR16      *TextDevicePaths;
  CHAR16      *TextDevicePathSeparator;

  ShellPrintHiiEx (
    -1, -1, NULL,
    STRING_TOKEN (STR_SETFDT_DEVICE_PATH_LIST),
    mFdtPlatformDxeHiiHandle
    );

  if (FeaturePcdGet (PcdOverridePlatformFdt)) {
    DataSize = 0;
    Status = gRT->GetVariable (
                    L"Fdt",
                    &gFdtVariableGuid,
                    NULL,
                    &DataSize,
                    NULL
                    );

    //
    // Keep going only if the "Fdt" variable is defined.
    //

    if (Status == EFI_BUFFER_TOO_SMALL) {
      TextDevicePath = AllocatePool (DataSize);
      if (TextDevicePath == NULL) {
        return;
      }

      Status = gRT->GetVariable (
                      L"Fdt",
                      &gFdtVariableGuid,
                      NULL,
                      &DataSize,
                      TextDevicePath
                      );
      if (!EFI_ERROR (Status)) {
        ShellPrintHiiEx (
          -1, -1, NULL,
          STRING_TOKEN (STR_SETFDT_DEVICE_PATH),
          mFdtPlatformDxeHiiHandle,
          TextDevicePath
          );
      }

      FreePool (TextDevicePath);
    }
  }

  //
  // Loop over the device path list provided by "PcdFdtDevicePaths". The device
  // paths are in text form and separated by a semi-colon.
  //

  TextDevicePaths = AllocateCopyPool (
                      StrSize ((CHAR16*)PcdGetPtr (PcdFdtDevicePaths)),
                      (CHAR16*)PcdGetPtr (PcdFdtDevicePaths)
                      );
  if (TextDevicePaths == NULL) {
    return;
  }

  for (TextDevicePath = TextDevicePaths;
       *TextDevicePath != L'\0'        ; ) {
    TextDevicePathSeparator = StrStr (TextDevicePath, L";");

    if (TextDevicePathSeparator != NULL) {
      *TextDevicePathSeparator = L'\0';
    }

    ShellPrintHiiEx (
      -1, -1, NULL,
      STRING_TOKEN (STR_SETFDT_DEVICE_PATH),
      mFdtPlatformDxeHiiHandle,
      TextDevicePath
      );

    if (TextDevicePathSeparator == NULL) {
      break;
    }
    TextDevicePath = TextDevicePathSeparator + 1;
  }

  FreePool (TextDevicePaths);
}

/**
  Update the text device path stored in the "Fdt" UEFI variable given
  an EFI Shell file path or a text device path.

  This function is a subroutine of the ShellDynCmdSetFdtHandler() function
  to make its code easier to read.

  @param[in]  Shell          The instance of the shell protocol used in the
                             context of processing the "setfdt" command.
  @param[in]  FilePath       EFI Shell path or the device path to the FDT file.

  @return  SHELL_SUCCESS            The text device path was succesfully updated.
  @return  SHELL_INVALID_PARAMETER  The Shell file path is not valid.
  @return  SHELL_OUT_OF_RESOURCES   A memory allocation failed.
  @return  SHELL_DEVICE_ERROR       The "Fdt" variable could not be saved due to a hardware failure.
  @return  SHELL_ACCESS_DENIED      The "Fdt" variable is read-only.
  @return  SHELL_ACCESS_DENIED      The "Fdt" variable cannot be deleted.
  @return  SHELL_ACCESS_DENIED      The "Fdt" variable could not be written due to security violation.
  @return  SHELL_NOT_FOUND          Device path to text protocol not found.
  @return  SHELL_ABORTED            Operation aborted.

**/
STATIC
SHELL_STATUS
UpdateFdtTextDevicePath (
  IN EFI_SHELL_PROTOCOL  *Shell,
  IN CONST CHAR16        *FilePath
  )
{
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH                     *DevicePath;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL    *EfiDevicePathToTextProtocol;
  CHAR16                              *TextDevicePath;
  CHAR16                              *FdtVariableValue;
  EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL  *EfiDevicePathFromTextProtocol;
  SHELL_STATUS                        ShellStatus;

  ASSERT (FilePath != NULL);
  DevicePath       = NULL;
  TextDevicePath   = NULL;
  FdtVariableValue = NULL;

  if (*FilePath != L'\0') {
    DevicePath = Shell->GetDevicePathFromFilePath (FilePath);
    if (DevicePath != NULL) {
      Status = gBS->LocateProtocol (
                      &gEfiDevicePathToTextProtocolGuid,
                      NULL,
                      (VOID **)&EfiDevicePathToTextProtocol
                      );
      if (EFI_ERROR (Status)) {
        goto Error;
      }

      TextDevicePath = EfiDevicePathToTextProtocol->ConvertDevicePathToText (
                                                      DevicePath,
                                                      FALSE,
                                                      FALSE
                                                      );
      if (TextDevicePath == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error;
      }
      FdtVariableValue = TextDevicePath;
    } else {
      //
      // Try to convert back the EFI Device Path String into a EFI device Path
      // to ensure the format is valid
      //
      Status = gBS->LocateProtocol (
                      &gEfiDevicePathFromTextProtocolGuid,
                      NULL,
                      (VOID **)&EfiDevicePathFromTextProtocol
                      );
      if (EFI_ERROR (Status)) {
        goto Error;
      }

      DevicePath = EfiDevicePathFromTextProtocol->ConvertTextToDevicePath (
                                                    FilePath
                                                    );
      if (DevicePath == NULL) {
        Status = EFI_INVALID_PARAMETER;
        goto Error;
      }
      FdtVariableValue = (CHAR16*)FilePath;
    }
  }

  Status = gRT->SetVariable (
                  (CHAR16*)L"Fdt",
                  &gFdtVariableGuid,
                  EFI_VARIABLE_RUNTIME_ACCESS    |
                  EFI_VARIABLE_NON_VOLATILE      |
                  EFI_VARIABLE_BOOTSERVICE_ACCESS ,
                  (FdtVariableValue != NULL) ?
                  StrSize (FdtVariableValue) : 0,
                  FdtVariableValue
                  );

Error:
  ShellStatus = EfiCodeToShellCode (Status);
  if (!EFI_ERROR (Status)) {
    if (FdtVariableValue != NULL) {
      ShellPrintHiiEx (
        -1, -1, NULL,
        STRING_TOKEN (STR_SETFDT_UPDATE_SUCCEEDED),
        mFdtPlatformDxeHiiHandle,
        FdtVariableValue
        );
    } else {
      ShellPrintHiiEx (
        -1, -1, NULL,
        STRING_TOKEN (STR_SETFDT_UPDATE_DELETED),
        mFdtPlatformDxeHiiHandle
        );
    }
  } else {
    if (Status == EFI_INVALID_PARAMETER) {
      ShellPrintHiiEx (
        -1, -1, NULL,
        STRING_TOKEN (STR_SETFDT_INVALID_PATH),
        mFdtPlatformDxeHiiHandle,
        FilePath
        );
    } else {
      ShellPrintHiiEx (
        -1, -1, NULL,
        STRING_TOKEN (STR_SETFDT_ERROR),
        mFdtPlatformDxeHiiHandle,
        Status
        );
    }
  }

  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }
  if (TextDevicePath != NULL) {
    FreePool (TextDevicePath);
  }

  return ShellStatus;
}

/**
  This is the shell command "setfdt" handler function. This function handles
  the command when it is invoked in the shell.

  @param[in]  This             The instance of the
                               EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in]  SystemTable      The pointer to the UEFI system table.
  @param[in]  ShellParameters  The parameters associated with the command.
  @param[in]  Shell            The instance of the shell protocol used in the
                               context of processing this command.

  @return  SHELL_SUCCESS            The operation was successful.
  @return  SHELL_ABORTED            Operation aborted due to internal error.
  @return  SHELL_INVALID_PARAMETER  The parameters of the command are not valid.
  @return  SHELL_INVALID_PARAMETER  The EFI Shell file path is not valid.
  @return  SHELL_NOT_FOUND          Failed to locate a protocol or a file.
  @return  SHELL_UNSUPPORTED        Device path not supported.
  @return  SHELL_OUT_OF_RESOURCES   A memory allocation failed.
  @return  SHELL_DEVICE_ERROR       The "Fdt" variable could not be saved due to a hardware failure.
  @return  SHELL_ACCESS_DENIED      The "Fdt" variable is read-only.
  @return  SHELL_ACCESS_DENIED      The "Fdt" variable cannot be deleted.
  @return  SHELL_ACCESS_DENIED      The "Fdt" variable could not be written due to security violation.

**/
SHELL_STATUS
EFIAPI
ShellDynCmdSetFdtHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters,
  IN EFI_SHELL_PROTOCOL                  *Shell
  )
{
  SHELL_STATUS  ShellStatus;
  EFI_STATUS    Status;
  LIST_ENTRY    *ParamPackage;
  BOOLEAN       FilePath;
  CONST CHAR16  *ValueStr;
  CHAR16        *TextDevicePath;

  ShellStatus  = SHELL_SUCCESS;
  ParamPackage = NULL;
  FilePath     = FALSE;

  //
  // Install the Shell and Shell Parameters Protocols on the driver
  // image. This is necessary for the initialisation of the Shell
  // Library to succeed in the next step.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gImageHandle,
                  &gEfiShellProtocolGuid, Shell,
                  &gEfiShellParametersProtocolGuid, ShellParameters,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return SHELL_ABORTED;
  }

  //
  // Initialise the Shell Library as we are going to use it.
  // Assert that the return code is EFI_SUCCESS as it should.
  // To anticipate any change is the codes returned by
  // ShellInitialize(), leave in case of error.
  //
  Status = ShellInitialize ();
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return SHELL_ABORTED;
  }

  Status = ShellCommandLineParse (ParamList, &ParamPackage, NULL, TRUE);
  if (!EFI_ERROR (Status)) {
    switch (ShellCommandLineGetCount (ParamPackage)) {
    case 1:
      //
      // Case "setfdt" or "setfdt -i"
      //
      if (!ShellCommandLineGetFlag (ParamPackage, L"-i")) {
        DisplayFdtDevicePaths ();
      }
      break;

    case 2:
      //
      // Case "setfdt file_path"    or
      //      "setfdt -i file_path" or
      //      "setfdt file_path -i"
      //
      FilePath = TRUE;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
    }
  }
  if (EFI_ERROR (Status)) {
    ShellStatus = EfiCodeToShellCode (Status);
    ShellPrintHiiEx (
      -1, -1, NULL,
      STRING_TOKEN (STR_SETFDT_ERROR),
      mFdtPlatformDxeHiiHandle,
      Status
      );
    goto Error;
  }

  //
  // Update the preferred device path for the FDT if asked for.
  //
  if (FilePath) {
    ValueStr = ShellCommandLineGetRawValue (ParamPackage, 1);
    ShellPrintHiiEx (
      -1, -1, NULL,
      STRING_TOKEN (STR_SETFDT_UPDATING),
      mFdtPlatformDxeHiiHandle
      );
    ShellStatus = UpdateFdtTextDevicePath (Shell, ValueStr);
    if (ShellStatus != SHELL_SUCCESS) {
      goto Error;
    }
  }

  //
  // Run the FDT installation process if asked for.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-i")) {
    ShellPrintHiiEx (
      -1, -1, NULL,
      STRING_TOKEN (STR_SETFDT_INSTALLING),
      mFdtPlatformDxeHiiHandle
      );
    Status = RunFdtInstallation (&TextDevicePath);
    ShellStatus = EfiCodeToShellCode (Status);
    if (!EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1, -1, NULL,
        STRING_TOKEN (STR_SETFDT_INSTALL_SUCCEEDED),
        mFdtPlatformDxeHiiHandle,
        TextDevicePath
        );
      FreePool (TextDevicePath);
    } else {
      if (Status == EFI_INVALID_PARAMETER) {
        ShellPrintHiiEx (
          -1, -1, NULL,
          STRING_TOKEN (STR_SETFDT_INVALID_DEVICE_PATH),
          mFdtPlatformDxeHiiHandle
          );
      } else {
        ShellPrintHiiEx (
          -1, -1, NULL,
          STRING_TOKEN (STR_SETFDT_ERROR),
          mFdtPlatformDxeHiiHandle,
          Status
          );
      }
      DisplayFdtDevicePaths ();
    }
  }

Error:
  gBS->UninstallMultipleProtocolInterfaces (
         gImageHandle,
         &gEfiShellProtocolGuid, Shell,
         &gEfiShellParametersProtocolGuid, ShellParameters,
         NULL
         );
  ShellCommandLineFreeVarList (ParamPackage);

  return ShellStatus;
}

/**
  This is the shell command "setfdt" help handler function. This
  function returns the formatted help for the "setfdt" command.
  The format matchs that in Appendix B of the revision 2.1 of the
  UEFI Shell Specification.

  @param[in]  This      The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in]  Language  The pointer to the language string to use.

  @return  CHAR16*  Pool allocated help string, must be freed by caller.
**/
CHAR16*
EFIAPI
ShellDynCmdSetFdtGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  )
{
  //
  // This allocates memory. The caller has to free the allocated memory.
  //
  return HiiGetString (
                mFdtPlatformDxeHiiHandle,
                STRING_TOKEN (STR_GET_HELP_SETFDT),
                Language
                );
}
