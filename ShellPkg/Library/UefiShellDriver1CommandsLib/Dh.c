/** @file
  Main file for Dh shell Driver1 function.

  (C) Copyright 2014-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2017 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-p",       TypeValue },
  { L"-d",       TypeFlag  },
  { L"-v",       TypeFlag  },
  { L"-verbose", TypeFlag  },
  { L"-sfo",     TypeFlag  },
  { L"-l",       TypeValue },
  { NULL,        TypeMax   }
};

STATIC CONST EFI_GUID  *UefiDriverModelProtocolsGuidArray[] = {
  &gEfiDriverBindingProtocolGuid,
  &gEfiPlatformDriverOverrideProtocolGuid,
  &gEfiBusSpecificDriverOverrideProtocolGuid,
  &gEfiDriverDiagnosticsProtocolGuid,
  &gEfiDriverDiagnostics2ProtocolGuid,
  &gEfiComponentNameProtocolGuid,
  &gEfiComponentName2ProtocolGuid,
  &gEfiPlatformToDriverConfigurationProtocolGuid,
  &gEfiDriverSupportedEfiVersionProtocolGuid,
  &gEfiDriverFamilyOverrideProtocolGuid,
  &gEfiDriverHealthProtocolGuid,
  &gEfiLoadedImageProtocolGuid,
  NULL
};

UINTN  mGuidDataLen[] = { 8, 4, 4, 4, 12 };

/**
  Function to determine if the string can convert to a GUID.
  The string must be restricted as "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" format.

  @param[in]  String  The string to test.

  @retval     TRUE    The string can convert to a GUID.
  @retval     FALSE   The string can't convert to a GUID.
**/
BOOLEAN
IsValidGuidString (
  IN CONST CHAR16  *String
  )
{
  CONST CHAR16  *Walker;
  CONST CHAR16  *PrevWalker;
  UINTN         Index;

  if (String == NULL) {
    return FALSE;
  }

  Walker     = String;
  PrevWalker = String;
  Index      = 0;

  while (Walker != NULL && *Walker != CHAR_NULL) {
    if (((*Walker >= '0') && (*Walker <= '9')) ||
        ((*Walker >= 'a') && (*Walker <= 'f')) ||
        ((*Walker >= 'A') && (*Walker <= 'F'))
        )
    {
      Walker++;
    } else {
      if ((*Walker == L'-') && ((((UINTN)Walker - (UINTN)PrevWalker) / sizeof (CHAR16)) == mGuidDataLen[Index])) {
        Walker++;
        PrevWalker = Walker;
        Index++;
      } else {
        return FALSE;
      }
    }
  }

  if ((((UINTN)Walker - (UINTN)PrevWalker) / sizeof (CHAR16)) == mGuidDataLen[Index]) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Convert a hex-character to decimal value.

  This internal function only deal with Unicode character
  which maps to a valid hexadecimal ASII character, i.e.
  L'0' to L'9', L'a' to L'f' or L'A' to L'F'. For other
  Unicode character, the value returned does not make sense.

  @param[in]  Char      The character to convert.

  @retval               The numerical value converted.
**/
UINTN
HexCharToDecimal (
  IN CHAR16  Char
  )
{
  if ((Char >= '0') && (Char <= '9')) {
    return Char - L'0';
  } else if ((Char >= 'a') && (Char <= 'f')) {
    return Char - L'a' + 10;
  } else {
    return Char - L'A' + 10;
  }
}

/**
  Function try to convert a string to GUID format.

  @param[in]    String    The string will be converted.
  @param[out]   Guid      Save the result convert from string.

  @retval EFI_SUCCESS     The string was successfully converted to a GUID.
  @retval EFI_UNSUPPORTED The input string is not in registry format.
**/
EFI_STATUS
ConvertStrToGuid (
  IN  CONST CHAR16  *String,
  OUT GUID          *Guid
  )
{
  CONST CHAR16  *Walker;
  UINT8         TempValue;
  UINTN         Index;

  if ((String == NULL) || !IsValidGuidString (String)) {
    return EFI_UNSUPPORTED;
  }

  Index = 0;

  Walker      = String;
  Guid->Data1 = (UINT32)StrHexToUint64 (Walker);

  Walker     += 9;
  Guid->Data2 = (UINT16)StrHexToUint64 (Walker);

  Walker     += 5;
  Guid->Data3 = (UINT16)StrHexToUint64 (Walker);

  Walker += 5;
  while (Walker != NULL && *Walker != CHAR_NULL) {
    if (*Walker == L'-') {
      Walker++;
    } else {
      TempValue = (UINT8)HexCharToDecimal (*Walker);
      TempValue = (UINT8)LShiftU64 (TempValue, 4);
      Walker++;

      TempValue += (UINT8)HexCharToDecimal (*Walker);
      Walker++;

      Guid->Data4[Index] = TempValue;
      Index++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Get the name of a driver by it's handle.

  If a name is found the memory must be callee freed.

  @param[in] TheHandle    The driver's handle.
  @param[in] Language     The language to use.
  @param[in] NameFound    Upon a successful return the name found.

  @retval EFI_SUCCESS     The name was found.
**/
EFI_STATUS
GetDriverName (
  IN EFI_HANDLE   TheHandle,
  IN CONST CHAR8  *Language,
  IN CHAR16       **NameFound
  )
{
  CHAR8                         *Lang;
  EFI_STATUS                    Status;
  EFI_COMPONENT_NAME2_PROTOCOL  *CompName2;
  CHAR16                        *NameToReturn;

  //
  // Go through those handles until we get one that passes for GetComponentName
  //
  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **)&CompName2,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    TheHandle,
                    &gEfiComponentNameProtocolGuid,
                    (VOID **)&CompName2,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
  }

  if (EFI_ERROR (Status)) {
    return (EFI_NOT_FOUND);
  }

  Lang   = GetBestLanguageForDriver (CompName2->SupportedLanguages, Language, FALSE);
  Status = CompName2->GetDriverName (CompName2, Lang, &NameToReturn);
  FreePool (Lang);

  if (!EFI_ERROR (Status) && (NameToReturn != NULL)) {
    *NameFound = NULL;
    StrnCatGrow (NameFound, NULL, NameToReturn, 0);
  }

  return (Status);
}

/**
  Discover if a protocol guid is one of the UEFI Driver Model Protocols.

  @param[in] Guid   The guid to test.

  @retval TRUE      The guid does represent a driver model protocol.
  @retval FALSE     The guid does not represent a driver model protocol.
**/
BOOLEAN
IsDriverProt (
  IN CONST EFI_GUID  *Guid
  )
{
  CONST EFI_GUID  **GuidWalker;
  BOOLEAN         GuidFound;

  GuidFound = FALSE;
  for (GuidWalker = UefiDriverModelProtocolsGuidArray
       ; GuidWalker != NULL && *GuidWalker != NULL
       ; GuidWalker++
       )
  {
    if (CompareGuid (*GuidWalker, Guid)) {
      GuidFound = TRUE;
      break;
    }
  }

  return (GuidFound);
}

/**
  Get information for a handle.

  @param[in] TheHandle        The handles to show info on.
  @param[in] Language         Language string per UEFI specification.
  @param[in] Separator        Separator string between information blocks.
  @param[in] Verbose          TRUE for extra info, FALSE otherwise.
  @param[in] ExtraInfo        TRUE for extra info, FALSE otherwise.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_INVALID_PARAMETER ProtocolName was NULL or invalid.
**/
CHAR16 *
GetProtocolInfoString (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST CHAR8       *Language,
  IN CONST CHAR16      *Separator,
  IN CONST BOOLEAN     Verbose,
  IN CONST BOOLEAN     ExtraInfo
  )
{
  EFI_GUID    **ProtocolGuidArray;
  UINTN       ArrayCount;
  UINTN       ProtocolIndex;
  EFI_STATUS  Status;
  CHAR16      *RetVal;
  UINTN       Size;
  CHAR16      *Temp;
  CHAR16      GuidStr[40];
  VOID        *Instance;
  CHAR16      InstanceStr[17];

  ProtocolGuidArray = NULL;
  RetVal            = NULL;
  Size              = 0;

  Status = gBS->ProtocolsPerHandle (
                  TheHandle,
                  &ProtocolGuidArray,
                  &ArrayCount
                  );
  if (!EFI_ERROR (Status)) {
    for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {
      Temp = GetStringNameFromGuid (ProtocolGuidArray[ProtocolIndex], Language);
      ASSERT ((RetVal == NULL && Size == 0) || (RetVal != NULL));
      if (Size != 0) {
        StrnCatGrow (&RetVal, &Size, Separator, 0);
      }

      StrnCatGrow (&RetVal, &Size, L"%H", 0);
      if (Temp == NULL) {
        UnicodeSPrint (GuidStr, sizeof (GuidStr), L"%g", ProtocolGuidArray[ProtocolIndex]);
        StrnCatGrow (&RetVal, &Size, GuidStr, 0);
      } else {
        StrnCatGrow (&RetVal, &Size, Temp, 0);
        FreePool (Temp);
      }

      StrnCatGrow (&RetVal, &Size, L"%N", 0);

      if (Verbose) {
        Status = gBS->HandleProtocol (TheHandle, ProtocolGuidArray[ProtocolIndex], &Instance);
        if (!EFI_ERROR (Status)) {
          StrnCatGrow (&RetVal, &Size, L"(%H", 0);
          UnicodeSPrint (InstanceStr, sizeof (InstanceStr), L"%p", Instance);
          StrnCatGrow (&RetVal, &Size, InstanceStr, 0);
          StrnCatGrow (&RetVal, &Size, L"%N)", 0);
        }
      }

      if (ExtraInfo) {
        Temp = GetProtocolInformationDump (TheHandle, ProtocolGuidArray[ProtocolIndex], Verbose);
        if (Temp != NULL) {
          ASSERT ((RetVal == NULL && Size == 0) || (RetVal != NULL));
          if (!Verbose) {
            StrnCatGrow (&RetVal, &Size, L"(", 0);
            StrnCatGrow (&RetVal, &Size, Temp, 0);
            StrnCatGrow (&RetVal, &Size, L")", 0);
          } else {
            StrnCatGrow (&RetVal, &Size, Separator, 0);
            StrnCatGrow (&RetVal, &Size, Temp, 0);
          }

          FreePool (Temp);
        }
      }
    }
  }

  SHELL_FREE_NON_NULL (ProtocolGuidArray);

  if (RetVal == NULL) {
    return (NULL);
  }

  ASSERT ((RetVal == NULL && Size == 0) || (RetVal != NULL));
  StrnCatGrow (&RetVal, &Size, Separator, 0);
  return (RetVal);
}

/**
  Gets the name of the loaded image.

  @param[in] TheHandle    The handle of the driver to get info on.
  @param[out] Name        The pointer to the pointer.  Valid upon a successful return.

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
GetDriverImageName (
  IN EFI_HANDLE  TheHandle,
  OUT CHAR16     **Name
  )
{
  // get loaded image and devicepathtotext on image->Filepath
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;

  if ((TheHandle == NULL) || (Name == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  DevicePath = LoadedImage->FilePath;
  *Name      = ConvertDevicePathToText (DevicePath, TRUE, TRUE);
  return (EFI_SUCCESS);
}

/**
  Display driver model information for a given handle.

  @param[in] Handle     The handle to display info on.
  @param[in] BestName   Use the best name?
  @param[in] Language   The language to output in.
**/
EFI_STATUS
DisplayDriverModelHandle (
  IN EFI_HANDLE   Handle,
  IN BOOLEAN      BestName,
  IN CONST CHAR8  *Language OPTIONAL
  )
{
  EFI_STATUS                   Status;
  BOOLEAN                      ConfigurationStatus;
  BOOLEAN                      DiagnosticsStatus;
  UINTN                        DriverBindingHandleCount;
  EFI_HANDLE                   *DriverBindingHandleBuffer;
  UINTN                        ParentControllerHandleCount;
  EFI_HANDLE                   *ParentControllerHandleBuffer;
  UINTN                        ChildControllerHandleCount;
  EFI_HANDLE                   *ChildControllerHandleBuffer;
  CHAR16                       *TempStringPointer;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;
  UINTN                        Index;
  CHAR16                       *DriverName;
  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding;
  UINTN                        NumberOfChildren;
  UINTN                        HandleIndex;
  UINTN                        ControllerHandleCount;
  EFI_HANDLE                   *ControllerHandleBuffer;
  UINTN                        ChildIndex;
  BOOLEAN                      Image;

  DriverName = NULL;

  //
  // See if Handle is a device handle and display its details.
  //
  DriverBindingHandleBuffer = NULL;
  Status                    = PARSE_HANDLE_DATABASE_UEFI_DRIVERS (
                                Handle,
                                &DriverBindingHandleCount,
                                &DriverBindingHandleBuffer
                                );

  ParentControllerHandleBuffer = NULL;
  Status                       = PARSE_HANDLE_DATABASE_PARENTS (
                                   Handle,
                                   &ParentControllerHandleCount,
                                   &ParentControllerHandleBuffer
                                   );

  ChildControllerHandleBuffer = NULL;
  Status                      = ParseHandleDatabaseForChildControllers (
                                  Handle,
                                  &ChildControllerHandleCount,
                                  &ChildControllerHandleBuffer
                                  );

  DiagnosticsStatus   = FALSE;
  ConfigurationStatus = FALSE;

  if (!EFI_ERROR (gBS->OpenProtocol (Handle, &gEfiDriverConfigurationProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
    ConfigurationStatus = TRUE;
  }

  if (!EFI_ERROR (gBS->OpenProtocol (Handle, &gEfiDriverConfiguration2ProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
    ConfigurationStatus = TRUE;
  }

  if (!EFI_ERROR (gBS->OpenProtocol (Handle, &gEfiDriverDiagnosticsProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
    DiagnosticsStatus = TRUE;
  }

  if (!EFI_ERROR (gBS->OpenProtocol (Handle, &gEfiDriverDiagnostics2ProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
    DiagnosticsStatus = TRUE;
  }

  Status = EFI_SUCCESS;

  if ((DriverBindingHandleCount > 0) || (ParentControllerHandleCount > 0) || (ChildControllerHandleCount > 0)) {
    DevicePath        = NULL;
    TempStringPointer = NULL;
    Status            = gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);

    Status = gEfiShellProtocol->GetDeviceName (Handle, EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8 *)Language, &TempStringPointer);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_OUTPUT_DRIVER1), gShellDriver1HiiHandle, TempStringPointer != NULL ? TempStringPointer : L"<Unknown>");
    SHELL_FREE_NON_NULL (TempStringPointer);

    TempStringPointer = ConvertDevicePathToText (DevicePath, TRUE, FALSE);
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_DH_OUTPUT_DRIVER2),
      gShellDriver1HiiHandle,
      TempStringPointer != NULL ? TempStringPointer : L"<None>",
      ParentControllerHandleCount == 0 ? L"ROOT" : (ChildControllerHandleCount > 0) ? L"BUS" : L"DEVICE",
      ConfigurationStatus ? L"YES" : L"NO",
      DiagnosticsStatus ? L"YES" : L"NO"
      );

    SHELL_FREE_NON_NULL (TempStringPointer);

    if (DriverBindingHandleCount == 0) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER3),
        gShellDriver1HiiHandle,
        L"<None>"
        );
    } else {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER3),
        gShellDriver1HiiHandle,
        L""
        );
      for (Index = 0; Index < DriverBindingHandleCount; Index++) {
        Image  = FALSE;
        Status = GetDriverName (
                   DriverBindingHandleBuffer[Index],
                   Language,
                   &DriverName
                   );
        if (EFI_ERROR (Status)) {
          Status = GetDriverImageName (
                     DriverBindingHandleBuffer[Index],
                     &DriverName
                     );
          if (EFI_ERROR (Status)) {
            DriverName = NULL;
          }
        }

        if (Image) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_DH_OUTPUT_DRIVER4A),
            gShellDriver1HiiHandle,
            ConvertHandleToHandleIndex (DriverBindingHandleBuffer[Index]),
            DriverName != NULL ? DriverName : L"<Unknown>"
            );
        } else {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_DH_OUTPUT_DRIVER4B),
            gShellDriver1HiiHandle,
            ConvertHandleToHandleIndex (DriverBindingHandleBuffer[Index]),
            DriverName != NULL ? DriverName : L"<Unknown>"
            );
        }

        SHELL_FREE_NON_NULL (DriverName);
      }
    }

    if (ParentControllerHandleCount == 0) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER5),
        gShellDriver1HiiHandle,
        L"<None>"
        );
    } else {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER5),
        gShellDriver1HiiHandle,
        L""
        );
      for (Index = 0; Index < ParentControllerHandleCount; Index++) {
        Status = gEfiShellProtocol->GetDeviceName (ParentControllerHandleBuffer[Index], EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8 *)Language, &TempStringPointer);
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_DH_OUTPUT_DRIVER5B),
          gShellDriver1HiiHandle,
          ConvertHandleToHandleIndex (ParentControllerHandleBuffer[Index]),
          TempStringPointer != NULL ? TempStringPointer : L"<Unknown>"
          );
        SHELL_FREE_NON_NULL (TempStringPointer);
      }
    }

    if (ChildControllerHandleCount == 0) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER6),
        gShellDriver1HiiHandle,
        L"<None>"
        );
    } else {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER6),
        gShellDriver1HiiHandle,
        L""
        );
      for (Index = 0; Index < ChildControllerHandleCount; Index++) {
        Status = gEfiShellProtocol->GetDeviceName (ChildControllerHandleBuffer[Index], EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8 *)Language, &TempStringPointer);
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_DH_OUTPUT_DRIVER6B),
          gShellDriver1HiiHandle,
          ConvertHandleToHandleIndex (ChildControllerHandleBuffer[Index]),
          TempStringPointer != NULL ? TempStringPointer : L"<Unknown>"
          );
        SHELL_FREE_NON_NULL (TempStringPointer);
      }
    }
  }

  SHELL_FREE_NON_NULL (DriverBindingHandleBuffer);

  SHELL_FREE_NON_NULL (ParentControllerHandleBuffer);

  SHELL_FREE_NON_NULL (ChildControllerHandleBuffer);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // See if Handle is a driver binding handle and display its details.
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiDriverBindingProtocolGuid,
                  (VOID **)&DriverBinding,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  NumberOfChildren       = 0;
  ControllerHandleBuffer = NULL;
  Status                 = PARSE_HANDLE_DATABASE_DEVICES (
                             Handle,
                             &ControllerHandleCount,
                             &ControllerHandleBuffer
                             );
  if (ControllerHandleCount > 0) {
    for (HandleIndex = 0; HandleIndex < ControllerHandleCount; HandleIndex++) {
      Status = PARSE_HANDLE_DATABASE_MANAGED_CHILDREN (
                 Handle,
                 ControllerHandleBuffer[HandleIndex],
                 &ChildControllerHandleCount,
                 NULL
                 );
      NumberOfChildren += ChildControllerHandleCount;
    }
  }

  Status = GetDriverName (Handle, Language, &DriverName);
  if (EFI_ERROR (Status)) {
    DriverName = NULL;
  }

  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_DH_OUTPUT_DRIVER7),
    gShellDriver1HiiHandle,
    ConvertHandleToHandleIndex (Handle),
    DriverName != NULL ? DriverName : L"<Unknown>"
    );
  SHELL_FREE_NON_NULL (DriverName);
  Status = GetDriverImageName (
             Handle,
             &DriverName
             );
  if (EFI_ERROR (Status)) {
    DriverName = NULL;
  }

  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_DH_OUTPUT_DRIVER7B),
    gShellDriver1HiiHandle,
    DriverName != NULL ? DriverName : L"<Unknown>"
    );
  SHELL_FREE_NON_NULL (DriverName);

  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_DH_OUTPUT_DRIVER8),
    gShellDriver1HiiHandle,
    DriverBinding->Version,
    NumberOfChildren > 0 ? L"Bus" : ControllerHandleCount > 0 ? L"Device" : L"<Unknown>",
    ConfigurationStatus ? L"YES" : L"NO",
    DiagnosticsStatus ? L"YES" : L"NO"
    );

  if (ControllerHandleCount == 0) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_DH_OUTPUT_DRIVER9),
      gShellDriver1HiiHandle,
      L"None"
      );
  } else {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_DH_OUTPUT_DRIVER9),
      gShellDriver1HiiHandle,
      L""
      );
    for (HandleIndex = 0; HandleIndex < ControllerHandleCount; HandleIndex++) {
      Status = gEfiShellProtocol->GetDeviceName (ControllerHandleBuffer[HandleIndex], EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8 *)Language, &TempStringPointer);

      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER9B),
        gShellDriver1HiiHandle,
        ConvertHandleToHandleIndex (ControllerHandleBuffer[HandleIndex]),
        TempStringPointer != NULL ? TempStringPointer : L"<Unknown>"
        );
      SHELL_FREE_NON_NULL (TempStringPointer);

      Status = PARSE_HANDLE_DATABASE_MANAGED_CHILDREN (
                 Handle,
                 ControllerHandleBuffer[HandleIndex],
                 &ChildControllerHandleCount,
                 &ChildControllerHandleBuffer
                 );
      if (!EFI_ERROR (Status)) {
        for (ChildIndex = 0; ChildIndex < ChildControllerHandleCount; ChildIndex++) {
          Status = gEfiShellProtocol->GetDeviceName (ChildControllerHandleBuffer[ChildIndex], EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8 *)Language, &TempStringPointer);

          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_DH_OUTPUT_DRIVER6C),
            gShellDriver1HiiHandle,
            ConvertHandleToHandleIndex (ChildControllerHandleBuffer[ChildIndex]),
            TempStringPointer != NULL ? TempStringPointer : L"<Unknown>"
            );
          SHELL_FREE_NON_NULL (TempStringPointer);
        }

        SHELL_FREE_NON_NULL (ChildControllerHandleBuffer);
      }
    }

    SHELL_FREE_NON_NULL (ControllerHandleBuffer);
  }

  return EFI_SUCCESS;
}

