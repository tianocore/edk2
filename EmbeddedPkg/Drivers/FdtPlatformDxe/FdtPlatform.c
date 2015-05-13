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

#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BdsLib.h>

#include <Protocol/DevicePath.h>

//
// Internal variables
//

STATIC CONST EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL mShellDynCmdProtocolSetFdt = {
    L"setfdt",                // Name of the command
    ShellDynCmdSetFdtHandler, // Handler
    ShellDynCmdSetFdtGetHelp  // GetHelp
};

STATIC CONST EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL mShellDynCmdProtocolDumpFdt = {
    L"dumpfdt",                // Name of the command
    ShellDynCmdDumpFdtHandler, // Handler
    ShellDynCmdDumpFdtGetHelp  // GetHelp
};

STATIC CONST EFI_GUID  mFdtPlatformDxeHiiGuid = {
                         0x8afa7610, 0x62b1, 0x46aa,
                         {0xb5, 0x34, 0xc3, 0xde, 0xff, 0x39, 0x77, 0x8c}
                         };

EFI_HANDLE mFdtPlatformDxeHiiHandle;

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
  UINTN                               NumPages;
  EFI_PHYSICAL_ADDRESS                FdtConfigurationTableBase;

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
  NumPages    = 0;
  Status = BdsLoadImage (DevicePath, AllocateAnyPages, &FdtBlobBase, &FdtBlobSize);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Ensure that the FDT header is valid and that the Size of the Device Tree
  // is smaller than the size of the read file
  //
  if (fdt_check_header ((VOID*)(UINTN)FdtBlobBase) != 0 ||
      (UINTN)fdt_totalsize ((VOID*)(UINTN)FdtBlobBase) > FdtBlobSize) {
    DEBUG ((EFI_D_ERROR, "InstallFdt() - loaded FDT binary image seems corrupt\n"));
    Status = EFI_LOAD_ERROR;
    goto Error;
  }

  //
  // Store the FDT as Runtime Service Data to prevent the Kernel from
  // overwritting its data.
  //
  NumPages = EFI_SIZE_TO_PAGES (FdtBlobSize);
  Status = gBS->AllocatePages (
                  AllocateAnyPages, EfiRuntimeServicesData,
                  NumPages, &FdtConfigurationTableBase
                  );
  if (EFI_ERROR (Status)) {
    goto Error;
  }
  CopyMem (
    (VOID*)(UINTN)FdtConfigurationTableBase,
    (VOID*)(UINTN)FdtBlobBase,
    FdtBlobSize
    );

  //
  // Install the FDT into the Configuration Table
  //
  Status = gBS->InstallConfigurationTable (
                  &gFdtTableGuid,
                  (VOID*)(UINTN)FdtConfigurationTableBase
                  );
  if (EFI_ERROR (Status)) {
    gBS->FreePages (FdtConfigurationTableBase, NumPages);
  }

Error:
  if (FdtBlobBase != 0) {
    gBS->FreePages (FdtBlobBase, NumPages);
  }
  FreePool (DevicePath);

  return Status;
}

/**
  Main entry point of the FDT platform driver.

  @param[in]  ImageHandle   The firmware allocated handle for the present driver
                            UEFI image.
  @param[in]  *SystemTable  A pointer to the EFI System table.

  @retval  EFI_SUCCESS           The driver was initialized.
  @retval  EFI_OUT_OF_RESOURCES  The "End of DXE" event could not be allocated or
                                 there was not enough memory in pool to install
                                 the Shell Dynamic Command protocol.
  @retval  EFI_LOAD_ERROR        Unable to add the HII package.

**/
EFI_STATUS
FdtPlatformEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  //
  // Install the Device Tree from its expected location
  //
  Status = RunFdtInstallation (NULL);

  if (FeaturePcdGet (PcdOverridePlatformFdt) || FeaturePcdGet (PcdDumpFdtShellCommand)) {
    //
    // Register the strings for the user interface in the HII Database.
    // This shows the way to the multi-language support, even if
    // only the English language is actually supported. The strings to register
    // are stored in the "ShellSetFdtStrings[]" array. This array is
    // built by the building process from the "*.uni" file associated to
    // the present driver (cf. FdtPlatfromDxe.inf). Examine your Build
    // folder under your package's DEBUG folder and you will find the array
    // defined in a xxxStrDefs.h file.
    //
    mFdtPlatformDxeHiiHandle = HiiAddPackages (
                                 &mFdtPlatformDxeHiiGuid,
                                 ImageHandle,
                                 FdtPlatformDxeStrings,
                                 NULL
                                 );
  }

  //
  // If the development features are enabled, install the dynamic shell
  // command "setfdt" to be able to define a device path for the FDT
  // that has precedence over the device paths defined by
  // "PcdFdtDevicePaths".
  //

  if (FeaturePcdGet (PcdOverridePlatformFdt)) {
    if (mFdtPlatformDxeHiiHandle != NULL) {
      // We install dynamic EFI command on separate handles as we cannot register
      // more than one protocol of the same protocol interface on the same handle.
      Handle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Handle,
                      &gEfiShellDynamicCommandProtocolGuid,
                      &mShellDynCmdProtocolSetFdt,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        HiiRemovePackages (mFdtPlatformDxeHiiHandle);
      }
    } else {
      Status = EFI_LOAD_ERROR;
    }
    if (EFI_ERROR (Status)) {
      DEBUG ((
        EFI_D_WARN,
        "Unable to install \"setfdt\" EFI Shell command - %r \n",
        Status
        ));
    }
  }

  if (FeaturePcdGet (PcdDumpFdtShellCommand)) {
    if (mFdtPlatformDxeHiiHandle != NULL) {
      // We install dynamic EFI command on separate handles as we cannot register
      // more than one protocol of the same protocol interface on the same handle.
      Handle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Handle,
                      &gEfiShellDynamicCommandProtocolGuid,
                      &mShellDynCmdProtocolDumpFdt,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        HiiRemovePackages (mFdtPlatformDxeHiiHandle);
      }
    } else {
      Status = EFI_LOAD_ERROR;
    }
    if (EFI_ERROR (Status)) {
      DEBUG ((
        EFI_D_WARN,
        "Unable to install \"dumpfdt\" EFI Shell command - %r \n",
        Status
        ));
    }
  }

  return Status;
}

