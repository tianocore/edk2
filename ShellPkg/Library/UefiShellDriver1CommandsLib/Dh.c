/** @file
  Main file for Dh shell Driver1 function.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-p", TypeValue},
  {L"-d", TypeFlag},
  {L"-v", TypeFlag},
  {L"-verbose", TypeFlag},
  {L"-sfo", TypeFlag},
  {L"-l", TypeValue},
  {NULL, TypeMax}
  };

STATIC CONST EFI_GUID *UefiDriverModelProtocolsGuidArray[] = {
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

/**
  Get the name of a driver by it's handle.

  If a name is found the memory must be callee freed.

  @param[in] TheHandle    The driver's handle.
  @param[in] Language     The language to use.
  @param[in] NameFound    Upon a successful return the name found.

  @retval EFI_SUCCESS     The name was found.
**/
EFI_STATUS
EFIAPI
GetDriverName (
  IN EFI_HANDLE   TheHandle,
  IN CONST CHAR8  *Language,
  IN CHAR16       **NameFound
  )
{
  CHAR8                             *Lang;
  CHAR8                             *TempChar;
  EFI_STATUS                        Status;
  EFI_COMPONENT_NAME2_PROTOCOL      *CompName2;
  CHAR16                            *NameToReturn;
  //
  // Go through those handles until we get one that passes for GetComponentName
  //
  Status = gBS->OpenProtocol(
    TheHandle,
    &gEfiComponentName2ProtocolGuid,
    (VOID**)&CompName2,
    gImageHandle,
    NULL,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR(Status)) {
    Status = gBS->OpenProtocol(
      TheHandle,
      &gEfiComponentNameProtocolGuid,
      (VOID**)&CompName2,
      gImageHandle,
      NULL,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  }

  if (EFI_ERROR(Status)) {
    return (EFI_NOT_FOUND);
  }
  if (Language == NULL) {
    Lang = AllocateZeroPool(AsciiStrSize(CompName2->SupportedLanguages));
    if (Lang == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    AsciiStrCpy(Lang, CompName2->SupportedLanguages);
    TempChar = AsciiStrStr(Lang, ";");
    if (TempChar != NULL){
      *TempChar = CHAR_NULL;
    }
  } else {
    Lang = AllocateZeroPool(AsciiStrSize(Language));
    if (Lang == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    AsciiStrCpy(Lang, Language);
  }
  Status = CompName2->GetDriverName(CompName2, Lang, &NameToReturn);
  FreePool(Lang);

  if (!EFI_ERROR(Status) && NameToReturn != NULL) {
    *NameFound = NULL;
    StrnCatGrow(NameFound, NULL, NameToReturn, 0);
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
EFIAPI
IsDriverProt (
  IN CONST EFI_GUID *Guid
  )
{
  CONST EFI_GUID            **GuidWalker;
  BOOLEAN                   GuidFound;
  GuidFound = FALSE;
  for (GuidWalker = UefiDriverModelProtocolsGuidArray
    ;  GuidWalker != NULL && *GuidWalker != NULL
    ;  GuidWalker++
   ){
    if (CompareGuid(*GuidWalker, Guid)) {
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
  @param[in] Seperator        Separator string between information blocks.
  @param[in] Verbose          TRUE for extra info, FALSE otherwise.
  @param[in] ExtraInfo        TRUE for extra info, FALSE otherwise.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_INVALID_PARAMETER ProtocolName was NULL or invalid.
**/
CHAR16*
EFIAPI
GetProtocolInfoString(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST CHAR8      *Language,
  IN CONST CHAR16     *Seperator,
  IN CONST BOOLEAN    Verbose,
  IN CONST BOOLEAN    ExtraInfo
  )
{
  EFI_GUID                  **ProtocolGuidArray;
  UINTN                     ArrayCount;
  UINTN                     ProtocolIndex;
  EFI_STATUS                Status;
  CHAR16                    *RetVal;
  UINTN                     Size;
  CHAR16                    *Temp;

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
      Temp = GetStringNameFromGuid(ProtocolGuidArray[ProtocolIndex], Language);
      if (Temp != NULL) {
        ASSERT((RetVal == NULL && Size == 0) || (RetVal != NULL));
        if (Size != 0) {
          StrnCatGrow(&RetVal, &Size, Seperator, 0);
        }
        StrnCatGrow(&RetVal, &Size, L"%H", 0);
        StrnCatGrow(&RetVal, &Size, Temp, 0);
        StrnCatGrow(&RetVal, &Size, L"%N", 0);
        FreePool(Temp);
      }
      if (ExtraInfo) {
        Temp = GetProtocolInformationDump(TheHandle, ProtocolGuidArray[ProtocolIndex], Verbose);
        if (Temp != NULL) {
          ASSERT((RetVal == NULL && Size == 0) || (RetVal != NULL));
          if (!Verbose) {
            StrnCatGrow(&RetVal, &Size, L"(", 0);
            StrnCatGrow(&RetVal, &Size, Temp, 0);
            StrnCatGrow(&RetVal, &Size, L")\r\n", 0);
          } else {
            StrnCatGrow(&RetVal, &Size, Seperator, 0);
            StrnCatGrow(&RetVal, &Size, Temp, 0);
          }
          FreePool(Temp);
        }
      }
    }
  }
  
  SHELL_FREE_NON_NULL(ProtocolGuidArray);

  if (RetVal == NULL) {
    return (NULL);
  }

  ASSERT((RetVal == NULL && Size == 0) || (RetVal != NULL));
  StrnCatGrow(&RetVal, &Size, Seperator, 0);
  return (RetVal);
}

/**
  Gets the name of the loaded image.

  @param[in] TheHandle    The handle of the driver to get info on.
  @param[out] Name        The pointer to the pointer.  Valid upon a successful return.

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
EFIAPI
GetDriverImageName (
  IN EFI_HANDLE   TheHandle,
  OUT CHAR16      **Name
  )
{
  // get loaded image and devicepathtotext on image->Filepath
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  if (TheHandle == NULL || Name == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = gBS->OpenProtocol (
                TheHandle,
                &gEfiLoadedImageProtocolGuid,
                (VOID **) &LoadedImage,
                gImageHandle,
                NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL
               );
  if (EFI_ERROR(Status)) {
    return (Status);
  }
  DevicePath = LoadedImage->FilePath;
  *Name = gDevPathToText->ConvertDevicePathToText(DevicePath, TRUE, TRUE);
  return (EFI_SUCCESS);
}

/**
  Display driver model information for a given handle.
  
  @param[in] Handle     The handle to display info on.
  @param[in] BestName   Use the best name?
  @param[in] Language   The language to output in.
**/
EFI_STATUS
EFIAPI
DisplayDriverModelHandle (
  IN EFI_HANDLE  Handle,
  IN BOOLEAN     BestName,
  IN CONST CHAR8 *Language OPTIONAL
  )
{
  EFI_STATUS                  Status;
  BOOLEAN                     ConfigurationStatus;
  BOOLEAN                     DiagnosticsStatus;
  UINTN                       DriverBindingHandleCount;
  EFI_HANDLE                  *DriverBindingHandleBuffer;
  UINTN                       ParentControllerHandleCount;
  EFI_HANDLE                  *ParentControllerHandleBuffer;
  UINTN                       ChildControllerHandleCount;
  EFI_HANDLE                  *ChildControllerHandleBuffer;
  CHAR16                      *TempStringPointer;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  UINTN                       Index;
  CHAR16                      *DriverName;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;
  UINTN                       NumberOfChildren;
  UINTN                       HandleIndex;
  UINTN                       ControllerHandleCount;
  EFI_HANDLE                  *ControllerHandleBuffer;
  UINTN                       ChildIndex;
  BOOLEAN                     Image;

  //
  // See if Handle is a device handle and display its details.
  //
  DriverBindingHandleBuffer = NULL;
  Status = PARSE_HANDLE_DATABASE_UEFI_DRIVERS (
            Handle,
            &DriverBindingHandleCount,
            &DriverBindingHandleBuffer
            );

  ParentControllerHandleBuffer = NULL;
  Status = PARSE_HANDLE_DATABASE_PARENTS (
            Handle,
            &ParentControllerHandleCount,
            &ParentControllerHandleBuffer
            );

  ChildControllerHandleBuffer = NULL;
  Status = ParseHandleDatabaseForChildControllers (
            Handle,
            &ChildControllerHandleCount,
            &ChildControllerHandleBuffer
            );

  DiagnosticsStatus = FALSE;
  ConfigurationStatus = FALSE;

  if (!EFI_ERROR(gBS->OpenProtocol(Handle, &gEfiDriverConfigurationProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
    ConfigurationStatus = TRUE;
  }
  if (!EFI_ERROR(gBS->OpenProtocol(Handle, &gEfiDriverConfiguration2ProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
    ConfigurationStatus = TRUE;
  }
  if (!EFI_ERROR(gBS->OpenProtocol(Handle, &gEfiDriverDiagnosticsProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
    DiagnosticsStatus = TRUE;
  }
  if (!EFI_ERROR(gBS->OpenProtocol(Handle, &gEfiDriverDiagnostics2ProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
    DiagnosticsStatus = TRUE;
  }

  Status = EFI_SUCCESS;

  if (DriverBindingHandleCount > 0 || ParentControllerHandleCount > 0 || ChildControllerHandleCount > 0) {



    DevicePath          = NULL;
    TempStringPointer   = NULL;
    Status              = gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID**)&DevicePath);

    Status = gEfiShellProtocol->GetDeviceName(Handle, EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8*)Language, &TempStringPointer);
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DH_OUTPUT_DRIVER1), gShellDriver1HiiHandle, TempStringPointer!=NULL?TempStringPointer:L"<Unknown>");
    SHELL_FREE_NON_NULL(TempStringPointer);
  
    TempStringPointer = gDevPathToText->ConvertDevicePathToText(DevicePath, TRUE, FALSE);
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL, 
      STRING_TOKEN (STR_DH_OUTPUT_DRIVER2), 
      gShellDriver1HiiHandle, 
      TempStringPointer!=NULL?TempStringPointer:L"<None>",
      ParentControllerHandleCount == 0?L"ROOT":(ChildControllerHandleCount > 0)?L"BUS":L"DEVICE",
      ConfigurationStatus?L"YES":L"NO",
      DiagnosticsStatus?L"YES":L"NO"
      );

    SHELL_FREE_NON_NULL(TempStringPointer);

    if (DriverBindingHandleCount == 0) {
      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER3), 
        gShellDriver1HiiHandle, 
        L"<None>"
        );
    } else {
      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER3), 
        gShellDriver1HiiHandle, 
        L""
        );
      for (Index = 0; Index < DriverBindingHandleCount; Index++) {
        Image = FALSE;
        Status = GetDriverName (
                  DriverBindingHandleBuffer[Index],
                  Language,
                  &DriverName
                  );
        if (DriverName == NULL) {
          Status = GetDriverImageName (
                    DriverBindingHandleBuffer[Index],
                    &DriverName
                    );
        }

        if (Image) {
          ShellPrintHiiEx(
            -1, 
            -1, 
            NULL, 
            STRING_TOKEN (STR_DH_OUTPUT_DRIVER4A),
            gShellDriver1HiiHandle,
            ConvertHandleToHandleIndex (DriverBindingHandleBuffer[Index]),
            DriverName!=NULL?DriverName:L"<Unknown>"
            );
        } else {
          ShellPrintHiiEx(
            -1, 
            -1, 
            NULL, 
            STRING_TOKEN (STR_DH_OUTPUT_DRIVER4B),
            gShellDriver1HiiHandle,
            ConvertHandleToHandleIndex (DriverBindingHandleBuffer[Index]),
            DriverName!=NULL?DriverName:L"<Unknown>"
            );
        }
        SHELL_FREE_NON_NULL(DriverName);
      }
    }

    if (ParentControllerHandleCount == 0) {
      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER5), 
        gShellDriver1HiiHandle, 
        L"<None>"
        );
    } else {
      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER5), 
        gShellDriver1HiiHandle, 
        L""
        );
      for (Index = 0; Index < ParentControllerHandleCount; Index++) {
        Status = gEfiShellProtocol->GetDeviceName(ParentControllerHandleBuffer[Index], EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8*)Language, &TempStringPointer);
        ShellPrintHiiEx(
          -1, 
          -1, 
          NULL, 
          STRING_TOKEN (STR_DH_OUTPUT_DRIVER5B),
          gShellDriver1HiiHandle,
          ConvertHandleToHandleIndex (ParentControllerHandleBuffer[Index]),
          TempStringPointer!=NULL?TempStringPointer:L"<Unknown>"
          );
        SHELL_FREE_NON_NULL(TempStringPointer);
      }
    }

    if (ChildControllerHandleCount == 0) {
      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER6), 
        gShellDriver1HiiHandle, 
        L"<None>"
        );
    } else {
      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER6), 
        gShellDriver1HiiHandle, 
        L""
        );
      for (Index = 0; Index < ChildControllerHandleCount; Index++) {
        Status = gEfiShellProtocol->GetDeviceName(ChildControllerHandleBuffer[Index], EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8*)Language, &TempStringPointer);
        ShellPrintHiiEx(
          -1, 
          -1, 
          NULL, 
          STRING_TOKEN (STR_DH_OUTPUT_DRIVER6B),
          gShellDriver1HiiHandle,
          ConvertHandleToHandleIndex (ChildControllerHandleBuffer[Index]),
          TempStringPointer!=NULL?TempStringPointer:L"<Unknown>"
          );
        SHELL_FREE_NON_NULL(TempStringPointer);
      }
    }
  }

  SHELL_FREE_NON_NULL(DriverBindingHandleBuffer);

  SHELL_FREE_NON_NULL(ParentControllerHandleBuffer);

  SHELL_FREE_NON_NULL(ChildControllerHandleBuffer);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // See if Handle is a driver binding handle and display its details.
  //
  Status = gBS->OpenProtocol (
                Handle,
                &gEfiDriverBindingProtocolGuid,
                (VOID **) &DriverBinding,
                NULL,
                NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL
                );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  NumberOfChildren        = 0;
  ControllerHandleBuffer  = NULL;
  Status = PARSE_HANDLE_DATABASE_DEVICES (
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

  ShellPrintHiiEx(
    -1, 
    -1, 
    NULL, 
    STRING_TOKEN (STR_DH_OUTPUT_DRIVER6B),
    gShellDriver1HiiHandle,
    ConvertHandleToHandleIndex(Handle),
    DriverName!=NULL?DriverName:L"<Unknown>"
    );
  SHELL_FREE_NON_NULL(DriverName);
  DriverName = NULL;
  Status = GetDriverImageName (
            Handle,
            &DriverName
            );
  ShellPrintHiiEx(
    -1, 
    -1, 
    NULL, 
    STRING_TOKEN (STR_DH_OUTPUT_DRIVER7B),
    gShellDriver1HiiHandle,
    DriverName!=NULL?DriverName:L"<Unknown>"
    );
  SHELL_FREE_NON_NULL(DriverName);

  ShellPrintHiiEx(
    -1, 
    -1, 
    NULL, 
    STRING_TOKEN (STR_DH_OUTPUT_DRIVER8), 
    gShellDriver1HiiHandle, 
    DriverBinding->Version,
    NumberOfChildren > 0?L"Bus":ControllerHandleCount > 0?L"Device":L"<Unknown>",
    ConfigurationStatus?L"YES":L"NO",
    DiagnosticsStatus?L"YES":L"NO"
    );

  if (ControllerHandleCount == 0) {
      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER6), 
        gShellDriver1HiiHandle, 
        L"None"
        );
  } else {
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL, 
      STRING_TOKEN (STR_DH_OUTPUT_DRIVER6), 
      gShellDriver1HiiHandle, 
      L""
      );
    for (HandleIndex = 0; HandleIndex < ControllerHandleCount; HandleIndex++) {
      Status = gEfiShellProtocol->GetDeviceName(ControllerHandleBuffer[HandleIndex], EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8*)Language, &TempStringPointer);

      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_DH_OUTPUT_DRIVER9B),
        gShellDriver1HiiHandle,
        ConvertHandleToHandleIndex(ControllerHandleBuffer[HandleIndex]),
        TempStringPointer!=NULL?TempStringPointer:L"<Unknown>"
        );
      SHELL_FREE_NON_NULL(TempStringPointer);

      Status = PARSE_HANDLE_DATABASE_MANAGED_CHILDREN (
                Handle,
                ControllerHandleBuffer[HandleIndex],
                &ChildControllerHandleCount,
                &ChildControllerHandleBuffer
                );
      if (!EFI_ERROR (Status)) {
        for (ChildIndex = 0; ChildIndex < ChildControllerHandleCount; ChildIndex++) {
          Status = gEfiShellProtocol->GetDeviceName(ChildControllerHandleBuffer[ChildIndex], EFI_DEVICE_NAME_USE_COMPONENT_NAME|EFI_DEVICE_NAME_USE_DEVICE_PATH, (CHAR8*)Language, &TempStringPointer);

          ShellPrintHiiEx(
            -1, 
            -1, 
            NULL, 
            STRING_TOKEN (STR_DH_OUTPUT_DRIVER6B),
            gShellDriver1HiiHandle,
            ConvertHandleToHandleIndex(ChildControllerHandleBuffer[ChildIndex]),
            TempStringPointer!=NULL?TempStringPointer:L"<Unknown>"
            );
          SHELL_FREE_NON_NULL(TempStringPointer);
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

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_INVALID_PARAMETER ProtocolName was NULL or invalid.
**/
SHELL_STATUS
EFIAPI
DoDhByHandle(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST BOOLEAN    Verbose,
  IN CONST BOOLEAN    Sfo,
  IN CONST CHAR8      *Language,
  IN CONST BOOLEAN    DriverInfo,
  IN CONST BOOLEAN    Multiple
  )
{
  CHAR16              *ProtocolInfoString;
  SHELL_STATUS        ShellStatus;
  EFI_STATUS          Status;

  Status              = EFI_SUCCESS;
  ShellStatus         = SHELL_SUCCESS;
  ProtocolInfoString  = NULL;

  if (!Sfo) {
    if (Multiple) {
      ProtocolInfoString = GetProtocolInfoString(TheHandle, Language, L" ", Verbose, TRUE);
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT),
        gShellDriver1HiiHandle,
        ConvertHandleToHandleIndex(TheHandle),
        ProtocolInfoString==NULL?L"":ProtocolInfoString);
    } else {
      ProtocolInfoString = GetProtocolInfoString(TheHandle, Language, L"\r\n", Verbose, TRUE);
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_SINGLE),
        gShellDriver1HiiHandle,
        ConvertHandleToHandleIndex(TheHandle),
        TheHandle,
        ProtocolInfoString==NULL?L"":ProtocolInfoString);
    }

    if (DriverInfo) {
      DisplayDriverModelHandle ((EFI_HANDLE)TheHandle, TRUE, Language);
    }
  } else {
      ProtocolInfoString = GetProtocolInfoString(TheHandle, Language, L";", FALSE, FALSE);
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_SFO),
        gShellDriver1HiiHandle,
        Multiple ?L"HandlesInfo":L"HandleInfo",
        L"DriverName",
        L"ControllerName",
        ConvertHandleToHandleIndex(TheHandle),
        L"DevPath",
        ProtocolInfoString==NULL?L"":ProtocolInfoString);


  }


  if (ProtocolInfoString != NULL) {
    FreePool(ProtocolInfoString);
  }
  return (ShellStatus);
}