/**
  Display information for a handle.

  @param[in] TheHandle        The handles to show info on.
  @param[in] Verbose          TRUE for extra info, FALSE otherwise.
  @param[in] Sfo              TRUE to output in standard format output (spec).
  @param[in] Language         Language string per UEFI specification.
  @param[in] DriverInfo       TRUE to show all info about the handle.
  @param[in] Multiple         TRUE indicates more than  will be output,
                              FALSE for a single one.
**/
VOID
DoDhByHandle (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose,
  IN CONST BOOLEAN     Sfo,
  IN CONST CHAR8       *Language,
  IN CONST BOOLEAN     DriverInfo,
  IN CONST BOOLEAN     Multiple
  )
{
  CHAR16  *ProtocolInfoString;

  ProtocolInfoString = NULL;

  if (!Sfo) {
    if (Multiple) {
      ProtocolInfoString = GetProtocolInfoString (TheHandle, Language, L" ", Verbose, TRUE);
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT),
        gShellDriver1HiiHandle,
        ConvertHandleToHandleIndex (TheHandle),
        ProtocolInfoString == NULL ? L"" : ProtocolInfoString
        );
    } else {
      ProtocolInfoString = GetProtocolInfoString (TheHandle, Language, Verbose ? L"\r\n" : L" ", Verbose, TRUE);
      if (Verbose) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_DH_OUTPUT_SINGLE),
          gShellDriver1HiiHandle,
          ConvertHandleToHandleIndex (TheHandle),
          TheHandle,
          ProtocolInfoString == NULL ? L"" : ProtocolInfoString
          );
      } else {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_DH_OUTPUT_SINGLE_D),
          gShellDriver1HiiHandle,
          ConvertHandleToHandleIndex (TheHandle),
          ProtocolInfoString == NULL ? L"" : ProtocolInfoString
          );
      }
    }

    if (DriverInfo) {
      DisplayDriverModelHandle ((EFI_HANDLE)TheHandle, TRUE, Language);
    }
  } else {
    ProtocolInfoString = GetProtocolInfoString (TheHandle, Language, L";", FALSE, FALSE);
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_DH_OUTPUT_SFO),
      gShellDriver1HiiHandle,
      Multiple ? L"HandlesInfo" : L"HandleInfo",
      L"DriverName",
      L"ControllerName",
      ConvertHandleToHandleIndex (TheHandle),
      L"DevPath",
      ProtocolInfoString == NULL ? L"" : ProtocolInfoString
      );
  }

  if (ProtocolInfoString != NULL) {
    FreePool (ProtocolInfoString);
  }
}

