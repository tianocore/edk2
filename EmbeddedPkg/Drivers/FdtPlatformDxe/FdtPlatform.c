/** @file

  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/BdsLib.h>

#include <Protocol/DevicePathFromText.h>
#include <Protocol/DevicePath.h>

#include <Guid/EventGroup.h>
#include <Guid/Fdt.h>

#include <libfdt.h>

//
// Internal types
//
STATIC VOID OnEndOfDxe (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );
STATIC EFI_STATUS RunFdtInstallation (
  VOID
  );
STATIC EFI_STATUS InstallFdt (
  IN CONST CHAR16*  TextDevicePath
  );

/**
  Main entry point of the FDT platform driver.

  @param[in]  ImageHandle   The firmware allocated handle for the present driver
                            UEFI image.
  @param[in]  *SystemTable  A pointer to the EFI System table.

  @retval  EFI_SUCCESS           The driver was initialized.
  @retval  EFI_OUT_OF_RESOURCES  The "End of DXE" event could not be allocated.

**/
EFI_STATUS
FdtPlatformEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   EndOfDxeEvent;

  //
  // Create an event belonging to the "gEfiEndOfDxeEventGroupGuid" group.
  // The "OnEndOfDxe()" function is declared as the call back function.
  // It will be called at the end of the DXE phase when an event of the
  // same group is signalled to inform about the end of the DXE phase.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnEndOfDxe,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );

  return Status;
}

/**
  Notification function of the event defined as belonging to the
  EFI_END_OF_DXE_EVENT_GROUP_GUID event group that was created in
  the entry point of the driver.

  This function is called when an event belonging to the
  EFI_END_OF_DXE_EVENT_GROUP_GUID event group is signalled. Such an
  event is signalled once at the end of the dispatching of all
  drivers (end of the so called DXE phase).

  @param[in]  Event    Event declared in the entry point of the driver whose
                       notification function is being invoked.
  @param[in]  Context  NULL

**/
STATIC
VOID
OnEndOfDxe (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  RunFdtInstallation ();
  gBS->CloseEvent (Event);
}