/**
  Display information for all handles on a list.

  @param[in] HandleList       The NULL-terminated list of handles.
  @param[in] Verbose          TRUE for extra info, FALSE otherwise.
  @param[in] Sfo              TRUE to output in standard format output (spec).
  @param[in] Language         Language string per UEFI specification.
  @param[in] DriverInfo       TRUE to show all info about the handle.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_INVALID_PARAMETER ProtocolName was NULL or invalid.
**/
SHELL_STATUS
EFIAPI
DoDhForHandleList(
  IN CONST EFI_HANDLE *HandleList,
  IN CONST BOOLEAN    Verbose,
  IN CONST BOOLEAN    Sfo,
  IN CONST CHAR8      *Language,
  IN CONST BOOLEAN    DriverInfo
  )
{
  CONST EFI_HANDLE  *HandleWalker;
  SHELL_STATUS      ShellStatus;

  ShellStatus       = SHELL_SUCCESS;

  for (HandleWalker = HandleList ; HandleWalker != NULL && *HandleWalker != NULL && ShellStatus == SHELL_SUCCESS; HandleWalker++) {
    ShellStatus = DoDhByHandle(
          *HandleWalker,
          Verbose,
          Sfo,
          Language,
          DriverInfo,
          TRUE
         );
  }
  return (ShellStatus);
}

