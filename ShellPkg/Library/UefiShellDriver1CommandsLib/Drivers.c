/** @file
  Main file for Drivers shell Driver1 function.

  (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDriver1CommandsLib.h"

#define MAX_LEN_DRIVER_NAME  35

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-sfo", TypeFlag  },
  { L"-l",   TypeValue },
  { NULL,    TypeMax   }
};

/**
  Get a device path (in text format) for a given handle.

  @param[in] TheHandle      The handle to get the device path for.

  @retval NULL    An error occurred.
  @return         A pointer to the driver path as a string.  The callee must
                  free this memory.
**/
CHAR16 *
GetDevicePathTextForHandle (
  IN EFI_HANDLE  TheHandle
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL   *ImageDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *FinalPath;
  CHAR16                     *RetVal;

  FinalPath = NULL;

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    LoadedImage->DeviceHandle,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&ImageDevicePath,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      FinalPath = AppendDevicePath (ImageDevicePath, LoadedImage->FilePath);
      gBS->CloseProtocol (
             LoadedImage->DeviceHandle,
             &gEfiDevicePathProtocolGuid,
             gImageHandle,
             NULL
             );
    }

    gBS->CloseProtocol (
           TheHandle,
           &gEfiLoadedImageProtocolGuid,
           gImageHandle,
           NULL
           );
  }

  if (FinalPath == NULL) {
    return (NULL);
  }

  RetVal = gEfiShellProtocol->GetFilePathFromDevicePath (FinalPath);
  if (RetVal == NULL) {
    RetVal = ConvertDevicePathToText (FinalPath, TRUE, TRUE);
  }

  FreePool (FinalPath);
  return (RetVal);
}

