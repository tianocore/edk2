/** @file
The functions for Boot Maintainence Main menu.

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BootMaintenanceManager.h"

#define FRONT_PAGE_KEY_OFFSET          0x4000
//
// Boot video resolution and text mode.
//
UINT32    mBmmBootHorizontalResolution    = 0;
UINT32    mBmmBootVerticalResolution      = 0;
UINT32    mBmmBootTextModeColumn          = 0;
UINT32    mBmmBootTextModeRow             = 0;
//
// BIOS setup video resolution and text mode.
//
UINT32    mBmmSetupTextModeColumn         = 0;
UINT32    mBmmSetupTextModeRow            = 0;
UINT32    mBmmSetupHorizontalResolution   = 0;
UINT32    mBmmSetupVerticalResolution     = 0;

BOOLEAN   mBmmModeInitialized             = FALSE;

EFI_DEVICE_PATH_PROTOCOL  EndDevicePath[] = {
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

HII_VENDOR_DEVICE_PATH  mBmmHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {165A028F-0BB2-4b5f-8747-77592E3F6499}
    //
    { 0x165a028f, 0xbb2, 0x4b5f, { 0x87, 0x47, 0x77, 0x59, 0x2e, 0x3f, 0x64, 0x99 } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

EFI_GUID mBootMaintGuid          = BOOT_MAINT_FORMSET_GUID;

CHAR16  mBootMaintStorageName[]     = L"BmmData";
BMM_CALLBACK_DATA  gBootMaintenancePrivate = {
  BMM_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    BootMaintExtractConfig,
    BootMaintRouteConfig,
    BootMaintCallback
  }
};

BMM_CALLBACK_DATA *mBmmCallbackInfo = &gBootMaintenancePrivate;
BOOLEAN  mAllMenuInit               = FALSE;
BOOLEAN  mFirstEnterBMMForm         = FALSE;

/**
  Init all memu.

  @param CallbackData    The BMM context data.

**/
VOID
InitAllMenu (
  IN  BMM_CALLBACK_DATA    *CallbackData
  );

/**
  Free up all Menu Option list.

**/
VOID
FreeAllMenu (
  VOID
  );

/**

  Update the menus in the BMM page.

**/
VOID
CustomizeMenus (
  VOID
  );

/**
  This function will change video resolution and text mode
  according to defined setup mode or defined boot mode  

  @param  IsSetupMode   Indicate mode is changed to setup mode or boot mode. 

  @retval  EFI_SUCCESS  Mode is changed successfully.
  @retval  Others       Mode failed to be changed.

**/
EFI_STATUS
BmmSetConsoleMode (
  BOOLEAN  IsSetupMode
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL       *SimpleTextOut;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  UINT32                                MaxGopMode;
  UINT32                                MaxTextMode;
  UINT32                                ModeNumber;
  UINT32                                NewHorizontalResolution;
  UINT32                                NewVerticalResolution;
  UINT32                                NewColumns;
  UINT32                                NewRows;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *HandleBuffer;
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 CurrentColumn;
  UINTN                                 CurrentRow;  

  MaxGopMode  = 0;
  MaxTextMode = 0;

  //
  // Get current video resolution and text mode 
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID**)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID**)&SimpleTextOut
                  );
  if (EFI_ERROR (Status)) {
    SimpleTextOut = NULL;
  }  

  if ((GraphicsOutput == NULL) || (SimpleTextOut == NULL)) {
    return EFI_UNSUPPORTED;
  }

  if (IsSetupMode) {
    //
    // The required resolution and text mode is setup mode.
    //
    NewHorizontalResolution = mBmmSetupHorizontalResolution;
    NewVerticalResolution   = mBmmSetupVerticalResolution;
    NewColumns              = mBmmSetupTextModeColumn;
    NewRows                 = mBmmSetupTextModeRow;
  } else {
    //
    // The required resolution and text mode is boot mode.
    //
    NewHorizontalResolution = mBmmBootHorizontalResolution;
    NewVerticalResolution   = mBmmBootVerticalResolution;
    NewColumns              = mBmmBootTextModeColumn;
    NewRows                 = mBmmBootTextModeRow;   
  }

  if (GraphicsOutput != NULL) {
    MaxGopMode  = GraphicsOutput->Mode->MaxMode;
  } 

  if (SimpleTextOut != NULL) {
    MaxTextMode = SimpleTextOut->Mode->MaxMode;
  }

  //
  // 1. If current video resolution is same with required video resolution,
  //    video resolution need not be changed.
  //    1.1. If current text mode is same with required text mode, text mode need not be changed.
  //    1.2. If current text mode is different from required text mode, text mode need be changed.
  // 2. If current video resolution is different from required video resolution, we need restart whole console drivers.
  //
  for (ModeNumber = 0; ModeNumber < MaxGopMode; ModeNumber++) {
    Status = GraphicsOutput->QueryMode (
                       GraphicsOutput,
                       ModeNumber,
                       &SizeOfInfo,
                       &Info
                       );
    if (!EFI_ERROR (Status)) {
      if ((Info->HorizontalResolution == NewHorizontalResolution) &&
          (Info->VerticalResolution == NewVerticalResolution)) {
        if ((GraphicsOutput->Mode->Info->HorizontalResolution == NewHorizontalResolution) &&
            (GraphicsOutput->Mode->Info->VerticalResolution == NewVerticalResolution)) {
          //
          // Current resolution is same with required resolution, check if text mode need be set
          //
          Status = SimpleTextOut->QueryMode (SimpleTextOut, SimpleTextOut->Mode->Mode, &CurrentColumn, &CurrentRow);
          ASSERT_EFI_ERROR (Status);
          if (CurrentColumn == NewColumns && CurrentRow == NewRows) {
            //
            // If current text mode is same with required text mode. Do nothing
            //
            FreePool (Info);
            return EFI_SUCCESS;
          } else {
            //
            // If current text mode is different from required text mode.  Set new video mode
            //
            for (Index = 0; Index < MaxTextMode; Index++) {
              Status = SimpleTextOut->QueryMode (SimpleTextOut, Index, &CurrentColumn, &CurrentRow);
              if (!EFI_ERROR(Status)) {
                if ((CurrentColumn == NewColumns) && (CurrentRow == NewRows)) {
                  //
                  // Required text mode is supported, set it.
                  //
                  Status = SimpleTextOut->SetMode (SimpleTextOut, Index);
                  ASSERT_EFI_ERROR (Status);
                  //
                  // Update text mode PCD.
                  //
                  Status = PcdSet32S (PcdConOutColumn, mBmmSetupTextModeColumn);
                  ASSERT_EFI_ERROR (Status);
                  Status = PcdSet32S (PcdConOutRow, mBmmSetupTextModeRow);
                  ASSERT_EFI_ERROR (Status);
                  FreePool (Info);
                  return EFI_SUCCESS;
                }
              }
            }
            if (Index == MaxTextMode) {
              //
              // If required text mode is not supported, return error.
              //
              FreePool (Info);
              return EFI_UNSUPPORTED;
            }
          }
        } else {
          //
          // If current video resolution is not same with the new one, set new video resolution.
          // In this case, the driver which produces simple text out need be restarted.
          //
          Status = GraphicsOutput->SetMode (GraphicsOutput, ModeNumber);
          if (!EFI_ERROR (Status)) {
            FreePool (Info);
            break;
          }
        }
      }
      FreePool (Info);
    }
  }

  if (ModeNumber == MaxGopMode) {
    //
    // If the resolution is not supported, return error.
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Set PCD to Inform GraphicsConsole to change video resolution.
  // Set PCD to Inform Consplitter to change text mode.
  //
  Status = PcdSet32S (PcdVideoHorizontalResolution, NewHorizontalResolution);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdVideoVerticalResolution, NewVerticalResolution);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdConOutColumn, NewColumns);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdConOutRow, NewRows);
  ASSERT_EFI_ERROR (Status);

  //
  // Video mode is changed, so restart graphics console driver and higher level driver.
  // Reconnect graphics console driver and higher level driver.
  // Locate all the handles with GOP protocol and reconnect it.
  //
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiSimpleTextOutProtocolGuid,
                   NULL,
                   &HandleCount,
                   &HandleBuffer
                   );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
    }
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
  }

  return EFI_SUCCESS;
}

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath      A pointer to the device path structure.

  @return             A new allocated Unicode string that represents the device path.