/**
  Display information for all handles on a list.

  @param[in] HandleList       The NULL-terminated list of handles.
  @param[in] Verbose          TRUE for extra info, FALSE otherwise.
  @param[in] Sfo              TRUE to output in standard format output (spec).
  @param[in] Language         Language string per UEFI specification.
  @param[in] DriverInfo       TRUE to show all info about the handle.

  @retval SHELL_SUCCESS       The operation was successful.
  @retval SHELL_ABORTED       The operation was aborted.
**/
SHELL_STATUS
DoDhForHandleList (
  IN CONST EFI_HANDLE  *HandleList,
  IN CONST BOOLEAN     Verbose,
  IN CONST BOOLEAN     Sfo,
  IN CONST CHAR8       *Language,
  IN CONST BOOLEAN     DriverInfo
  )
{
  CONST EFI_HANDLE  *HandleWalker;
  SHELL_STATUS      ShellStatus;

  ShellStatus = SHELL_SUCCESS;
  for (HandleWalker = HandleList; HandleWalker != NULL && *HandleWalker != NULL; HandleWalker++) {
    DoDhByHandle (*HandleWalker, Verbose, Sfo, Language, DriverInfo, TRUE);
    if (ShellGetExecutionBreakFlag ()) {
      ShellStatus = SHELL_ABORTED;
      break;
    }
  }

  return (ShellStatus);
}