/**
  Run the FDT installation process.

  Loop in priority order over the device paths from which the FDT has
  been asked to be retrieved for. For each device path, try to install
  the FDT. Stop as soon as an installation succeeds.

  @retval  EFI_SUCCESS            The FDT was installed.
  @retval  EFI_NOT_FOUND          Failed to locate a protocol or a file.
  @retval  EFI_INVALID_PARAMETER  Invalid device path.
  @retval  EFI_UNSUPPORTED        Device path not supported.
  @retval  EFI_OUT_OF_RESOURCES   An allocation failed.

**/
STATIC
EFI_STATUS
RunFdtInstallation (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;
  VOID        *Data;
  CHAR16      *TextDevicePathStart;
  CHAR16      *TextDevicePathSeparator;
  UINTN       TextDevicePathLen;
  CHAR16      *TextDevicePath;

  //
  // For development purpose, if enabled through the "PcdOverridePlatformFdt"
  // feature PCD, try first to install the FDT specified by the device path in
  // text form stored in the "Fdt" UEFI variable.
  //
  if (FeaturePcdGet (PcdOverridePlatformFdt)) {
    Data     = NULL;
    DataSize = 0;
    Status = gRT->GetVariable (
                    L"Fdt",
                    &gFdtVariableGuid,
                    NULL,
                    &DataSize,
                    Data
                    );

    //
    // Keep going only if the "Fdt" variable is defined.
    //

    if (Status == EFI_BUFFER_TOO_SMALL) {
      Data = AllocatePool (DataSize);
      if (Data == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        Status = gRT->GetVariable (
                        L"Fdt",
                        &gFdtVariableGuid,
                        NULL,
                        &DataSize,
                        Data
                        );
        if (!EFI_ERROR (Status)) {
          Status = InstallFdt ((CHAR16*)Data);
          if (!EFI_ERROR (Status)) {
            DEBUG ((
              EFI_D_WARN,
              "Installation of the FDT using the device path <%s> completed.\n",
              (CHAR16*)Data
              ));
          }
        }
        FreePool (Data);
      }

      if (EFI_ERROR (Status)) {
        DEBUG ((
          EFI_D_ERROR,
          "Installation of the FDT specified by the \"Fdt\" UEFI variable failed - %r\n",
          Status
          ));
      } else {
        return Status;
      }
    }
  }

  //
  // Loop over the device path list provided by "PcdFdtDevicePaths". The device
  // paths are in text form and separated by a semi-colon.
  //

  Status = EFI_SUCCESS;
  for (TextDevicePathStart = (CHAR16*)PcdGetPtr (PcdFdtDevicePaths);
       *TextDevicePathStart != L'\0'                               ; ) {
    TextDevicePathSeparator = StrStr (TextDevicePathStart, L";");

    //
    // Last device path of the list
    //
    if (TextDevicePathSeparator == NULL) {
      TextDevicePath = TextDevicePathStart;
    } else {
      TextDevicePathLen = (UINTN)(TextDevicePathSeparator - TextDevicePathStart);
      TextDevicePath = AllocateCopyPool (
                         (TextDevicePathLen + 1) * sizeof (CHAR16),
                         TextDevicePathStart
                         );
      if (TextDevicePath == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((EFI_D_ERROR, "Memory allocation error during FDT installation process.\n"));
        break;
      }
      TextDevicePath[TextDevicePathLen] = L'\0';
    }

    Status = InstallFdt (TextDevicePath);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_WARN, "Installation of the FDT using the device path <%s> failed - %r.\n",
        TextDevicePath, Status
        ));
    } else {
      DEBUG ((EFI_D_WARN, "Installation of the FDT using the device path <%s> completed.\n",
        TextDevicePath
        ));
    }

    if (TextDevicePathSeparator == NULL) {
      break;
    } else {
      FreePool (TextDevicePath);
      if (!EFI_ERROR (Status)) {
        break;
      }
      TextDevicePathStart = TextDevicePathSeparator + 1;
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to install the FDT - %r.\n", Status));
  }

  return Status;
}

/**
  Install the FDT specified by its device path in text form.

  @param[in]  TextDevicePath  Device path of the FDT to install in text form

  @retval  EFI_SUCCESS            The FDT was installed.
  @retval  EFI_NOT_FOUND          Failed to locate a protocol or a file.
  @retval  EFI_INVALID_PARAMETER  Invalid device path.
  @retval  EFI_UNSUPPORTED        Device path not supported.
  @retval  EFI_OUT_OF_RESOURCES   An allocation failed.
**/
STATIC
EFI_STATUS
InstallFdt (
  IN CONST CHAR16*  TextDevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL  *EfiDevicePathFromTextProtocol;
  EFI_DEVICE_PATH                     *DevicePath;
  EFI_PHYSICAL_ADDRESS                FdtBlobBase;
  UINTN                               FdtBlobSize;
  UINTN                               NbPages;
  EFI_PHYSICAL_ADDRESS                RsFdtBlobBase;

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathFromTextProtocolGuid,
                  NULL,
                  (VOID **)&EfiDevicePathFromTextProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "InstallFdt() - Failed to locate EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL protocol\n"));
    return Status;
  }

  DevicePath = (EFI_DEVICE_PATH*)EfiDevicePathFromTextProtocol->ConvertTextToDevicePath (TextDevicePath);
  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Load the FDT given its device path.
  // This operation may fail if the device path is not supported.
  //
  FdtBlobBase = 0;
  NbPages     = 0;
  Status = BdsLoadImage (DevicePath, AllocateAnyPages, &FdtBlobBase, &FdtBlobSize);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  // Check the FDT header is valid. We only make this check in DEBUG mode in
  // case the FDT header change on production device and this ASSERT() becomes
  // not valid.
  ASSERT (fdt_check_header ((VOID*)(UINTN)FdtBlobBase) == 0);

  //
  // Ensure the Size of the Device Tree is smaller than the size of the read file
  //
  ASSERT ((UINTN)fdt_totalsize ((VOID*)(UINTN)FdtBlobBase) <= FdtBlobSize);

  //
  // Store the FDT as Runtime Service Data to prevent the Kernel from
  // overwritting its data.
  //
  NbPages = EFI_SIZE_TO_PAGES (FdtBlobSize);
  Status = gBS->AllocatePages (
                  AllocateAnyPages, EfiRuntimeServicesData,
                  NbPages, &RsFdtBlobBase
                  );
  if (EFI_ERROR (Status)) {
    goto Error;
  }
  CopyMem (
    (VOID*)((UINTN)RsFdtBlobBase),
    (VOID*)((UINTN)FdtBlobBase),
    FdtBlobSize
    );

  //
  // Install the FDT into the Configuration Table
  //
  Status = gBS->InstallConfigurationTable (
                  &gFdtTableGuid,
                  (VOID*)((UINTN)RsFdtBlobBase)
                  );
  if (EFI_ERROR (Status)) {
    gBS->FreePages (RsFdtBlobBase, NbPages);
  }