**/
CHAR16 *
UiDevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *ToText;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DevPathToText;

  if (DevPath == NULL) {
    return NULL;
  }

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **) &DevPathToText
                  );
  ASSERT_EFI_ERROR (Status);
  ToText = DevPathToText->ConvertDevicePathToText (
                            DevPath,
                            FALSE,
                            TRUE
                            );
  ASSERT (ToText != NULL);
  return ToText;
}

/**
  Extract filename from device path. The returned buffer is allocated using AllocateCopyPool.
  The caller is responsible for freeing the allocated buffer using FreePool().

  @param DevicePath       Device path.

  @return                 A new allocated string that represents the file name.

**/
CHAR16 *
ExtractFileNameFromDevicePath (
  IN   EFI_DEVICE_PATH_PROTOCOL *DevicePath
  )
{
  CHAR16          *String;
  CHAR16          *MatchString;
  CHAR16          *LastMatch;
  CHAR16          *FileName;
  UINTN           Length;

  ASSERT(DevicePath != NULL);

  String = UiDevicePathToStr(DevicePath);
  MatchString = String;
  LastMatch   = String;
  FileName    = NULL;

  while(MatchString != NULL){
    LastMatch   = MatchString + 1;
    MatchString = StrStr(LastMatch,L"\\");
  }

  Length = StrLen(LastMatch);
  FileName = AllocateCopyPool ((Length + 1) * sizeof(CHAR16), LastMatch);
  if (FileName != NULL) {
    *(FileName + Length) = 0;
  }

  FreePool(String);

  return FileName;
}

/**
  Extract device path for given HII handle and class guid.

  @param Handle          The HII handle.

  @retval  NULL          Fail to get the device path string.
  @return  PathString    Get the device path string.

**/
CHAR16 *
BmmExtractDevicePathFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle
  )
{
  EFI_STATUS                       Status;
  EFI_HANDLE                       DriverHandle;

  ASSERT (Handle != NULL);

  if (Handle == NULL) {
    return NULL;
  }

  Status = gHiiDatabase->GetPackageListHandle (gHiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get device path string.
  //
  return ConvertDevicePathToText(DevicePathFromHandle (DriverHandle), FALSE, FALSE);

}

/**
  Converts the unicode character of the string from uppercase to lowercase.
  This is a internal function.

  @param ConfigString  String to be converted

**/
VOID
HiiToLower (
  IN EFI_STRING  ConfigString
  )
{
  EFI_STRING  String;
  BOOLEAN     Lower;

  ASSERT (ConfigString != NULL);

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  for (String = ConfigString, Lower = FALSE; *String != L'\0'; String++) {
    if (*String == L'=') {
      Lower = TRUE;
    } else if (*String == L'&') {
      Lower = FALSE;
    } else if (Lower && *String >= L'A' && *String <= L'F') {
      *String = (CHAR16) (*String - L'A' + L'a');
    }
  }
}

/**
  Update the progress string through the offset value.

  @param Offset           The offset value
  @param Configuration    Point to the configuration string.

**/
EFI_STRING
UpdateProgress(
  IN  UINTN       Offset,
  IN  EFI_STRING  Configuration
)
{
  UINTN       Length;
  EFI_STRING  StringPtr;
  EFI_STRING  ReturnString;

  StringPtr    = NULL;
  ReturnString = NULL;

  //
  // &OFFSET=XXXX followed by a Null-terminator.
  // Length = StrLen (L"&OFFSET=") + 4 + 1
  //
  Length    = StrLen (L"&OFFSET=") + 4 + 1;

  StringPtr = AllocateZeroPool (Length * sizeof (CHAR16));

  if (StringPtr == NULL) {
    return  NULL;
  }

  UnicodeSPrint (
    StringPtr,
    (8 + 4 + 1) * sizeof (CHAR16),
    L"&OFFSET=%04x",
    Offset
    );

  ReturnString = StrStr (Configuration, StringPtr);

  if (ReturnString == NULL) {
    //
    // If doesn't find the string in Configuration, convert the string to lower case then search again.
    //
    HiiToLower (StringPtr);
    ReturnString = StrStr (Configuration, StringPtr);
  }

  FreePool (StringPtr);

  return ReturnString;
}

/**
  Update the terminal content in TerminalMenu.

  @param BmmData           The BMM fake NV data.

**/
VOID
UpdateTerminalContent (
  IN BMM_FAKE_NV_DATA       *BmmData
  )
{
  UINT16                          Index;
  BM_TERMINAL_CONTEXT             *NewTerminalContext;
  BM_MENU_ENTRY                   *NewMenuEntry;

  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&TerminalMenu, Index);
    ASSERT (NewMenuEntry != NULL);
    NewTerminalContext = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
    NewTerminalContext->BaudRateIndex = BmmData->COMBaudRate[Index];
    ASSERT (BmmData->COMBaudRate[Index] < (ARRAY_SIZE (BaudRateList)));
    NewTerminalContext->BaudRate      = BaudRateList[BmmData->COMBaudRate[Index]].Value;
    NewTerminalContext->DataBitsIndex = BmmData->COMDataRate[Index];
    ASSERT (BmmData->COMDataRate[Index] < (ARRAY_SIZE (DataBitsList)));
    NewTerminalContext->DataBits      = (UINT8) DataBitsList[BmmData->COMDataRate[Index]].Value;
    NewTerminalContext->StopBitsIndex = BmmData->COMStopBits[Index];
    ASSERT (BmmData->COMStopBits[Index] < (ARRAY_SIZE (StopBitsList)));
    NewTerminalContext->StopBits      = (UINT8) StopBitsList[BmmData->COMStopBits[Index]].Value;
    NewTerminalContext->ParityIndex   = BmmData->COMParity[Index];
    ASSERT (BmmData->COMParity[Index] < (ARRAY_SIZE (ParityList)));
    NewTerminalContext->Parity        = (UINT8) ParityList[BmmData->COMParity[Index]].Value;
    NewTerminalContext->TerminalType  = BmmData->COMTerminalType[Index];
    NewTerminalContext->FlowControl   = BmmData->COMFlowControl[Index];
    ChangeTerminalDevicePath (
      NewTerminalContext->DevicePath,
      FALSE
      );
  }
}