/**
  Display information for a GUID of protocol.

  @param[in] Guid             The pointer to the name of the protocol.
  @param[in] Verbose          TRUE for extra info, FALSE otherwise.
  @param[in] Sfo              TRUE to output in standard format output (spec).
  @param[in] Language         Language string per UEFI specification.
  @param[in] DriverInfo       TRUE to show all info about the handle.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_NOT_FOUND         The GUID was not found.
  @retval SHELL_INVALID_PARAMETER ProtocolName was NULL or invalid.
**/
SHELL_STATUS
DoDhByProtocolGuid (
  IN CONST GUID     *Guid,
  IN CONST BOOLEAN  Verbose,
  IN CONST BOOLEAN  Sfo,
  IN CONST CHAR8    *Language,
  IN CONST BOOLEAN  DriverInfo
  )
{
  CHAR16        *Name;
  SHELL_STATUS  ShellStatus;
  EFI_HANDLE    *HandleList;

  if (!Sfo) {
    if (Guid == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_OUTPUT_ALL_HEADER), gShellDriver1HiiHandle);
    } else {
      Name = GetStringNameFromGuid (Guid, NULL);
      if (Name == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_OUTPUT_GUID_HEADER), gShellDriver1HiiHandle, Guid);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_OUTPUT_NAME_HEADER), gShellDriver1HiiHandle, Name);
      }
    }
  }

  HandleList  = GetHandleListByProtocol (Guid);
  ShellStatus = DoDhForHandleList (HandleList, Verbose, Sfo, Language, DriverInfo);
  SHELL_FREE_NON_NULL (HandleList);

  return ShellStatus;
}