/**
  Display information for all handles.

  @param[in] Sfo              TRUE to output in standard format output (spec).
  @param[in] Verbose          TRUE for extra info, FALSE otherwise.
  @param[in] Language         Language string per UEFI specification.
  @param[in] DriverInfo       TRUE to show all info about the handle.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_INVALID_PARAMETER ProtocolName was NULL or invalid.
**/
SHELL_STATUS
EFIAPI
DoDhForAll(
  IN CONST BOOLEAN  Sfo,
  IN CONST BOOLEAN  Verbose,
  IN CONST CHAR8    *Language,
  IN CONST BOOLEAN  DriverInfo
  )
{
  EFI_HANDLE    *HandleList;
  SHELL_STATUS  ShellStatus;

  HandleList = GetHandleListByProtocol(NULL);

  ShellStatus = DoDhForHandleList(
    HandleList,
    Verbose,
    Sfo,
    Language,
    DriverInfo);

  FreePool(HandleList);

  return (ShellStatus);
}

/**
  Display information for all handles which have a specific protocol.

  @param[in] ProtocolName     The pointer to the name of the protocol.
  @param[in] Verbose          TRUE for extra info, FALSE otherwise.
  @param[in] Sfo              TRUE to output in standard format output (spec).
  @param[in] Language         Language string per UEFI specification.
  @param[in] DriverInfo       TRUE to show all info about the handle.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_INVALID_PARAMETER ProtocolName was NULL or invalid.
**/
SHELL_STATUS
EFIAPI
DoDhByProtocol(
  IN CONST CHAR16   *ProtocolName,
  IN CONST BOOLEAN  Verbose,
  IN CONST BOOLEAN  Sfo,
  IN CONST CHAR8    *Language,
  IN CONST BOOLEAN  DriverInfo
  )
{
  EFI_GUID      *Guid;
  EFI_STATUS    Status;
  EFI_HANDLE    *HandleList;
  SHELL_STATUS  ShellStatus;

  if (ProtocolName == NULL) {
    return (SHELL_INVALID_PARAMETER);
  }

  Status = GetGuidFromStringName(ProtocolName, Language, &Guid);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DH_NO_GUID_FOUND), gShellDriver1HiiHandle, ProtocolName);
    return (SHELL_INVALID_PARAMETER);
  }

  HandleList = GetHandleListByProtocol(Guid);

  ShellStatus = DoDhForHandleList(
    HandleList,
    Verbose,
    Sfo,
    Language,
    DriverInfo);

  SHELL_FREE_NON_NULL(HandleList);

  return (ShellStatus);
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
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CHAR8               *Language;
  CONST CHAR16        *Lang;
  CONST CHAR16        *Temp2;
  BOOLEAN             SfoMode;
  BOOLEAN             FlagD;
  BOOLEAN             Verbose;
  UINT64              Intermediate;

  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
  Language            = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
      ShellCommandLineFreeVarList (Package);
      return (SHELL_INVALID_PARAMETER);
    }

    Lang = ShellCommandLineGetValue(Package, L"-l");
    if (Lang != NULL) {
      Language = AllocateZeroPool(StrSize(Lang));
      AsciiSPrint(Language, StrSize(Lang), "%S", Lang);
    } else if (!ShellCommandLineGetFlag(Package, L"-l")){
      Language = AllocateZeroPool(10);
      AsciiSPrint(Language, 10, "en-us");
    } else {
      ASSERT(Language == NULL);
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"-l");
      ShellCommandLineFreeVarList (Package);
      return (SHELL_INVALID_PARAMETER);
    }

    SfoMode = ShellCommandLineGetFlag(Package, L"-sfo");
    FlagD   = ShellCommandLineGetFlag(Package, L"-d");
    Verbose = (BOOLEAN)(ShellCommandLineGetFlag(Package, L"-v") || ShellCommandLineGetFlag(Package, L"-verbose"));

    if (ShellCommandLineGetFlag(Package, L"-p")) {
      if (ShellCommandLineGetCount(Package) > 1) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (ShellCommandLineGetValue(Package, L"-p") == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"-p");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // print by protocol
        //
        ShellStatus = DoDhByProtocol(
          ShellCommandLineGetValue(Package, L"-p"),
          Verbose,
          SfoMode,
          Lang==NULL?NULL:Language,
          FlagD
         );
      }
    } else {
      Temp2 = ShellCommandLineGetRawValue(Package, 1);
      if (Temp2 == NULL) {
        //
        // Print everything
        //
        ShellStatus = DoDhForAll(
          SfoMode,
          Verbose,
          Lang==NULL?NULL:Language,
          FlagD
         );
      } else {
        Status = ShellConvertStringToUint64(Temp2, &Intermediate, TRUE, FALSE);
        if (EFI_ERROR(Status) || ConvertHandleIndexToHandle((UINTN)Intermediate) == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Temp2);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          //
          // print 1 handle
          //
          ShellStatus = DoDhByHandle(
            ConvertHandleIndexToHandle((UINTN)Intermediate),
            Verbose,
            SfoMode,
            Lang==NULL?NULL:Language,
            FlagD,
            FALSE
           );
        }
      }
    }


    ShellCommandLineFreeVarList (Package);
    SHELL_FREE_NON_NULL(Language);
  }

  return (ShellStatus);
}