/**
  Update the console content in ConsoleMenu.

  @param ConsoleName       The name for the console device type.
  @param BmmData           The BMM fake NV data.

**/
VOID
UpdateConsoleContent(
  IN CHAR16                 *ConsoleName,
  IN BMM_FAKE_NV_DATA       *BmmData
  )
{
  UINT16                          Index;
  BM_CONSOLE_CONTEXT              *NewConsoleContext;
  BM_TERMINAL_CONTEXT             *NewTerminalContext;
  BM_MENU_ENTRY                   *NewMenuEntry;

  if (StrCmp (ConsoleName, L"ConIn") == 0) {
    for (Index = 0; Index < ConsoleInpMenu.MenuNumber; Index++){
      NewMenuEntry                = BOpt_GetMenuEntry(&ConsoleInpMenu, Index);
      NewConsoleContext           = (BM_CONSOLE_CONTEXT *)NewMenuEntry->VariableContext;
      ASSERT (Index < MAX_MENU_NUMBER);
      NewConsoleContext->IsActive = BmmData->ConsoleInCheck[Index];
    }
    for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
      NewMenuEntry                = BOpt_GetMenuEntry (&TerminalMenu, Index);
      NewTerminalContext          = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
      ASSERT (Index + ConsoleInpMenu.MenuNumber < MAX_MENU_NUMBER);
      NewTerminalContext->IsConIn = BmmData->ConsoleInCheck[Index + ConsoleInpMenu.MenuNumber];
    }
  }

  if (StrCmp (ConsoleName, L"ConOut") == 0) {
    for (Index = 0; Index < ConsoleOutMenu.MenuNumber; Index++){
      NewMenuEntry                = BOpt_GetMenuEntry(&ConsoleOutMenu, Index);
      NewConsoleContext           = (BM_CONSOLE_CONTEXT *)NewMenuEntry->VariableContext;
      ASSERT (Index < MAX_MENU_NUMBER);
      NewConsoleContext->IsActive = BmmData->ConsoleOutCheck[Index];
    }
    for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
      NewMenuEntry                = BOpt_GetMenuEntry (&TerminalMenu, Index);
      NewTerminalContext          = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
      ASSERT (Index + ConsoleOutMenu.MenuNumber < MAX_MENU_NUMBER);
      NewTerminalContext->IsConOut = BmmData->ConsoleOutCheck[Index + ConsoleOutMenu.MenuNumber];
    }
  }
  if (StrCmp (ConsoleName, L"ErrOut") == 0) {
    for (Index = 0; Index < ConsoleErrMenu.MenuNumber; Index++){
      NewMenuEntry                = BOpt_GetMenuEntry(&ConsoleErrMenu, Index);
      NewConsoleContext           = (BM_CONSOLE_CONTEXT *)NewMenuEntry->VariableContext;
      ASSERT (Index < MAX_MENU_NUMBER);
      NewConsoleContext->IsActive = BmmData->ConsoleErrCheck[Index];
    }
    for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
      NewMenuEntry                = BOpt_GetMenuEntry (&TerminalMenu, Index);
      NewTerminalContext          = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
      ASSERT (Index + ConsoleErrMenu.MenuNumber < MAX_MENU_NUMBER);
      NewTerminalContext->IsStdErr = BmmData->ConsoleErrCheck[Index + ConsoleErrMenu.MenuNumber];
    }
  }
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
BootMaintExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS         Status;
  UINTN              BufferSize;
  BMM_CALLBACK_DATA  *Private;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &mBootMaintGuid, mBootMaintStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  Private = BMM_CALLBACK_DATA_FROM_THIS (This);
  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize = sizeof (BMM_FAKE_NV_DATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&mBootMaintGuid, mBootMaintStorageName, Private->BmmDriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) &Private->BmmFakeNvData,
                                BufferSize,
                                Results,
                                Progress
                                );
  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }
  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job. Currently not implemented.

  @param[in]  This                Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration       A null-terminated Unicode string in
                                  <ConfigString> format.   
  @param[out] Progress            A pointer to a string filled in with the
                                  offset of the most recent '&' before the
                                  first failing name / value pair (or the
                                  beginn ing of the string if the failure
                                  is in the first name / value pair) or
                                  the terminating NULL if all was
                                  successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.  
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.
  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found.