/**
  Function to determine use which method to print information.
  If Protocol is NULL, The function will print all information.

  @param[in] Protocol         The pointer to the name or GUID of protocol or NULL.
  @param[in] Verbose          TRUE for extra info, FALSE otherwise.
  @param[in] Sfo              TRUE to output in standard format output (spec).
  @param[in] Language         Language string per UEFI specification.
  @param[in] DriverInfo       TRUE to show all info about the handle.

  @retval SHELL_SUCCESS             The operation was successful.
  @retval SHELL_NOT_FOUND           The protocol was not found.
  @retval SHELL_INVALID_PARAMETER   Protocol is invalid parameter.
**/
SHELL_STATUS
DoDhByProtocol (
  IN CONST CHAR16   *Protocol,
  IN CONST BOOLEAN  Verbose,
  IN CONST BOOLEAN  Sfo,
  IN CONST CHAR8    *Language,
  IN CONST BOOLEAN  DriverInfo
  )
{
  EFI_GUID    Guid;
  EFI_GUID    *GuidPtr;
  EFI_STATUS  Status;

  if (Protocol == NULL) {
    return DoDhByProtocolGuid (NULL, Verbose, Sfo, Language, DriverInfo);
  } else {
    Status = ConvertStrToGuid (Protocol, &Guid);
    if (!EFI_ERROR (Status)) {
      GuidPtr = &Guid;
    } else {
      //
      // Protocol is a Name, convert it to GUID
      //
      Status = GetGuidFromStringName (Protocol, Language, &GuidPtr);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_NO_NAME_FOUND), gShellDriver1HiiHandle, Protocol);
        return (SHELL_NOT_FOUND);
      }
    }

    return DoDhByProtocolGuid (GuidPtr, Verbose, Sfo, Language, DriverInfo);
  }
}