/**
  Determine if the given handle has Driver Configuration protocol.

  @param[in] TheHandle      The handle to the driver to test.

  @retval TRUE              The driver does have Driver Configuration.
  @retval FALSE             The driver does not have Driver Configuration.
**/
BOOLEAN
ReturnDriverConfig (
  IN CONST EFI_HANDLE  TheHandle
  )
{
  EFI_STATUS  Status;

  Status = gBS->OpenProtocol ((EFI_HANDLE)TheHandle, &gEfiDriverConfigurationProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
  if (EFI_ERROR (Status)) {
    return (FALSE);
  }

  return (TRUE);
}

/**
  Determine if the given handle has DriverDiagnostics protocol.

  @param[in] TheHandle      The handle to the driver to test.

  @retval TRUE              The driver does have Driver Diasgnostics.
  @retval FALSE             The driver does not have Driver Diagnostics.
**/
BOOLEAN
ReturnDriverDiag (
  IN CONST EFI_HANDLE  TheHandle
  )
{
  EFI_STATUS  Status;

  Status = gBS->OpenProtocol ((EFI_HANDLE)TheHandle, &gEfiDriverDiagnostics2ProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol ((EFI_HANDLE)TheHandle, &gEfiDriverDiagnosticsProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
    if (EFI_ERROR (Status)) {
      return (FALSE);
    }
  }

  return (TRUE);
}

/**
  Finds and returns the version of the driver specified by TheHandle.

  @param[in] TheHandle      The driver handle to get the version of.

  @return             The version of the driver.
  @retval 0xFFFFFFFF  An error ocurred.
**/
UINT32
ReturnDriverVersion (
  IN CONST EFI_HANDLE  TheHandle
  )
{
  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding;
  EFI_STATUS                   Status;
  UINT32                       RetVal;

  RetVal = (UINT32)-1;

  Status = gBS->OpenProtocol ((EFI_HANDLE)TheHandle, &gEfiDriverBindingProtocolGuid, (VOID **)&DriverBinding, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (!EFI_ERROR (Status)) {
    RetVal = DriverBinding->Version;
    gBS->CloseProtocol (TheHandle, &gEfiDriverBindingProtocolGuid, gImageHandle, NULL);
  }

  return (RetVal);
}

/**
  Get image name from Image Handle.

  @param[in] Handle      Image Handle

  @return         A pointer to the image name as a string.
**/
CHAR16 *
GetImageNameFromHandle (
  IN CONST EFI_HANDLE  Handle
  )
{
  EFI_STATUS                     Status;
  EFI_DRIVER_BINDING_PROTOCOL    *DriverBinding;
  EFI_LOADED_IMAGE_PROTOCOL      *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL       *DevPathNode;
  EFI_GUID                       *NameGuid;
  CHAR16                         *ImageName;
  UINTN                          BufferSize;
  UINT32                         AuthenticationStatus;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *Fv2;

  LoadedImage   = NULL;
  DriverBinding = NULL;
  ImageName     = NULL;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiDriverBindingProtocolGuid,
                  (VOID **)&DriverBinding,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gBS->OpenProtocol (
                  DriverBinding->ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    DevPathNode = LoadedImage->FilePath;
    if (DevPathNode == NULL) {
      return NULL;
    }

    while (!IsDevicePathEnd (DevPathNode)) {
      NameGuid = EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)DevPathNode);
      if (NameGuid != NULL) {
        Status = gBS->HandleProtocol (
                        LoadedImage->DeviceHandle,
                        &gEfiFirmwareVolume2ProtocolGuid,
                        (VOID **)&Fv2
                        );
        if (!EFI_ERROR (Status)) {
          Status = Fv2->ReadSection (
                          Fv2,
                          NameGuid,
                          EFI_SECTION_USER_INTERFACE,
                          0,
                          (VOID **)&ImageName,
                          &BufferSize,
                          &AuthenticationStatus
                          );
          if (!EFI_ERROR (Status)) {
            break;
          }

          ImageName = NULL;
        }
      }

      //
      // Next device path node
      //
      DevPathNode = NextDevicePathNode (DevPathNode);
    }

    if (ImageName == NULL) {
      ImageName = ConvertDevicePathToText (LoadedImage->FilePath, TRUE, TRUE);
    }
  }

  return ImageName;
}

/**
  Function for 'drivers' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDrivers (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CHAR8         *Language;
  CONST CHAR16  *Lang;
  EFI_HANDLE    *HandleList;
  EFI_HANDLE    *HandleWalker;
  UINTN         ChildCount;
  UINTN         DeviceCount;
  CHAR16        ChildCountStr[21];
  CHAR16        DeviceCountStr[21];
  CHAR16        *Temp2;
  CONST CHAR16  *FullDriverName;
  CHAR16        *TruncatedDriverName;
  CHAR16        *ImageName;
  CHAR16        *FormatString;
  UINT32        DriverVersion;
  BOOLEAN       DriverConfig;
  BOOLEAN       DriverDiag;
  BOOLEAN       SfoFlag;

  ShellStatus  = SHELL_SUCCESS;
  Status       = EFI_SUCCESS;
  Language     = NULL;
  FormatString = NULL;
  SfoFlag      = FALSE;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"drivers", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 1) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"drivers");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (ShellCommandLineGetFlag (Package, L"-l")) {
        Lang = ShellCommandLineGetValue (Package, L"-l");
        if (Lang != NULL) {
          Language = AllocateZeroPool (StrSize (Lang));
          AsciiSPrint (Language, StrSize (Lang), "%S", Lang);
        } else {
          ASSERT (Language == NULL);
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"drivers", L"-l");
          ShellCommandLineFreeVarList (Package);
          return (SHELL_INVALID_PARAMETER);
        }
      }

      if (ShellCommandLineGetFlag (Package, L"-sfo")) {
        SfoFlag      = TRUE;
        FormatString = HiiGetString (gShellDriver1HiiHandle, STRING_TOKEN (STR_DRIVERS_ITEM_LINE_SFO), Language);
        //
        // print the SFO header
        //
        ShellPrintHiiEx (
          -1,
          -1,
          Language,
          STRING_TOKEN (STR_GEN_SFO_HEADER),
          gShellDriver1HiiHandle,
          L"drivers"
          );
      } else {
        FormatString = HiiGetString (gShellDriver1HiiHandle, STRING_TOKEN (STR_DRIVERS_ITEM_LINE), Language);
        //
        // print the header row
        //
        ShellPrintHiiEx (
          -1,
          -1,
          Language,
          STRING_TOKEN (STR_DRIVERS_HEADER_LINES),
          gShellDriver1HiiHandle
          );
      }

      HandleList = GetHandleListByProtocol (&gEfiDriverBindingProtocolGuid);
      for (HandleWalker = HandleList; HandleWalker != NULL && *HandleWalker != NULL; HandleWalker++) {
        ChildCount     = 0;
        DeviceCount    = 0;
        Status         = ParseHandleDatabaseForChildDevices (*HandleWalker, &ChildCount, NULL);
        Status         = PARSE_HANDLE_DATABASE_DEVICES (*HandleWalker, &DeviceCount, NULL);
        Temp2          = GetDevicePathTextForHandle (*HandleWalker);
        DriverVersion  = ReturnDriverVersion (*HandleWalker);
        DriverConfig   = ReturnDriverConfig (*HandleWalker);
        DriverDiag     = ReturnDriverDiag (*HandleWalker);
        FullDriverName = GetStringNameFromHandle (*HandleWalker, Language);
        ImageName      = GetImageNameFromHandle (*HandleWalker);

        UnicodeValueToStringS (ChildCountStr, sizeof (ChildCountStr), 0, ChildCount, 0);
        UnicodeValueToStringS (DeviceCountStr, sizeof (DeviceCountStr), 0, DeviceCount, 0);
        TruncatedDriverName = NULL;
        if (!SfoFlag && (FullDriverName != NULL)) {
          TruncatedDriverName = AllocateZeroPool ((MAX_LEN_DRIVER_NAME + 1) * sizeof (CHAR16));
          StrnCpyS (TruncatedDriverName, MAX_LEN_DRIVER_NAME + 1, FullDriverName, MAX_LEN_DRIVER_NAME);
        }

        if (!SfoFlag) {
          ShellPrintEx (
            -1,
            -1,
            FormatString,
            ConvertHandleToHandleIndex (*HandleWalker),
            DriverVersion,
            ChildCount > 0 ? L'B' : (DeviceCount > 0 ? L'D' : L'?'),
            DriverConfig ? L'X' : L'-',
            DriverDiag ? L'X' : L'-',
            DeviceCount > 0 ? DeviceCountStr : L"-",
            ChildCount  > 0 ? ChildCountStr : L"-",
            TruncatedDriverName,
            ImageName == NULL ? L"" : ImageName
            );
        } else {
          ShellPrintEx (
            -1,
            -1,
            FormatString,
            ConvertHandleToHandleIndex (*HandleWalker),
            DriverVersion,
            ChildCount > 0 ? L'B' : (DeviceCount > 0 ? L'D' : L'?'),
            DriverConfig ? L'Y' : L'N',
            DriverDiag ? L'Y' : L'N',
            DeviceCount,
            ChildCount,
            FullDriverName,
            Temp2 == NULL ? L"" : Temp2
            );
        }

        if (TruncatedDriverName != NULL) {
          FreePool (TruncatedDriverName);
        }

        if (Temp2 != NULL) {
          FreePool (Temp2);
        }

        if (ImageName != NULL) {
          FreePool (ImageName);
        }

        if (ShellGetExecutionBreakFlag ()) {
          ShellStatus = SHELL_ABORTED;
          break;
        }
      }
    }

    SHELL_FREE_NON_NULL (Language);
    ShellCommandLineFreeVarList (Package);
    SHELL_FREE_NON_NULL (FormatString);
  }

  return (ShellStatus);
}