**/
EFI_STATUS
EFIAPI
BootMaintRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN CONST EFI_STRING                     Configuration,
  OUT EFI_STRING                          *Progress
  )
{
  EFI_STATUS                      Status;
  UINTN                           BufferSize;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *ConfigRouting;
  BMM_FAKE_NV_DATA                *NewBmmData;
  BMM_FAKE_NV_DATA                *OldBmmData;
  BM_MENU_ENTRY                   *NewMenuEntry;
  BM_LOAD_CONTEXT                 *NewLoadContext;
  UINT16                          Index;
  BOOLEAN                         TerminalAttChange;
  BMM_CALLBACK_DATA               *Private;
  UINTN                           Offset;

  if (Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Configuration;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: there is no name for Name/Value storage, only GUID will be checked
  //
  if (!HiiIsConfigHdrMatch (Configuration, &mBootMaintGuid, mBootMaintStorageName)) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid, 
                  NULL, 
                  (VOID **)&ConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private = BMM_CALLBACK_DATA_FROM_THIS (This);  
  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (BMM_FAKE_NV_DATA);
  OldBmmData = &Private->BmmOldFakeNVData;
  NewBmmData = &Private->BmmFakeNvData;
  Offset     = 0;
  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  Status = ConfigRouting->ConfigToBlock (
                            ConfigRouting,
                            Configuration,
                            (UINT8 *) NewBmmData,
                            &BufferSize,
                            Progress
                            );
  ASSERT_EFI_ERROR (Status);    
  //
  // Compare new and old BMM configuration data and only do action for modified item to 
  // avoid setting unnecessary non-volatile variable
  //

  //
  // Check data which located in BMM main page and save the settings if need
  //         
  if (CompareMem (&NewBmmData->BootNext, &OldBmmData->BootNext, sizeof (NewBmmData->BootNext)) != 0) {
    Status = Var_UpdateBootNext (Private);
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, BootNext);
      goto Exit;
    }
  }

  //
  // Check data which located in Boot Options Menu and save the settings if need
  //      
  if (CompareMem (NewBmmData->BootOptionDel, OldBmmData->BootOptionDel, sizeof (NewBmmData->BootOptionDel)) != 0) {  
    for (Index = 0; 
         ((Index < BootOptionMenu.MenuNumber) && (Index < (sizeof (NewBmmData->BootOptionDel) / sizeof (NewBmmData->BootOptionDel[0])))); 
         Index ++) {
      NewMenuEntry            = BOpt_GetMenuEntry (&BootOptionMenu, Index);
      NewLoadContext          = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
      NewLoadContext->Deleted = NewBmmData->BootOptionDel[Index];
      NewBmmData->BootOptionDel[Index] = FALSE;
      NewBmmData->BootOptionDelMark[Index] = FALSE;
    }

    Status = Var_DelBootOption ();
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, BootOptionDel);
      goto Exit;
    }
  }

  if (CompareMem (NewBmmData->BootOptionOrder, OldBmmData->BootOptionOrder, sizeof (NewBmmData->BootOptionOrder)) != 0) {
    Status = Var_UpdateBootOrder (Private);
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, BootOptionOrder);
      goto Exit;
    }
  }

  if (CompareMem (&NewBmmData->BootTimeOut, &OldBmmData->BootTimeOut, sizeof (NewBmmData->BootTimeOut)) != 0){
    Status = gRT->SetVariable(
                    L"Timeout",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof(UINT16),
                    &(NewBmmData->BootTimeOut)
                    );
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, BootTimeOut);
      goto Exit;
    }
    Private->BmmOldFakeNVData.BootTimeOut = NewBmmData->BootTimeOut;
  }

  //
  // Check data which located in Driver Options Menu and save the settings if need
  //              
  if (CompareMem (NewBmmData->DriverOptionDel, OldBmmData->DriverOptionDel, sizeof (NewBmmData->DriverOptionDel)) != 0) {       
    for (Index = 0; 
         ((Index < DriverOptionMenu.MenuNumber) && (Index < (sizeof (NewBmmData->DriverOptionDel) / sizeof (NewBmmData->DriverOptionDel[0])))); 
         Index++) {
      NewMenuEntry            = BOpt_GetMenuEntry (&DriverOptionMenu, Index);
      NewLoadContext          = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
      NewLoadContext->Deleted = NewBmmData->DriverOptionDel[Index];
      NewBmmData->DriverOptionDel[Index] = FALSE;
      NewBmmData->DriverOptionDelMark[Index] = FALSE;
    }
    Status = Var_DelDriverOption ();
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, DriverOptionDel);
      goto Exit;
    }
  }

  if (CompareMem (NewBmmData->DriverOptionOrder, OldBmmData->DriverOptionOrder, sizeof (NewBmmData->DriverOptionOrder)) != 0) {  
    Status = Var_UpdateDriverOrder (Private);
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, DriverOptionOrder);
      goto Exit;
    }
  }

  if (CompareMem (&NewBmmData->ConsoleOutMode, &OldBmmData->ConsoleOutMode, sizeof (NewBmmData->ConsoleOutMode)) != 0){
    Status = Var_UpdateConMode(Private);
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, ConsoleOutMode);
      goto Exit;
    }
  }

  TerminalAttChange = FALSE;
  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {

    //
    // only need update modified items
    //
    if (CompareMem (&NewBmmData->COMBaudRate[Index], &OldBmmData->COMBaudRate[Index], sizeof (NewBmmData->COMBaudRate[Index])) == 0 &&
         CompareMem (&NewBmmData->COMDataRate[Index], &OldBmmData->COMDataRate[Index], sizeof (NewBmmData->COMDataRate[Index])) == 0 &&
         CompareMem (&NewBmmData->COMStopBits[Index], &OldBmmData->COMStopBits[Index], sizeof (NewBmmData->COMStopBits[Index])) == 0 &&
         CompareMem (&NewBmmData->COMParity[Index], &OldBmmData->COMParity[Index], sizeof (NewBmmData->COMParity[Index])) == 0 &&
         CompareMem (&NewBmmData->COMTerminalType[Index], &OldBmmData->COMTerminalType[Index], sizeof (NewBmmData->COMTerminalType[Index])) == 0 &&
         CompareMem (&NewBmmData->COMFlowControl[Index], &OldBmmData->COMFlowControl[Index], sizeof (NewBmmData->COMFlowControl[Index])) == 0) {
      continue;
    }

    TerminalAttChange = TRUE;
  }
  if (TerminalAttChange) {
    if (CompareMem (&NewBmmData->COMBaudRate[Index], &OldBmmData->COMBaudRate[Index], sizeof (NewBmmData->COMBaudRate[Index])) != 0) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, COMBaudRate);
    } else if (CompareMem (&NewBmmData->COMDataRate[Index], &OldBmmData->COMDataRate[Index], sizeof (NewBmmData->COMDataRate[Index])) != 0) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, COMDataRate);
    } else if (CompareMem (&NewBmmData->COMStopBits[Index], &OldBmmData->COMStopBits[Index], sizeof (NewBmmData->COMStopBits[Index])) != 0) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, COMStopBits);
    } else if (CompareMem (&NewBmmData->COMParity[Index], &OldBmmData->COMParity[Index], sizeof (NewBmmData->COMParity[Index])) != 0) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, COMParity);
    } else if (CompareMem (&NewBmmData->COMTerminalType[Index], &OldBmmData->COMTerminalType[Index], sizeof (NewBmmData->COMTerminalType[Index])) != 0) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, COMTerminalType);
    } else if (CompareMem (&NewBmmData->COMFlowControl[Index], &OldBmmData->COMFlowControl[Index], sizeof (NewBmmData->COMFlowControl[Index])) != 0) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, COMFlowControl);
    }
    Status = Var_UpdateConsoleInpOption ();
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
    Status = Var_UpdateConsoleOutOption ();
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
    Status = Var_UpdateErrorOutOption ();
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }
  //
  // Check data which located in Console Options Menu and save the settings if need
  //
  if (CompareMem (NewBmmData->ConsoleInCheck, OldBmmData->ConsoleInCheck, sizeof (NewBmmData->ConsoleInCheck)) != 0){
    Status = Var_UpdateConsoleInpOption();
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, ConsoleInCheck);
      goto Exit;
    }
  }

  if (CompareMem (NewBmmData->ConsoleOutCheck, OldBmmData->ConsoleOutCheck, sizeof (NewBmmData->ConsoleOutCheck)) != 0){
    Status = Var_UpdateConsoleOutOption();
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, ConsoleOutCheck);
      goto Exit;
    }
  }

  if (CompareMem (NewBmmData->ConsoleErrCheck, OldBmmData->ConsoleErrCheck, sizeof (NewBmmData->ConsoleErrCheck)) != 0){
    Status = Var_UpdateErrorOutOption();
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, ConsoleErrCheck);
      goto Exit;
    }
  }

  if (CompareMem (NewBmmData->BootDescriptionData, OldBmmData->BootDescriptionData, sizeof (NewBmmData->BootDescriptionData)) != 0 ||
       CompareMem (NewBmmData->BootOptionalData, OldBmmData->BootOptionalData, sizeof (NewBmmData->BootOptionalData)) != 0) {
    Status = Var_UpdateBootOption (Private);
    NewBmmData->BootOptionChanged = FALSE;
    if (EFI_ERROR (Status)) {
      if (CompareMem (NewBmmData->BootDescriptionData, OldBmmData->BootDescriptionData, sizeof (NewBmmData->BootDescriptionData)) != 0) {
        Offset = OFFSET_OF (BMM_FAKE_NV_DATA, BootDescriptionData);
      } else {
        Offset = OFFSET_OF (BMM_FAKE_NV_DATA, BootOptionalData);
      }
      goto Exit;
    }
    BOpt_GetBootOptions (Private);
  }

  if (CompareMem (NewBmmData->DriverDescriptionData, OldBmmData->DriverDescriptionData, sizeof (NewBmmData->DriverDescriptionData)) != 0 ||
       CompareMem (NewBmmData->DriverOptionalData, OldBmmData->DriverOptionalData, sizeof (NewBmmData->DriverOptionalData)) != 0) {
    Status = Var_UpdateDriverOption (
              Private,
              Private->BmmHiiHandle,
              NewBmmData->DriverDescriptionData,
              NewBmmData->DriverOptionalData,
              NewBmmData->ForceReconnect
              );
    NewBmmData->DriverOptionChanged = FALSE;
    NewBmmData->ForceReconnect      = TRUE;
    if (EFI_ERROR (Status)) {
      if (CompareMem (NewBmmData->DriverDescriptionData, OldBmmData->DriverDescriptionData, sizeof (NewBmmData->DriverDescriptionData)) != 0) {
        Offset = OFFSET_OF (BMM_FAKE_NV_DATA, DriverDescriptionData);
      } else {
        Offset = OFFSET_OF (BMM_FAKE_NV_DATA, DriverOptionalData);
      }
      goto Exit;
    }

    BOpt_GetDriverOptions (Private);
  }

  //
  // After user do the save action, need to update OldBmmData.
  //
  CopyMem (OldBmmData, NewBmmData, sizeof (BMM_FAKE_NV_DATA));

  return EFI_SUCCESS;