/**
  Function to display decode information by Protocol.
  The parameter Protocol is either a GUID or the name of protocol.
  If the parameter Protocol is NULL, the function will print all
  decode information.

  @param[in] Protocol         The pointer to the name or GUID of protocol.
  @param[in] Language         Language string per UEFI specification.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_OUT_OT_RESOURCES  A memory allocation failed.
**/
SHELL_STATUS
DoDecodeByProtocol (
  IN CONST CHAR16  *Protocol,
  IN CONST CHAR8   *Language
  )
{
  EFI_STATUS  Status;
  EFI_GUID    *Guids;
  EFI_GUID    Guid;
  UINTN       Counts;
  UINTN       Index;
  CHAR16      *Name;

  if (Protocol == NULL) {
    Counts = 0;
    Status = GetAllMappingGuids (NULL, &Counts);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Guids = AllocatePool (Counts * sizeof (EFI_GUID));
      if (Guids == NULL) {
        return SHELL_OUT_OF_RESOURCES;
      }

      Status = GetAllMappingGuids (Guids, &Counts);
      if (Status == EFI_SUCCESS) {
        for (Index = 0; Index < Counts; Index++) {
          Name = GetStringNameFromGuid (&Guids[Index], Language);
          if (Name != NULL) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_OUTPUT_DECODE), gShellDriver1HiiHandle, Name, &Guids[Index]);
          } else {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_NO_GUID_FOUND), gShellDriver1HiiHandle, &Guids[Index]);
          }

          SHELL_FREE_NON_NULL (Name);
        }
      }

      FreePool (Guids);
    }
  } else {
    if (ConvertStrToGuid (Protocol, &Guid) == EFI_SUCCESS) {
      Name = GetStringNameFromGuid (&Guid, Language);
      if (Name != NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_OUTPUT_DECODE), gShellDriver1HiiHandle, Name, &Guid);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_NO_GUID_FOUND), gShellDriver1HiiHandle, &Guid);
      }

      SHELL_FREE_NON_NULL (Name);
    } else {
      Status = GetGuidFromStringName (Protocol, Language, &Guids);
      if (Status == EFI_SUCCESS) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_OUTPUT_DECODE), gShellDriver1HiiHandle, Protocol, Guids);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DH_NO_NAME_FOUND), gShellDriver1HiiHandle, Protocol);
      }
    }
  }

  return SHELL_SUCCESS;
}