/**
  Run the FDT installation process.

  Loop in priority order over the device paths from which the FDT has
  been asked to be retrieved for. For each device path, try to install
  the FDT. Stop as soon as an installation succeeds.

  @param[in]  SuccessfullDevicePath  If not NULL, address where to store the
                                     pointer to the text device path from
                                     which the FDT was successfully retrieved.
                                     Not used if the FDT installation failed.
                                     The returned address is the address of
                                     an allocated buffer that has to be
                                     freed by the caller.

  @retval  EFI_SUCCESS            The FDT was installed.
  @retval  EFI_NOT_FOUND          Failed to locate a protocol or a file.
  @retval  EFI_INVALID_PARAMETER  Invalid device path.
  @retval  EFI_UNSUPPORTED        Device path not supported.
  @retval  EFI_OUT_OF_RESOURCES   An allocation failed.

**/
EFI_STATUS
RunFdtInstallation (
  OUT CHAR16  **SuccessfullDevicePath
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;
  CHAR16      *TextDevicePath;
  CHAR16      *TextDevicePathStart;
  CHAR16      *TextDevicePathSeparator;
  UINTN       TextDevicePathLen;

  TextDevicePath = NULL;
  //
  // For development purpose, if enabled through the "PcdOverridePlatformFdt"
  // feature PCD, try first to install the FDT specified by the device path in
  // text form stored in the "Fdt" UEFI variable.
  //
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
        Status = EFI_OUT_OF_RESOURCES;
        goto Error;
      }

      Status = gRT->GetVariable (
                      L"Fdt",
                      &gFdtVariableGuid,
                      NULL,
                      &DataSize,
                      TextDevicePath
                      );
      if (EFI_ERROR (Status)) {
        FreePool (TextDevicePath);
        goto Error;
      }

      Status = InstallFdt (TextDevicePath);
      if (!EFI_ERROR (Status)) {
        DEBUG ((
          EFI_D_WARN,
          "Installation of the FDT using the device path <%s> completed.\n",
          TextDevicePath
          ));
        goto Done;
      }
      DEBUG ((
        EFI_D_ERROR,
        "Installation of the FDT specified by the \"Fdt\" UEFI variable failed - %r\n",
        Status
        ));
      FreePool (TextDevicePath);
    }
  }

  //
  // Loop over the device path list provided by "PcdFdtDevicePaths". The device
  // paths are in text form and separated by a semi-colon.
  //

  Status = EFI_NOT_FOUND;
  for (TextDevicePathStart = (CHAR16*)PcdGetPtr (PcdFdtDevicePaths);
       *TextDevicePathStart != L'\0'                               ; ) {
    TextDevicePathSeparator = StrStr (TextDevicePathStart, L";");

    //
    // Last device path of the list
    //
    if (TextDevicePathSeparator == NULL) {
      TextDevicePathLen = StrLen (TextDevicePathStart);
    } else {
      TextDevicePathLen = (UINTN)(TextDevicePathSeparator - TextDevicePathStart);
    }

    TextDevicePath = AllocateCopyPool (
                       (TextDevicePathLen + 1) * sizeof (CHAR16),
                       TextDevicePathStart
                       );
    if (TextDevicePath == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }
    TextDevicePath[TextDevicePathLen] = L'\0';

    Status = InstallFdt (TextDevicePath);
    if (!EFI_ERROR (Status)) {
      DEBUG ((EFI_D_WARN, "Installation of the FDT using the device path <%s> completed.\n",
        TextDevicePath
        ));
      goto Done;
    }

    DEBUG ((EFI_D_WARN, "Installation of the FDT using the device path <%s> failed - %r.\n",
      TextDevicePath, Status
      ));
    FreePool (TextDevicePath);

    if (TextDevicePathSeparator == NULL) {
      goto Error;
    }
    TextDevicePathStart = TextDevicePathSeparator + 1;
  }

Error:
Done:

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to install the FDT - %r.\n", Status));
    return Status;
  }

  if (SuccessfullDevicePath != NULL) {
    *SuccessfullDevicePath = TextDevicePath;
  } else {
    FreePool (TextDevicePath);
  }

  return EFI_SUCCESS;
}

/**
  Transcode one of the EFI return code used by the model into an EFI Shell return code.

  @param[in]  Status  EFI return code.

  @return  Transcoded EFI Shell return code.

**/
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