Exit:
  //
  // Fail to save the data, update the progress string.
  //
  *Progress = UpdateProgress (Offset, Configuration);
  if (Status == EFI_OUT_OF_RESOURCES) {
    return Status;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  This function processes the results of changes in configuration.


  @param This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action             Specifies the type of action taken by the browser.
  @param QuestionId         A unique value which is sent to the original exporting driver
                            so that it can identify the type of data to expect.
  @param Type               The type of value for the question.
  @param Value              A pointer to the data being sent to the original exporting driver.
  @param ActionRequest      On return, points to the action requested by the callback function.

  @retval EFI_SUCCESS           The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR      The variable could not be saved.
  @retval EFI_UNSUPPORTED       The specified Action is not supported by the callback.
  @retval EFI_INVALID_PARAMETER The parameter of Value or ActionRequest is invalid.
**/
EFI_STATUS
EFIAPI
BootMaintCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL         *This,
  IN        EFI_BROWSER_ACTION                     Action,
  IN        EFI_QUESTION_ID                        QuestionId,
  IN        UINT8                                  Type,
  IN        EFI_IFR_TYPE_VALUE                     *Value,
  OUT       EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  BMM_CALLBACK_DATA *Private;
  BM_MENU_ENTRY     *NewMenuEntry;
  BMM_FAKE_NV_DATA  *CurrentFakeNVMap;
  BMM_FAKE_NV_DATA  *OldFakeNVMap;
  UINTN             Index;
  EFI_DEVICE_PATH_PROTOCOL * File;

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED && Action != EFI_BROWSER_ACTION_FORM_OPEN) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed or the form is open.
    //
    return EFI_UNSUPPORTED;
  }

  Private        = BMM_CALLBACK_DATA_FROM_THIS (This);

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    if (QuestionId == KEY_VALUE_TRIGGER_FORM_OPEN_ACTION) {
      if (!mFirstEnterBMMForm) {
        //
        // BMMUiLib depends on LegacyUi library to show legacy menus.
        // If we want to show Legacy menus correctly in BMM page,
        // we must do it after the LegacyUi library has already been initialized.
        // Opening the BMM form is the appropriate time that the LegacyUi library has already been initialized.
        // So we do the tasks which are related to legacy menus here.
        // 1. Update the menus (including legacy munu) show in BootMiantenanceManager page.
        // 2. Re-scan the BootOption menus (including the legacy boot option).
        //
        CustomizeMenus ();
        BOpt_GetBootOptions (Private);
        mFirstEnterBMMForm = TRUE;
      }
    }
  }
  //
  // Retrieve uncommitted data from Form Browser
  //
  CurrentFakeNVMap = &Private->BmmFakeNvData;
  OldFakeNVMap     = &Private->BmmOldFakeNVData;
  HiiGetBrowserData (&mBootMaintGuid, mBootMaintStorageName, sizeof (BMM_FAKE_NV_DATA), (UINT8 *) CurrentFakeNVMap);

  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    if (Value == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    
    UpdatePageId (Private, QuestionId);

    if (QuestionId < FILE_OPTION_OFFSET) {
      if (QuestionId < CONFIG_OPTION_OFFSET) {
        switch (QuestionId) {
        case FORM_BOOT_ADD_ID:
          // Leave BMM and enter FileExplorer. 
          ChooseFile (NULL, L".efi", CreateBootOptionFromFile, &File);
          break;

        case FORM_DRV_ADD_FILE_ID:
          // Leave BMM and enter FileExplorer.
          ChooseFile (NULL, L".efi", CreateDriverOptionFromFile, &File);
          break;

        case FORM_DRV_ADD_HANDLE_ID:
          CleanUpPage (FORM_DRV_ADD_HANDLE_ID, Private);
          UpdateDrvAddHandlePage (Private);
          break;

        case FORM_BOOT_DEL_ID:
          CleanUpPage (FORM_BOOT_DEL_ID, Private);
          UpdateBootDelPage (Private);
          break;

        case FORM_BOOT_CHG_ID:
        case FORM_DRV_CHG_ID:
          UpdatePageBody (QuestionId, Private);
          break;

        case FORM_DRV_DEL_ID:
          CleanUpPage (FORM_DRV_DEL_ID, Private);
          UpdateDrvDelPage (Private);
          break;

        case FORM_CON_IN_ID:
        case FORM_CON_OUT_ID:
        case FORM_CON_ERR_ID:
          UpdatePageBody (QuestionId, Private);
          break;

        case FORM_CON_MODE_ID:
          CleanUpPage (FORM_CON_MODE_ID, Private);
          UpdateConModePage (Private);
          break;

        case FORM_CON_COM_ID:
          CleanUpPage (FORM_CON_COM_ID, Private);
          UpdateConCOMPage (Private);
          break;

        default:
          break;
        }
      } else if ((QuestionId >= TERMINAL_OPTION_OFFSET) && (QuestionId < CONSOLE_OPTION_OFFSET)) {
        Index                  = (UINT16) (QuestionId - TERMINAL_OPTION_OFFSET);
        Private->CurrentTerminal  = Index;

        CleanUpPage (FORM_CON_COM_SETUP_ID, Private);
        UpdateTerminalPage (Private);

      } else if (QuestionId >= HANDLE_OPTION_OFFSET) {
        Index                  = (UINT16) (QuestionId - HANDLE_OPTION_OFFSET);

        NewMenuEntry            = BOpt_GetMenuEntry (&DriverMenu, Index);
        ASSERT (NewMenuEntry != NULL);
        Private->HandleContext  = (BM_HANDLE_CONTEXT *) NewMenuEntry->VariableContext;

        CleanUpPage (FORM_DRV_ADD_HANDLE_DESC_ID, Private);

        Private->MenuEntry                  = NewMenuEntry;
        Private->LoadContext->FilePathList  = Private->HandleContext->DevicePath;

        UpdateDriverAddHandleDescPage (Private);
      }
    }
    if (QuestionId == KEY_VALUE_BOOT_FROM_FILE){
      // Leave BMM and enter FileExplorer.
      ChooseFile (NULL, L".efi", BootFromFile, &File);
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
    }
   
    if (QuestionId == KEY_VALUE_SAVE_AND_EXIT_BOOT) {
      CleanUselessBeforeSubmit (Private);
      CurrentFakeNVMap->BootOptionChanged = FALSE;
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
    } else if (QuestionId == KEY_VALUE_SAVE_AND_EXIT_DRIVER) {
      CleanUselessBeforeSubmit (Private);
      CurrentFakeNVMap->DriverOptionChanged = FALSE;
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
    } else if (QuestionId == KEY_VALUE_NO_SAVE_AND_EXIT_DRIVER) {
      //
      // Discard changes and exit formset
      //
      ZeroMem (CurrentFakeNVMap->DriverOptionalData, sizeof (CurrentFakeNVMap->DriverOptionalData));
      ZeroMem (CurrentFakeNVMap->BootDescriptionData, sizeof (CurrentFakeNVMap->BootDescriptionData));
      ZeroMem (OldFakeNVMap->DriverOptionalData, sizeof (OldFakeNVMap->DriverOptionalData));
      ZeroMem (OldFakeNVMap->DriverDescriptionData, sizeof (OldFakeNVMap->DriverDescriptionData));
      CurrentFakeNVMap->DriverOptionChanged = FALSE;
      CurrentFakeNVMap->ForceReconnect      = TRUE;
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
    } else if (QuestionId == KEY_VALUE_NO_SAVE_AND_EXIT_BOOT) {
      //
      // Discard changes and exit formset
      //
      ZeroMem (CurrentFakeNVMap->BootOptionalData, sizeof (CurrentFakeNVMap->BootOptionalData));
      ZeroMem (CurrentFakeNVMap->BootDescriptionData, sizeof (CurrentFakeNVMap->BootDescriptionData));
      ZeroMem (OldFakeNVMap->BootOptionalData, sizeof (OldFakeNVMap->BootOptionalData));
      ZeroMem (OldFakeNVMap->BootDescriptionData, sizeof (OldFakeNVMap->BootDescriptionData));
      CurrentFakeNVMap->BootOptionChanged = FALSE;
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
    } else if (QuestionId == KEY_VALUE_BOOT_DESCRIPTION || QuestionId == KEY_VALUE_BOOT_OPTION) {
      CurrentFakeNVMap->BootOptionChanged = TRUE;
    } else if (QuestionId == KEY_VALUE_DRIVER_DESCRIPTION || QuestionId == KEY_VALUE_DRIVER_OPTION) {
      CurrentFakeNVMap->DriverOptionChanged = TRUE;
    } 

    if ((QuestionId >= BOOT_OPTION_DEL_QUESTION_ID) && (QuestionId < BOOT_OPTION_DEL_QUESTION_ID + MAX_MENU_NUMBER)) {
      if (Value->b){
        //
        // Means user try to delete this boot option but not press F10 or "Commit Changes and Exit" menu.
        //
        CurrentFakeNVMap->BootOptionDelMark[QuestionId - BOOT_OPTION_DEL_QUESTION_ID] = TRUE;
      } else {
        //
        // Means user remove the old check status.
        //
        CurrentFakeNVMap->BootOptionDelMark[QuestionId - BOOT_OPTION_DEL_QUESTION_ID] = FALSE;
      }
    } else if ((QuestionId >= DRIVER_OPTION_DEL_QUESTION_ID) && (QuestionId < DRIVER_OPTION_DEL_QUESTION_ID + MAX_MENU_NUMBER)) {
      if (Value->b){
        CurrentFakeNVMap->DriverOptionDelMark[QuestionId - DRIVER_OPTION_DEL_QUESTION_ID] = TRUE;
      } else {
        CurrentFakeNVMap->DriverOptionDelMark[QuestionId - DRIVER_OPTION_DEL_QUESTION_ID] = FALSE;
      }
    } else {
      switch (QuestionId) {
      case KEY_VALUE_SAVE_AND_EXIT:
      case KEY_VALUE_NO_SAVE_AND_EXIT:
        if (QuestionId == KEY_VALUE_SAVE_AND_EXIT) {
          CleanUselessBeforeSubmit (Private);
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
        } else if (QuestionId == KEY_VALUE_NO_SAVE_AND_EXIT) {
          DiscardChangeHandler (Private, CurrentFakeNVMap);
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
        }

        break;

      case FORM_RESET:
        gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
        return EFI_UNSUPPORTED;

      default:
        break;
      }
    }
    //
    // Update the content in Terminal menu and Console menu here.
    //
    if (QuestionId == COM_BAUD_RATE_QUESTION_ID + Private->CurrentTerminal || QuestionId == COM_DATA_RATE_QUESTION_ID + Private->CurrentTerminal ||
      QuestionId == COM_PARITY_QUESTION_ID + Private->CurrentTerminal || QuestionId == COM_STOP_BITS_QUESTION_ID + Private->CurrentTerminal ||
      QuestionId == COM_TERMINAL_QUESTION_ID + Private->CurrentTerminal || QuestionId == COM_FLOWCONTROL_QUESTION_ID + Private->CurrentTerminal
    ) {
      UpdateTerminalContent(CurrentFakeNVMap);
    }
    if ((QuestionId >= CON_IN_DEVICE_QUESTION_ID) && (QuestionId < CON_IN_DEVICE_QUESTION_ID + MAX_MENU_NUMBER)) {
      UpdateConsoleContent (L"ConIn",CurrentFakeNVMap);
    } else if ((QuestionId >= CON_OUT_DEVICE_QUESTION_ID) && (QuestionId < CON_OUT_DEVICE_QUESTION_ID + MAX_MENU_NUMBER)) {
      UpdateConsoleContent (L"ConOut", CurrentFakeNVMap);
    } else if ((QuestionId >= CON_ERR_DEVICE_QUESTION_ID) && (QuestionId < CON_ERR_DEVICE_QUESTION_ID + MAX_MENU_NUMBER)) {
      UpdateConsoleContent (L"ErrOut", CurrentFakeNVMap);
    }
  }

  //
  // Pass changed uncommitted data back to Form Browser
  //
  HiiSetBrowserData (&mBootMaintGuid, mBootMaintStorageName, sizeof (BMM_FAKE_NV_DATA), (UINT8 *) CurrentFakeNVMap, NULL);

  return EFI_SUCCESS;
}