Error :

  if (FdtBlobBase != 0) {
    gBS->FreePages (FdtBlobBase, NbPages);
  }
  FreePool (DevicePath);

  return Status;
}

=======

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
STATIC
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
      // Case "setfdt -i"
      //
      if (!ShellCommandLineGetFlag (ParamPackage, L"-i")) {
        Status = EFI_INVALID_PARAMETER;
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
    Status = RunFdtInstallation ();
    ShellStatus = EfiCodeToShellCode (Status);
    if (!EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1, -1, NULL,
        STRING_TOKEN (STR_SETFDT_INSTALL_SUCCEEDED),
        mFdtPlatformDxeHiiHandle
        );
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
STATIC
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
  TextDevicePath   = NULL;
  FdtVariableValue = NULL;

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

  Status = gRT->SetVariable (
                  (CHAR16*)L"Fdt",
                  &gFdtVariableGuid,
                  EFI_VARIABLE_RUNTIME_ACCESS    |
                  EFI_VARIABLE_NON_VOLATILE      |
                  EFI_VARIABLE_BOOTSERVICE_ACCESS ,
                  StrSize (FdtVariableValue),
                  FdtVariableValue
                  );

Error:
  ShellStatus = EfiCodeToShellCode (Status);
  if (!EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1, -1, NULL,
      STRING_TOKEN (STR_SETFDT_UPDATE_SUCCEEDED),
      mFdtPlatformDxeHiiHandle,
      FdtVariableValue
      );
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
  Transcode one of the EFI return code used by the model into an EFI Shell return code.

  @param[in]  Status  EFI return code.

  @return  Transcoded EFI Shell return code.

**/
STATIC
SHELL_STATUS
EfiCodeToShellCode (
  IN EFI_STATUS  Status
  )
{
  SHELL_STATUS  ShellStatus;

  switch (Status) {
  case EFI_SUCCESS :
    ShellStatus = SHELL_SUCCESS;
    break;

  case EFI_INVALID_PARAMETER :
    ShellStatus = SHELL_INVALID_PARAMETER;
    break;

  case EFI_UNSUPPORTED :
    ShellStatus = SHELL_UNSUPPORTED;
    break;

  case EFI_DEVICE_ERROR :
    ShellStatus = SHELL_DEVICE_ERROR;
    break;

  case EFI_WRITE_PROTECTED    :
  case EFI_SECURITY_VIOLATION :
    ShellStatus = SHELL_ACCESS_DENIED;
    break;

  case EFI_OUT_OF_RESOURCES :
    ShellStatus = SHELL_OUT_OF_RESOURCES;
    break;

  case EFI_NOT_FOUND :
    ShellStatus = SHELL_NOT_FOUND;
    break;

  default :
    ShellStatus = SHELL_ABORTED;
  }

  return ShellStatus;
}
>>>>>>> 4ac4fed... EmbeddedPkg/FdtPlatformDxe: Fix typo issue