/**
  Function for 'dh' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDh (
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
  CONST CHAR16  *RawValue;
  CONST CHAR16  *ProtocolVal;
  BOOLEAN       SfoFlag;
  BOOLEAN       DriverFlag;
  BOOLEAN       VerboseFlag;
  UINT64        Intermediate;
  EFI_HANDLE    Handle;

  ShellStatus = SHELL_SUCCESS;
  Status      = EFI_SUCCESS;
  Language    = NULL;

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
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"dh", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 2) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"dh");
      ShellCommandLineFreeVarList (Package);
      return (SHELL_INVALID_PARAMETER);
    }

    if (ShellCommandLineGetFlag (Package, L"-l")) {
      Lang = ShellCommandLineGetValue (Package, L"-l");
      if (Lang != NULL) {
        Language = AllocateZeroPool (StrSize (Lang));
        AsciiSPrint (Language, StrSize (Lang), "%S", Lang);
      } else {
        ASSERT (Language == NULL);
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"dh", L"-l");
        ShellCommandLineFreeVarList (Package);
        return (SHELL_INVALID_PARAMETER);
      }
    } else {
      Language = AllocateZeroPool (10);
      AsciiSPrint (Language, 10, "en-us");
    }

    SfoFlag     = ShellCommandLineGetFlag (Package, L"-sfo");
    DriverFlag  = ShellCommandLineGetFlag (Package, L"-d");
    VerboseFlag = (BOOLEAN)(ShellCommandLineGetFlag (Package, L"-v") || ShellCommandLineGetFlag (Package, L"-verbose"));
    RawValue    = ShellCommandLineGetRawValue (Package, 1);
    ProtocolVal = ShellCommandLineGetValue (Package, L"-p");

    if (RawValue == NULL) {
      if (ShellCommandLineGetFlag (Package, L"-p") && (ProtocolVal == NULL)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"dh", L"-p");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // Print information by protocol, The ProtocolVal maybe is name or GUID or NULL.
        //
        ShellStatus = DoDhByProtocol (ProtocolVal, VerboseFlag, SfoFlag, Language, DriverFlag);
      }
    } else if ((RawValue != NULL) &&
               (gUnicodeCollation->StriColl (gUnicodeCollation, L"decode", (CHAR16 *)RawValue) == 0))
    {
      if (ShellCommandLineGetFlag (Package, L"-p") && (ProtocolVal == NULL)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"dh", L"-p");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // Print decode informatino by protocol.
        //
        ShellStatus = DoDecodeByProtocol (ProtocolVal, Language);
      }
    } else {
      if (ShellCommandLineGetFlag (Package, L"-p")) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"dh");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Status = ShellConvertStringToUint64 (RawValue, &Intermediate, TRUE, FALSE);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"dh", RawValue);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          Handle = ConvertHandleIndexToHandle ((UINTN)Intermediate);
          if (Handle == NULL) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"dh", RawValue);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            //
            // Print information by handle.
            //
            DoDhByHandle (Handle, VerboseFlag, SfoFlag, Language, DriverFlag, FALSE);
          }
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
    SHELL_FREE_NON_NULL (Language);
  }

  return (ShellStatus);
}