/**
  Discard all changes done to the BMM pages such as Boot Order change,
  Driver order change.

  @param Private            The BMM context data.
  @param CurrentFakeNVMap   The current Fack NV Map.

**/
VOID
DiscardChangeHandler (
  IN  BMM_CALLBACK_DATA               *Private,
  IN  BMM_FAKE_NV_DATA                *CurrentFakeNVMap
  )
{
  UINT16  Index;

  switch (Private->BmmPreviousPageId) {
  case FORM_BOOT_CHG_ID:
    CopyMem (CurrentFakeNVMap->BootOptionOrder, Private->BmmOldFakeNVData.BootOptionOrder, sizeof (CurrentFakeNVMap->BootOptionOrder));
    break;

  case FORM_DRV_CHG_ID:
    CopyMem (CurrentFakeNVMap->DriverOptionOrder, Private->BmmOldFakeNVData.DriverOptionOrder, sizeof (CurrentFakeNVMap->DriverOptionOrder));
    break;

  case FORM_BOOT_DEL_ID:
    ASSERT (BootOptionMenu.MenuNumber <= (sizeof (CurrentFakeNVMap->BootOptionDel) / sizeof (CurrentFakeNVMap->BootOptionDel[0])));
    for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
      CurrentFakeNVMap->BootOptionDel[Index] = FALSE;
    }
    break;

  case FORM_DRV_DEL_ID:
    ASSERT (DriverOptionMenu.MenuNumber <= (sizeof (CurrentFakeNVMap->DriverOptionDel) / sizeof (CurrentFakeNVMap->DriverOptionDel[0])));
    for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
      CurrentFakeNVMap->DriverOptionDel[Index] = FALSE;
    }
    break;

  case FORM_BOOT_NEXT_ID:
    CurrentFakeNVMap->BootNext = Private->BmmOldFakeNVData.BootNext;
    break;

  case FORM_TIME_OUT_ID:
    CurrentFakeNVMap->BootTimeOut = Private->BmmOldFakeNVData.BootTimeOut;
    break;

  case FORM_DRV_ADD_HANDLE_DESC_ID:
  case FORM_DRV_ADD_FILE_ID:
  case FORM_DRV_ADD_HANDLE_ID:
    CurrentFakeNVMap->DriverAddHandleDesc[0]          = 0x0000;
    CurrentFakeNVMap->DriverAddHandleOptionalData[0]  = 0x0000;
    break;

  default:
    break;
  }
}

/**
  This function is to clean some useless data before submit changes.

  @param Private            The BMM context data.

**/
VOID
CleanUselessBeforeSubmit (
  IN  BMM_CALLBACK_DATA               *Private
  )
{
  UINT16  Index;
  if (Private->BmmPreviousPageId != FORM_BOOT_DEL_ID) {
    for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
      if (Private->BmmFakeNvData.BootOptionDel[Index] && !Private->BmmFakeNvData.BootOptionDelMark[Index]) {
        Private->BmmFakeNvData.BootOptionDel[Index] = FALSE;
        Private->BmmOldFakeNVData.BootOptionDel[Index] = FALSE;
      }
    }
  }
  if (Private->BmmPreviousPageId != FORM_DRV_DEL_ID) {
    for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
      if (Private->BmmFakeNvData.DriverOptionDel[Index] && !Private->BmmFakeNvData.DriverOptionDelMark[Index]) {
        Private->BmmFakeNvData.DriverOptionDel[Index] = FALSE;
        Private->BmmOldFakeNVData.DriverOptionDel[Index] = FALSE;
      }
    }
  }
}

/**

  Update the menus in the BMM page.

**/
VOID
CustomizeMenus (
  VOID
  )
{
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartGuidLabel;
  EFI_IFR_GUID_LABEL          *EndGuidLabel;

  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);
  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartGuidLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartGuidLabel->Number       = LABEL_FORM_MAIN_START;
  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndGuidLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndGuidLabel->Number       = LABEL_FORM_MAIN_END;

  //
  //Updata Front Page form
  //
  UiCustomizeBMMPage (
    mBmmCallbackInfo->BmmHiiHandle,
    StartOpCodeHandle
    );

  HiiUpdateForm (
    mBmmCallbackInfo->BmmHiiHandle,
    &mBootMaintGuid,
    FORM_MAIN_ID,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}

/**
   Create dynamic code for BMM and initialize all of BMM configuration data in BmmFakeNvData and
   BmmOldFakeNVData member in BMM context data.

  @param CallbackData    The BMM context data.

**/
VOID
InitializeBmmConfig (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  UINT16          Index;

  ASSERT (CallbackData != NULL);

  //
  // Initialize data which located in BMM main page
  //
  CallbackData->BmmFakeNvData.BootNext = NONE_BOOTNEXT_VALUE;
  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry    = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    NewLoadContext  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;

    if (NewLoadContext->IsBootNext) {
      CallbackData->BmmFakeNvData.BootNext = Index;
      break;
    }
  }

  CallbackData->BmmFakeNvData.BootTimeOut = PcdGet16 (PcdPlatformBootTimeOut);

  //
  // Initialize data which located in Boot Options Menu
  //
  GetBootOrder (CallbackData);

  //
  // Initialize data which located in Driver Options Menu
  //
  GetDriverOrder (CallbackData);

  //
  // Initialize data which located in Console Options Menu
  //
  GetConsoleOutMode (CallbackData);
  GetConsoleInCheck (CallbackData);
  GetConsoleOutCheck (CallbackData);
  GetConsoleErrCheck (CallbackData);
  GetTerminalAttribute (CallbackData);

  CallbackData->BmmFakeNvData.ForceReconnect = TRUE;

  //
  // Backup Initialize BMM configuartion data to BmmOldFakeNVData
  //
  CopyMem (&CallbackData->BmmOldFakeNVData, &CallbackData->BmmFakeNvData, sizeof (BMM_FAKE_NV_DATA));
}

/**
  Initialized all Menu Option List.

  @param CallbackData    The BMM context data.

**/
VOID
InitAllMenu (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  InitializeListHead (&BootOptionMenu.Head);
  InitializeListHead (&DriverOptionMenu.Head);
  BOpt_GetBootOptions (CallbackData);
  BOpt_GetDriverOptions (CallbackData);
  BOpt_FindDrivers ();
  InitializeListHead (&ConsoleInpMenu.Head);
  InitializeListHead (&ConsoleOutMenu.Head);
  InitializeListHead (&ConsoleErrMenu.Head);
  InitializeListHead (&TerminalMenu.Head);
  LocateSerialIo ();
  GetAllConsoles ();
  mAllMenuInit = TRUE;
}

/**
  Free up all Menu Option list.

**/
VOID
FreeAllMenu (
  VOID
  )
{
  if (!mAllMenuInit){
    return;
  }
  BOpt_FreeMenu (&BootOptionMenu);
  BOpt_FreeMenu (&DriverOptionMenu);
  BOpt_FreeMenu (&DriverMenu);
  FreeAllConsoles ();
  mAllMenuInit = FALSE;
}

/**
  Initial the boot mode related parameters.

**/
VOID
BmmInitialBootModeInfo (
  VOID
  )
{
  EFI_STATUS                         Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *SimpleTextOut;
  UINTN                              BootTextColumn;
  UINTN                              BootTextRow;

  if (mBmmModeInitialized) {
    return;
  }

  //
  // After the console is ready, get current video resolution
  // and text mode before launching setup at first time.
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID**)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID**)&SimpleTextOut
                  );
  if (EFI_ERROR (Status)) {
    SimpleTextOut = NULL;
  }

  if (GraphicsOutput != NULL) {
    //
    // Get current video resolution and text mode.
    //
    mBmmBootHorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
    mBmmBootVerticalResolution   = GraphicsOutput->Mode->Info->VerticalResolution;
  }

  if (SimpleTextOut != NULL) {
    Status = SimpleTextOut->QueryMode (
                              SimpleTextOut,
                              SimpleTextOut->Mode->Mode,
                              &BootTextColumn,
                              &BootTextRow
                              );
    mBmmBootTextModeColumn = (UINT32)BootTextColumn;
    mBmmBootTextModeRow    = (UINT32)BootTextRow;
  }

  //
  // Get user defined text mode for setup.
  //
  mBmmSetupHorizontalResolution = PcdGet32 (PcdSetupVideoHorizontalResolution);
  mBmmSetupVerticalResolution   = PcdGet32 (PcdSetupVideoVerticalResolution);
  mBmmSetupTextModeColumn       = PcdGet32 (PcdSetupConOutColumn);
  mBmmSetupTextModeRow          = PcdGet32 (PcdSetupConOutRow);

  mBmmModeInitialized           = TRUE;
}

/**

  Install Boot Maintenance Manager Menu driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Install Boot manager menu success.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
BootMaintenanceManagerUiLibConstructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )

{
  EFI_STATUS               Status;
  UINT8                    *Ptr;

  Status = EFI_SUCCESS;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mBmmCallbackInfo->BmmDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mBmmHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mBmmCallbackInfo->BmmConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Post our Boot Maint VFR binary to the HII database.
  //
  mBmmCallbackInfo->BmmHiiHandle = HiiAddPackages (
                                    &mBootMaintGuid,
                                    mBmmCallbackInfo->BmmDriverHandle,
                                    BootMaintenanceManagerBin,
                                    BootMaintenanceManagerUiLibStrings,
                                    NULL
                                    );
  ASSERT (mBmmCallbackInfo->BmmHiiHandle != NULL);

  //
  // Locate Formbrowser2 protocol
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &mBmmCallbackInfo->FormBrowser2);
  ASSERT_EFI_ERROR (Status);

  EfiBootManagerRefreshAllBootOption ();

  //
  // Create LoadOption in BmmCallbackInfo for Driver Callback
  //
  Ptr = AllocateZeroPool (sizeof (BM_LOAD_CONTEXT) + sizeof (BM_FILE_CONTEXT) + sizeof (BM_HANDLE_CONTEXT) + sizeof (BM_MENU_ENTRY));
  ASSERT (Ptr != NULL);

  //
  // Initialize Bmm callback data.
  //
  mBmmCallbackInfo->LoadContext = (BM_LOAD_CONTEXT *) Ptr;
  Ptr += sizeof (BM_LOAD_CONTEXT);

  mBmmCallbackInfo->FileContext = (BM_FILE_CONTEXT *) Ptr;
  Ptr += sizeof (BM_FILE_CONTEXT);

  mBmmCallbackInfo->HandleContext = (BM_HANDLE_CONTEXT *) Ptr;
  Ptr += sizeof (BM_HANDLE_CONTEXT);

  mBmmCallbackInfo->MenuEntry     = (BM_MENU_ENTRY *) Ptr;

  mBmmCallbackInfo->BmmPreviousPageId  = FORM_MAIN_ID;
  mBmmCallbackInfo->BmmCurrentPageId   = FORM_MAIN_ID;

  InitAllMenu (mBmmCallbackInfo);

  CreateUpdateData();
  //
  // Update boot maintenance manager page 
  //
  InitializeBmmConfig(mBmmCallbackInfo);

  BmmInitialBootModeInfo();

  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param ImageHandle       Handle that identifies the image to be unloaded.
  @param  SystemTable      The system table.

  @retval EFI_SUCCESS      The image has been unloaded.

**/
EFI_STATUS
EFIAPI
BootMaintenanceManagerUiLibDestructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )

{
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  if (mEndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mEndOpCodeHandle);
  }

  FreeAllMenu ();

  //
  // Remove our IFR data from HII database
  //
  HiiRemovePackages (mBmmCallbackInfo->BmmHiiHandle);

  gBS->UninstallMultipleProtocolInterfaces (
         mBmmCallbackInfo->BmmDriverHandle,
         &gEfiDevicePathProtocolGuid,
         &mBmmHiiVendorDevicePath,
         &gEfiHiiConfigAccessProtocolGuid,
         &mBmmCallbackInfo->BmmConfigAccess,
         NULL
         );

  FreePool (mBmmCallbackInfo->LoadContext);

  return EFI_SUCCESS;
}

