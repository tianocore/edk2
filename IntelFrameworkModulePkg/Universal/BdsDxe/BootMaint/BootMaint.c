/** @file
  The functions for Boot Maintainence Main menu.

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BootMaint.h"
#include "FormGuid.h"
#include "Bds.h"
#include "FrontPage.h"

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
    BOOT_MAINT_FORMSET_GUID
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

HII_VENDOR_DEVICE_PATH  mFeHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    FILE_EXPLORE_FORMSET_GUID
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

CHAR16  mBootMaintStorageName[]     = L"BmmData";
CHAR16  mFileExplorerStorageName[]  = L"FeData";
BMM_CALLBACK_DATA *mBmmCallbackInfo = NULL;

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
  Initialize all of BMM configuration data in BmmFakeNvData and BmmOldFakeNVData member
  in BMM context data and create all of dynamic OP code for BMM.

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
  CallbackData->BmmFakeNvData.BootNext = (UINT16) (BootOptionMenu.MenuNumber);
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
  GetLegacyDeviceOrder (CallbackData);

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

  //
  // Backup Initialize BMM configuartion data to BmmOldFakeNVData
  //
  CopyMem (&CallbackData->BmmOldFakeNVData, &CallbackData->BmmFakeNvData, sizeof (BMM_FAKE_NV_DATA));
}

/**
  Create string tokens for a menu from its help strings and display strings

  @param CallbackData       The BMM context data.
  @param HiiHandle          Hii Handle of the package to be updated.
  @param MenuOption         The Menu whose string tokens need to be created

  @retval  EFI_SUCCESS      String tokens created successfully
  @retval  others           contain some errors
**/
EFI_STATUS
CreateMenuStringToken (
  IN BMM_CALLBACK_DATA                *CallbackData,
  IN EFI_HII_HANDLE                   HiiHandle,
  IN BM_MENU_OPTION                   *MenuOption
  )
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINTN         Index;

  for (Index = 0; Index < MenuOption->MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (MenuOption, Index);

    NewMenuEntry->DisplayStringToken = HiiSetString (
                                         HiiHandle,
                                         0,
                                         NewMenuEntry->DisplayString,
                                         NULL
                                         );

    if (NULL == NewMenuEntry->HelpString) {
      NewMenuEntry->HelpStringToken = NewMenuEntry->DisplayStringToken;
    } else {
      NewMenuEntry->HelpStringToken = HiiSetString (
                                        HiiHandle,
                                        0,
                                        NewMenuEntry->HelpString,
                                        NULL
                                        );
    }
  }

  return EFI_SUCCESS;
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
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gBootMaintFormSetGuid, mBootMaintStorageName)) {
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
    ConfigRequestHdr = HiiConstructConfigHdr (&gBootMaintFormSetGuid, mBootMaintStorageName, Private->BmmDriverHandle);
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
  BM_CONSOLE_CONTEXT              *NewConsoleContext;
  BM_TERMINAL_CONTEXT             *NewTerminalContext;
  BM_MENU_ENTRY                   *NewMenuEntry;
  BM_LOAD_CONTEXT                 *NewLoadContext;
  UINT16                          Index;  
  BOOLEAN                         TerminalAttChange;
  BMM_CALLBACK_DATA               *Private;  

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
  if (!HiiIsConfigHdrMatch (Configuration, &gBootMaintFormSetGuid, mBootMaintStorageName)) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid, 
                  NULL, 
                  (VOID**) &ConfigRouting
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
  if (CompareMem (NewBmmData->LegacyFD, OldBmmData->LegacyFD, sizeof (NewBmmData->LegacyFD)) != 0) {
    Var_UpdateBBSOption (Private, FORM_SET_FD_ORDER_ID);
  }

  if (CompareMem (NewBmmData->LegacyHD, OldBmmData->LegacyHD, sizeof (NewBmmData->LegacyHD)) != 0) {
    Var_UpdateBBSOption (Private, FORM_SET_HD_ORDER_ID);
  }

  if (CompareMem (NewBmmData->LegacyCD, OldBmmData->LegacyCD, sizeof (NewBmmData->LegacyCD)) != 0) {
    Var_UpdateBBSOption (Private, FORM_SET_CD_ORDER_ID);
  }

  if (CompareMem (NewBmmData->LegacyNET, OldBmmData->LegacyNET, sizeof (NewBmmData->LegacyNET)) != 0) {
    Var_UpdateBBSOption (Private, FORM_SET_NET_ORDER_ID);
  }

  if (CompareMem (NewBmmData->LegacyBEV, OldBmmData->LegacyBEV, sizeof (NewBmmData->LegacyBEV)) != 0) {
    Var_UpdateBBSOption (Private, FORM_SET_BEV_ORDER_ID);
  }

  //
  // Change for "delete boot option" page need update NewBmmData->BootOptionOrder, so process 
  // NewBmmData->BootOptionOrder before NewBmmData->BootOptionDel
  //
  if (CompareMem (NewBmmData->BootOptionOrder, OldBmmData->BootOptionOrder, sizeof (NewBmmData->BootOptionOrder)) != 0) {  
    Status = Var_UpdateBootOrder (Private);
  }

  //
  // Change for "delete driver option" page need update NewBmmData->DriverOptionOrder, so process 
  // NewBmmData->DriverOptionOrder before NewBmmData->DriverOptionDel
  //
  if (CompareMem (NewBmmData->DriverOptionOrder, OldBmmData->DriverOptionOrder, sizeof (NewBmmData->DriverOptionOrder)) != 0) {  
    Status = Var_UpdateDriverOrder (Private);
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

    Var_DelBootOption ();
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
    Var_DelDriverOption ();
  }

  if (CompareMem (&NewBmmData->BootTimeOut, &OldBmmData->BootTimeOut, sizeof (NewBmmData->BootTimeOut)) != 0) {
    Status = gRT->SetVariable (
                    L"Timeout",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof (UINT16),
                    &(NewBmmData->BootTimeOut)
                    );
    ASSERT_EFI_ERROR(Status);

    //
    // Bugbug: code not exit in UiApp but in IntelFrameworkModulePkg, need do more check.
    //
    Private->BmmOldFakeNVData.BootTimeOut = NewBmmData->BootTimeOut; 
  }

  if (CompareMem (&NewBmmData->BootNext, &OldBmmData->BootNext, sizeof (NewBmmData->BootNext)) != 0) {
    Status = Var_UpdateBootNext (Private);
  }

  if (CompareMem (&NewBmmData->ConsoleOutMode, &OldBmmData->ConsoleOutMode, sizeof (NewBmmData->ConsoleOutMode)) != 0) {  
    Var_UpdateConMode (Private);
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

    NewMenuEntry = BOpt_GetMenuEntry (&TerminalMenu, Index);
    ASSERT (NewMenuEntry != NULL);
    NewTerminalContext = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
    NewTerminalContext->BaudRateIndex = NewBmmData->COMBaudRate[Index];
    ASSERT (NewBmmData->COMBaudRate[Index] < (sizeof (BaudRateList) / sizeof (BaudRateList[0])));
    NewTerminalContext->BaudRate      = BaudRateList[NewBmmData->COMBaudRate[Index]].Value;
    NewTerminalContext->DataBitsIndex = NewBmmData->COMDataRate[Index];
    ASSERT (NewBmmData->COMDataRate[Index] < (sizeof (DataBitsList) / sizeof (DataBitsList[0])));
    NewTerminalContext->DataBits      = (UINT8) DataBitsList[NewBmmData->COMDataRate[Index]].Value;
    NewTerminalContext->StopBitsIndex = NewBmmData->COMStopBits[Index];
    ASSERT (NewBmmData->COMStopBits[Index] < (sizeof (StopBitsList) / sizeof (StopBitsList[0])));
    NewTerminalContext->StopBits      = (UINT8) StopBitsList[NewBmmData->COMStopBits[Index]].Value;
    NewTerminalContext->ParityIndex   = NewBmmData->COMParity[Index];
    ASSERT (NewBmmData->COMParity[Index] < (sizeof (ParityList) / sizeof (ParityList[0])));
    NewTerminalContext->Parity        = (UINT8) ParityList[NewBmmData->COMParity[Index]].Value;
    NewTerminalContext->TerminalType  = NewBmmData->COMTerminalType[Index];
    NewTerminalContext->FlowControl   = NewBmmData->COMFlowControl[Index];
    ChangeTerminalDevicePath (
      &(NewTerminalContext->DevicePath),
      FALSE
      );
    TerminalAttChange = TRUE;
  }
  if (TerminalAttChange) {
    Var_UpdateConsoleInpOption ();
    Var_UpdateConsoleOutOption ();
    Var_UpdateErrorOutOption ();
  }

  //
  // Check data which located in Console Options Menu and save the settings if need
  //
  if (CompareMem (NewBmmData->ConsoleInCheck, OldBmmData->ConsoleInCheck, sizeof (NewBmmData->ConsoleInCheck)) != 0) {
    for (Index = 0; Index < ConsoleInpMenu.MenuNumber; Index++) {
      NewMenuEntry                = BOpt_GetMenuEntry (&ConsoleInpMenu, Index);
      NewConsoleContext           = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
      ASSERT (Index < MAX_MENU_NUMBER);
      NewConsoleContext->IsActive = NewBmmData->ConsoleInCheck[Index];
    }

    Var_UpdateConsoleInpOption ();
  }

  if (CompareMem (NewBmmData->ConsoleOutCheck, OldBmmData->ConsoleOutCheck, sizeof (NewBmmData->ConsoleOutCheck)) != 0) {
    for (Index = 0; Index < ConsoleOutMenu.MenuNumber; Index++) {
      NewMenuEntry                = BOpt_GetMenuEntry (&ConsoleOutMenu, Index);
      NewConsoleContext           = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
      ASSERT (Index < MAX_MENU_NUMBER);
      NewConsoleContext->IsActive = NewBmmData->ConsoleOutCheck[Index];
    }

    Var_UpdateConsoleOutOption ();
  }

  if (CompareMem (NewBmmData->ConsoleErrCheck, OldBmmData->ConsoleErrCheck, sizeof (NewBmmData->ConsoleErrCheck)) != 0) {  
    for (Index = 0; Index < ConsoleErrMenu.MenuNumber; Index++) {
      NewMenuEntry                = BOpt_GetMenuEntry (&ConsoleErrMenu, Index);
      NewConsoleContext           = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
      ASSERT (Index < MAX_MENU_NUMBER);
      NewConsoleContext->IsActive = NewBmmData->ConsoleErrCheck[Index];
    }

    Var_UpdateErrorOutOption ();
  }

  //
  // After user do the save action, need to update OldBmmData.
  //
  CopyMem (OldBmmData, NewBmmData, sizeof (BMM_FAKE_NV_DATA));

  return EFI_SUCCESS;
}

/**
  Create GoTo OP code into FORM_BOOT_LEGACY_DEVICE label for legacy boot option.

**/
EFI_STATUS
InitializeLegacyBootOption (
  VOID
  )
{
  RefreshUpdateData ();
  mStartLabel->Number = FORM_BOOT_LEGACY_DEVICE_ID;
  
  //
  // If LegacyBios Protocol is installed, add 3 tags about legacy boot option
  // in BootOption form: legacy FD/HD/CD/NET/BEV
  //
  HiiCreateGotoOpCode (
    mStartOpCodeHandle,
    FORM_SET_FD_ORDER_ID,
    STRING_TOKEN (STR_FORM_SET_FD_ORDER_TITLE),
    STRING_TOKEN (STR_FORM_SET_FD_ORDER_TITLE),
    EFI_IFR_FLAG_CALLBACK,
    FORM_SET_FD_ORDER_ID
    );

  HiiCreateGotoOpCode (
    mStartOpCodeHandle,
    FORM_SET_HD_ORDER_ID,
    STRING_TOKEN (STR_FORM_SET_HD_ORDER_TITLE),
    STRING_TOKEN (STR_FORM_SET_HD_ORDER_TITLE),
    EFI_IFR_FLAG_CALLBACK,
    FORM_SET_HD_ORDER_ID
    );

  HiiCreateGotoOpCode (
    mStartOpCodeHandle,
    FORM_SET_CD_ORDER_ID,
    STRING_TOKEN (STR_FORM_SET_CD_ORDER_TITLE),
    STRING_TOKEN (STR_FORM_SET_CD_ORDER_TITLE),
    EFI_IFR_FLAG_CALLBACK,
    FORM_SET_CD_ORDER_ID
    );

  HiiCreateGotoOpCode (
    mStartOpCodeHandle,
    FORM_SET_NET_ORDER_ID,
    STRING_TOKEN (STR_FORM_SET_NET_ORDER_TITLE),
    STRING_TOKEN (STR_FORM_SET_NET_ORDER_TITLE),
    EFI_IFR_FLAG_CALLBACK,
    FORM_SET_NET_ORDER_ID
    );

  HiiCreateGotoOpCode (
    mStartOpCodeHandle,
    FORM_SET_BEV_ORDER_ID,
    STRING_TOKEN (STR_FORM_SET_BEV_ORDER_TITLE),
    STRING_TOKEN (STR_FORM_SET_BEV_ORDER_TITLE),
    EFI_IFR_FLAG_CALLBACK,
    FORM_SET_BEV_ORDER_ID
    );
  
  HiiUpdateForm (
    mBmmCallbackInfo->BmmHiiHandle,
    &gBootMaintFormSetGuid,
    FORM_BOOT_SETUP_ID,
    mStartOpCodeHandle, // Label FORM_BOOT_LEGACY_DEVICE_ID
    mEndOpCodeHandle    // LABEL_END
    );
  
  return EFI_SUCCESS;
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
  EFI_STATUS        Status;
  UINTN             OldValue;
  UINTN             NewValue;
  UINTN             Number;
  UINTN             Pos;
  UINTN             Bit;
  UINT16            NewValuePos;
  UINT16            Index3;  
  UINT16            Index2;
  UINT16            Index;
  UINT8             *OldLegacyDev;
  UINT8             *NewLegacyDev;
  UINT8             *DisMap;
  EFI_LEGACY_BIOS_PROTOCOL    *LegacyBios;

  Private = BMM_CALLBACK_DATA_FROM_THIS (This);
  if (Action == EFI_BROWSER_ACTION_FORM_OPEN && QuestionId == FORM_BOOT_SETUP_ID) {
    //
    // Initilize Form for legacy boot option.
    //
    Status = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
    if (!EFI_ERROR (Status)) {
      InitializeLegacyBootOption ();
    }
    
    return EFI_SUCCESS;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {    
    //
    // All other action return unsupported.
    //
    return EFI_UNSUPPORTED;
  }
  
  Status       = EFI_SUCCESS;
  OldValue     = 0;
  NewValue     = 0;
  Number       = 0;
  OldLegacyDev = NULL;
  NewLegacyDev = NULL;
  NewValuePos  = 0;
  DisMap       = NULL;

  Private      = BMM_CALLBACK_DATA_FROM_THIS (This);
  //
  // Retrive uncommitted data from Form Browser
  //
  CurrentFakeNVMap = &Private->BmmFakeNvData;
  HiiGetBrowserData (&gBootMaintFormSetGuid, mBootMaintStorageName, sizeof (BMM_FAKE_NV_DATA), (UINT8 *) CurrentFakeNVMap);
  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    if (Value == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    
    UpdatePageId (Private, QuestionId);

    if (QuestionId < FILE_OPTION_OFFSET) {
      if (QuestionId < CONFIG_OPTION_OFFSET) {
        switch (QuestionId) {
        case KEY_VALUE_BOOT_FROM_FILE:
          Private->FeCurrentState = FileExplorerStateBootFromFile;
          break;

        case FORM_BOOT_ADD_ID:
          Private->FeCurrentState = FileExplorerStateAddBootOption;
          break;

        case FORM_DRV_ADD_FILE_ID:
          Private->FeCurrentState = FileExplorerStateAddDriverOptionState;
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

        case FORM_BOOT_NEXT_ID:
          CleanUpPage (FORM_BOOT_NEXT_ID, Private);
          UpdateBootNextPage (Private);
          break;

        case FORM_TIME_OUT_ID:
          CleanUpPage (FORM_TIME_OUT_ID, Private);
          UpdateTimeOutPage (Private);
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

        case FORM_SET_FD_ORDER_ID:
        case FORM_SET_HD_ORDER_ID:
        case FORM_SET_CD_ORDER_ID:
        case FORM_SET_NET_ORDER_ID:
        case FORM_SET_BEV_ORDER_ID:
          CleanUpPage (QuestionId, Private);
          UpdateSetLegacyDeviceOrderPage (QuestionId, Private);
          break;

        default:
          break;
        }
      } else if ((QuestionId >= TERMINAL_OPTION_OFFSET) && (QuestionId < CONSOLE_OPTION_OFFSET)) {
        Index2                    = (UINT16) (QuestionId - TERMINAL_OPTION_OFFSET);
        Private->CurrentTerminal  = Index2;

        CleanUpPage (FORM_CON_COM_SETUP_ID, Private);
        UpdateTerminalPage (Private);

      } else if (QuestionId >= HANDLE_OPTION_OFFSET) {
        Index2                  = (UINT16) (QuestionId - HANDLE_OPTION_OFFSET);

        NewMenuEntry            = BOpt_GetMenuEntry (&DriverMenu, Index2);
        ASSERT (NewMenuEntry != NULL);
        Private->HandleContext  = (BM_HANDLE_CONTEXT *) NewMenuEntry->VariableContext;

        CleanUpPage (FORM_DRV_ADD_HANDLE_DESC_ID, Private);

        Private->MenuEntry                  = NewMenuEntry;
        Private->LoadContext->FilePathList  = Private->HandleContext->DevicePath;

        UpdateDriverAddHandleDescPage (Private);
      }
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
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
    } else if ((QuestionId >= LEGACY_FD_QUESTION_ID) && (QuestionId < LEGACY_BEV_QUESTION_ID + MAX_MENU_NUMBER)) {
      //
      // Update Select FD/HD/CD/NET/BEV Order Form
      //

      DisMap  = Private->BmmOldFakeNVData.DisableMap;

      if (QuestionId >= LEGACY_FD_QUESTION_ID && QuestionId < LEGACY_FD_QUESTION_ID + MAX_MENU_NUMBER) {
        Number        = (UINT16) LegacyFDMenu.MenuNumber;
        OldLegacyDev  = Private->BmmOldFakeNVData.LegacyFD;
        NewLegacyDev  = CurrentFakeNVMap->LegacyFD;
      } else if (QuestionId >= LEGACY_HD_QUESTION_ID && QuestionId < LEGACY_HD_QUESTION_ID + MAX_MENU_NUMBER) {
        Number        = (UINT16) LegacyHDMenu.MenuNumber;
        OldLegacyDev  = Private->BmmOldFakeNVData.LegacyHD;
        NewLegacyDev  = CurrentFakeNVMap->LegacyHD;
      } else if (QuestionId >= LEGACY_CD_QUESTION_ID && QuestionId < LEGACY_CD_QUESTION_ID + MAX_MENU_NUMBER) {
        Number        = (UINT16) LegacyCDMenu.MenuNumber;
        OldLegacyDev  = Private->BmmOldFakeNVData.LegacyCD;
        NewLegacyDev  = CurrentFakeNVMap->LegacyCD;
      } else if (QuestionId >= LEGACY_NET_QUESTION_ID && QuestionId < LEGACY_NET_QUESTION_ID + MAX_MENU_NUMBER) {
        Number        = (UINT16) LegacyNETMenu.MenuNumber;
        OldLegacyDev  = Private->BmmOldFakeNVData.LegacyNET;
        NewLegacyDev  = CurrentFakeNVMap->LegacyNET;
      } else if (QuestionId >= LEGACY_BEV_QUESTION_ID && QuestionId < LEGACY_BEV_QUESTION_ID + MAX_MENU_NUMBER) {
        Number        = (UINT16) LegacyBEVMenu.MenuNumber;
        OldLegacyDev  = Private->BmmOldFakeNVData.LegacyBEV;
        NewLegacyDev  = CurrentFakeNVMap->LegacyBEV;
      }
      //
      //  First, find the different position
      //  if there is change, it should be only one
      //
      for (Index = 0; Index < Number; Index++) {
        if (OldLegacyDev[Index] != NewLegacyDev[Index]) {
          OldValue  = OldLegacyDev[Index];
          NewValue  = NewLegacyDev[Index];
          break;
        }
      }

      if (Index != Number) {
        //
        // there is change, now process
        //
        if (0xFF == NewValue) {
          //
          // This item will be disable
          // Just move the items behind this forward to overlap it
          //
          Pos = OldValue / 8;
          Bit = 7 - (OldValue % 8);
          DisMap[Pos] = (UINT8) (DisMap[Pos] | (UINT8) (1 << Bit));
          for (Index2 = Index; Index2 < Number - 1; Index2++) {
            NewLegacyDev[Index2] = NewLegacyDev[Index2 + 1];
          }

          NewLegacyDev[Index2] = 0xFF;
        } else {
          for (Index2 = 0; Index2 < Number; Index2++) {
            if (Index2 == Index) {
              continue;
            }

            if (OldLegacyDev[Index2] == NewValue) {
              //
              // If NewValue is in OldLegacyDev array
              // remember its old position
              //
              NewValuePos = Index2;
              break;
            }
          }

          if (Index2 != Number) {
            //
            // We will change current item to an existing item
            // (It's hard to describe here, please read code, it's like a cycle-moving)
            //
            for (Index2 = NewValuePos; Index2 != Index;) {
              if (NewValuePos < Index) {
                NewLegacyDev[Index2] = OldLegacyDev[Index2 + 1];
                Index2++;
              } else {
                NewLegacyDev[Index2] = OldLegacyDev[Index2 - 1];
                Index2--;
              }
            }
          } else {
            //
            // If NewValue is not in OldlegacyDev array, we are changing to a disabled item
            // so we should modify DisMap to reflect the change
            //
            Pos = NewValue / 8;
            Bit = 7 - (NewValue % 8);
            DisMap[Pos] = (UINT8) (DisMap[Pos] & (~ (UINT8) (1 << Bit)));
            if (0xFF != OldValue) {
              //
              // Because NewValue is a item that was disabled before
              // so after changing the OldValue should be disabled
              // actually we are doing a swap of enable-disable states of two items
              //
              Pos = OldValue / 8;
              Bit = 7 - (OldValue % 8);
              DisMap[Pos] = (UINT8) (DisMap[Pos] | (UINT8) (1 << Bit));
            }
          }
        }
        //
        // To prevent DISABLE appears in the middle of the list
        // we should perform a re-ordering
        //
        Index3 = Index;
        Index = 0;
        while (Index < Number) {
          if (0xFF != NewLegacyDev[Index]) {
            Index++;
            continue;
          }

          Index2 = Index;
          Index2++;
          while (Index2 < Number) {
            if (0xFF != NewLegacyDev[Index2]) {
              break;
            }

            Index2++;
          }

          if (Index2 < Number) {
            NewLegacyDev[Index]   = NewLegacyDev[Index2];
            NewLegacyDev[Index2]  = 0xFF;
          }

          Index++;
        }

        //
        //  Return correct question value.
        //
        Value->u8 = NewLegacyDev[Index3];
      }
    } else {
      switch (QuestionId) {
      case KEY_VALUE_SAVE_AND_EXIT:
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
        break;

      case KEY_VALUE_NO_SAVE_AND_EXIT:
        //
        // Restore local maintain data.
        //
        DiscardChangeHandler (Private, CurrentFakeNVMap);
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
        break;  

      case FORM_RESET:
        gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
        return EFI_UNSUPPORTED;

      default:
        break;
      }
    }
  }

  //
  // Pass changed uncommitted data back to Form Browser
  //
  HiiSetBrowserData (&gBootMaintFormSetGuid, mBootMaintStorageName, sizeof (BMM_FAKE_NV_DATA), (UINT8 *) CurrentFakeNVMap, NULL);
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
      CurrentFakeNVMap->BootOptionDelMark[Index] = FALSE;
    }
    break;

  case FORM_DRV_DEL_ID:
    ASSERT (DriverOptionMenu.MenuNumber <= (sizeof (CurrentFakeNVMap->DriverOptionDel) / sizeof (CurrentFakeNVMap->DriverOptionDel[0])));
    for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
      CurrentFakeNVMap->DriverOptionDel[Index] = FALSE;
      CurrentFakeNVMap->DriverOptionDelMark[Index] = FALSE;
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
  Initialize the Boot Maintenance Utitliy.


  @retval  EFI_SUCCESS      utility ended successfully
  @retval  others           contain some errors

**/
EFI_STATUS
InitializeBM (
  VOID
  )
{
  BMM_CALLBACK_DATA           *BmmCallbackInfo;
  EFI_STATUS                  Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader;
  UINT32                      Length;
  UINT8                       *Data;

  Status = EFI_SUCCESS;
  BmmCallbackInfo = mBmmCallbackInfo;  

  BmmCallbackInfo->BmmPreviousPageId             = FORM_MAIN_ID;
  BmmCallbackInfo->BmmCurrentPageId              = FORM_MAIN_ID;
  BmmCallbackInfo->FeCurrentState                = FileExplorerStateInActive;
  BmmCallbackInfo->FeDisplayContext              = FileExplorerDisplayUnknown;

  //
  // Reinstall String packages to include more new strings.
  //
  
  //
  // String package size
  //
  Length = ReadUnaligned32 ((UINT32 *) BdsDxeStrings) - sizeof (UINT32);

  //
  // Add the length of the Package List Header and the terminating Package Header 
  //
  Length += sizeof (EFI_HII_PACKAGE_LIST_HEADER) + sizeof (EFI_HII_PACKAGE_HEADER);
  
  //
  // Allocate the storage for the entire Package List
  //
  PackageListHeader = AllocateZeroPool (Length);

  //
  // If the Package List can not be allocated, then return a NULL HII Handle
  //
  if (PackageListHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill in the GUID and Length of the Package List Header
  //
  PackageListHeader->PackageLength = Length;

  //
  // Copy String Data into Package list. 
  //
  Data = (UINT8 *)(PackageListHeader + 1);
  Length = ReadUnaligned32 ((UINT32 *) BdsDxeStrings) - sizeof (UINT32);
  CopyMem (Data, (UINT8 *) BdsDxeStrings + sizeof (UINT32), Length);
  
  //
  // Add End type HII package.
  //
  Data += Length;
  ((EFI_HII_PACKAGE_HEADER *) Data)->Type   = EFI_HII_PACKAGE_END;
  ((EFI_HII_PACKAGE_HEADER *) Data)->Length = sizeof (EFI_HII_PACKAGE_HEADER);

  //
  // Update String package for BM
  //
  CopyGuid (&PackageListHeader->PackageListGuid, &gBootMaintFormSetGuid);
  Status = gHiiDatabase->UpdatePackageList (gHiiDatabase, BmmCallbackInfo->BmmHiiHandle, PackageListHeader);
  
  //
  // Update String package for FE.
  //
  CopyGuid (&PackageListHeader->PackageListGuid, &gFileExploreFormSetGuid);
  Status = gHiiDatabase->UpdatePackageList (gHiiDatabase, BmmCallbackInfo->FeHiiHandle, PackageListHeader);
  
  FreePool (PackageListHeader);

  //
  // Init OpCode Handle and Allocate space for creation of Buffer
  //
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (mStartOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  mEndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (mEndOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mStartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (mStartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  mEndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (mEndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  mEndLabel->Number       = LABEL_END;

  InitializeStringDepository ();

  InitAllMenu (BmmCallbackInfo);

  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &ConsoleInpMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &ConsoleOutMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &ConsoleErrMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &BootOptionMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &DriverOptionMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &TerminalMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &DriverMenu);
  
  InitializeBmmConfig (BmmCallbackInfo);

  //
  // Dispatch BMM main formset and File Explorer formset.
  //
  FormSetDispatcher (BmmCallbackInfo);

  //
  // Clean up.
  //
  CleanUpStringDepository ();
  
  FreeAllMenu ();

Exit:
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  if (mEndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mEndOpCodeHandle);
  }

  return Status;
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
  BOpt_GetLegacyOptions ();
  InitializeListHead (&FsOptionMenu.Head);
  BOpt_FindDrivers ();
  InitializeListHead (&DirectoryMenu.Head);
  InitializeListHead (&ConsoleInpMenu.Head);
  InitializeListHead (&ConsoleOutMenu.Head);
  InitializeListHead (&ConsoleErrMenu.Head);
  InitializeListHead (&TerminalMenu.Head);
  LocateSerialIo ();
  GetAllConsoles ();
}

/**
  Free up all Menu Option list.

**/
VOID
FreeAllMenu (
  VOID
  )
{
  BOpt_FreeMenu (&DirectoryMenu);
  BOpt_FreeMenu (&FsOptionMenu);
  BOpt_FreeMenu (&BootOptionMenu);
  BOpt_FreeMenu (&DriverOptionMenu);
  BOpt_FreeMenu (&DriverMenu);
  BOpt_FreeLegacyOptions ();
  FreeAllConsoles ();
}

/**
  Initialize all the string depositories.

**/
VOID
InitializeStringDepository (
  VOID
  )
{
  STRING_DEPOSITORY *StringDepository;
  StringDepository              = AllocateZeroPool (sizeof (STRING_DEPOSITORY) * STRING_DEPOSITORY_NUMBER);
  FileOptionStrDepository       = StringDepository++;
  ConsoleOptionStrDepository    = StringDepository++;
  BootOptionStrDepository       = StringDepository++;
  BootOptionHelpStrDepository   = StringDepository++;
  DriverOptionStrDepository     = StringDepository++;
  DriverOptionHelpStrDepository = StringDepository++;
  TerminalStrDepository         = StringDepository;
}

/**
  Fetch a usable string node from the string depository and return the string token.

  @param CallbackData       The BMM context data.
  @param StringDepository   The string repository.

  @retval  EFI_STRING_ID           String token.

**/
EFI_STRING_ID
GetStringTokenFromDepository (
  IN   BMM_CALLBACK_DATA     *CallbackData,
  IN   STRING_DEPOSITORY     *StringDepository
  )
{
  STRING_LIST_NODE  *CurrentListNode;
  STRING_LIST_NODE  *NextListNode;

  CurrentListNode = StringDepository->CurrentNode;

  if ((NULL != CurrentListNode) && (NULL != CurrentListNode->Next)) {
    //
    // Fetch one reclaimed node from the list.
    //
    NextListNode = StringDepository->CurrentNode->Next;
  } else {
    //
    // If there is no usable node in the list, update the list.
    //
    NextListNode = AllocateZeroPool (sizeof (STRING_LIST_NODE));
    ASSERT (NextListNode != NULL);
    NextListNode->StringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, L" ", NULL);
    ASSERT (NextListNode->StringToken != 0);

    StringDepository->TotalNodeNumber++;

    if (NULL == CurrentListNode) {
      StringDepository->ListHead = NextListNode;
    } else {
      CurrentListNode->Next = NextListNode;
    }
  }

  StringDepository->CurrentNode = NextListNode;

  return StringDepository->CurrentNode->StringToken;
}

/**
  Reclaim string depositories by moving the current node pointer to list head..

**/
VOID
ReclaimStringDepository (
  VOID
  )
{
  UINTN             DepositoryIndex;
  STRING_DEPOSITORY *StringDepository;

  StringDepository = FileOptionStrDepository;
  for (DepositoryIndex = 0; DepositoryIndex < STRING_DEPOSITORY_NUMBER; DepositoryIndex++) {
    StringDepository->CurrentNode = StringDepository->ListHead;
    StringDepository++;
  }
}

/**
  Release resource for all the string depositories.

**/
VOID
CleanUpStringDepository (
  VOID
  )
{
  UINTN             NodeIndex;
  UINTN             DepositoryIndex;
  STRING_LIST_NODE  *CurrentListNode;
  STRING_LIST_NODE  *NextListNode;
  STRING_DEPOSITORY *StringDepository;

  //
  // Release string list nodes.
  //
  StringDepository = FileOptionStrDepository;
  for (DepositoryIndex = 0; DepositoryIndex < STRING_DEPOSITORY_NUMBER; DepositoryIndex++) {
    CurrentListNode = StringDepository->ListHead;
    for (NodeIndex = 0; NodeIndex < StringDepository->TotalNodeNumber; NodeIndex++) {
      NextListNode = CurrentListNode->Next;
      FreePool (CurrentListNode);
      CurrentListNode = NextListNode;
    }

    StringDepository++;
  }
  //
  // Release string depository.
  //
  FreePool (FileOptionStrDepository);
}

/**
  Start boot maintenance manager

  @retval EFI_SUCCESS If BMM is invoked successfully.
  @return Other value if BMM return unsuccessfully.

**/
EFI_STATUS
BdsStartBootMaint (
  VOID
  )
{
  EFI_STATUS      Status;
  LIST_ENTRY      BdsBootOptionList;

  InitializeListHead (&BdsBootOptionList);

  //
  // Connect all prior to entering the platform setup menu.
  //
  if (!gConnectAllHappened) {
    BdsLibConnectAllDriversToAllControllers ();
    gConnectAllHappened = TRUE;
  }
  //
  // Have chance to enumerate boot device
  //
  BdsLibEnumerateAllBootOption (&BdsBootOptionList);

  //
  // Group the legacy boot options for the same device type
  //
  GroupMultipleLegacyBootOption4SameType ();

  //
  // Init the BMM
  //
  Status = InitializeBM ();

  return Status;
}

/**
  Dispatch BMM formset and FileExplorer formset.


  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS If function complete successfully.
  @return Other value if the Setup Browser process BMM's pages and
           return unsuccessfully.

**/
EFI_STATUS
FormSetDispatcher (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  EFI_STATUS                 Status;
  EFI_BROWSER_ACTION_REQUEST ActionRequest;

  while (TRUE) {
    UpdatePageId (CallbackData, FORM_MAIN_ID);

    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    Status = gFormBrowser2->SendForm (
                             gFormBrowser2,
                             &CallbackData->BmmHiiHandle,
                             1,
                             &gBootMaintFormSetGuid,
                             0,
                             NULL,
                             &ActionRequest
                             );
    if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
      EnableResetRequired ();
    }

    ReclaimStringDepository ();

    //
    // When this Formset returns, check if we are going to explore files.
    //
    if (FileExplorerStateInActive != CallbackData->FeCurrentState) {
      UpdateFileExplorer (CallbackData, 0);

      ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
      Status = gFormBrowser2->SendForm (
                               gFormBrowser2,
                               &CallbackData->FeHiiHandle,
                               1,
                               &gFileExploreFormSetGuid,
                               0,
                               NULL,
                               &ActionRequest
                               );
      if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
        EnableResetRequired ();
      }

      CallbackData->FeCurrentState    = FileExplorerStateInActive;
      CallbackData->FeDisplayContext  = FileExplorerDisplayUnknown;
      ReclaimStringDepository ();
    } else {
      break;
    }
  }

  return Status;
}

/**
  Intall BootMaint and FileExplorer HiiPackages.

**/
EFI_STATUS
InitBMPackage (
  VOID
  )
{
  BMM_CALLBACK_DATA           *BmmCallbackInfo;
  EFI_STATUS                  Status;
  UINT8                       *Ptr;
  
  //
  // Create CallbackData structures for Driver Callback
  //
  BmmCallbackInfo = AllocateZeroPool (sizeof (BMM_CALLBACK_DATA));
  if (BmmCallbackInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Create LoadOption in BmmCallbackInfo for Driver Callback
  //
  Ptr = AllocateZeroPool (sizeof (BM_LOAD_CONTEXT) + sizeof (BM_FILE_CONTEXT) + sizeof (BM_HANDLE_CONTEXT) + sizeof (BM_MENU_ENTRY));
  if (Ptr == NULL) {
    FreePool (BmmCallbackInfo);
    BmmCallbackInfo = NULL;
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Initialize Bmm callback data.
  //
  BmmCallbackInfo->LoadContext = (BM_LOAD_CONTEXT *) Ptr;
  Ptr += sizeof (BM_LOAD_CONTEXT);

  BmmCallbackInfo->FileContext = (BM_FILE_CONTEXT *) Ptr;
  Ptr += sizeof (BM_FILE_CONTEXT);

  BmmCallbackInfo->HandleContext = (BM_HANDLE_CONTEXT *) Ptr;
  Ptr += sizeof (BM_HANDLE_CONTEXT);

  BmmCallbackInfo->MenuEntry      = (BM_MENU_ENTRY *) Ptr;

  BmmCallbackInfo->Signature                     = BMM_CALLBACK_DATA_SIGNATURE;
  BmmCallbackInfo->BmmConfigAccess.ExtractConfig = BootMaintExtractConfig;
  BmmCallbackInfo->BmmConfigAccess.RouteConfig   = BootMaintRouteConfig;
  BmmCallbackInfo->BmmConfigAccess.Callback      = BootMaintCallback;
  BmmCallbackInfo->FeConfigAccess.ExtractConfig  = FakeExtractConfig;
  BmmCallbackInfo->FeConfigAccess.RouteConfig    = FileExplorerRouteConfig;
  BmmCallbackInfo->FeConfigAccess.Callback       = FileExplorerCallback;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &BmmCallbackInfo->BmmDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mBmmHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &BmmCallbackInfo->BmmConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &BmmCallbackInfo->FeDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mFeHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &BmmCallbackInfo->FeConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Post our Boot Maint VFR binary to the HII database.
  //
  BmmCallbackInfo->BmmHiiHandle = HiiAddPackages (
                                    &gBootMaintFormSetGuid,
                                    BmmCallbackInfo->BmmDriverHandle,
                                    BmBin,
                                    BdsDxeStrings,
                                    NULL
                                    );
  ASSERT (BmmCallbackInfo->BmmHiiHandle != NULL);

  //
  // Post our File Explorer VFR binary to the HII database.
  //
  BmmCallbackInfo->FeHiiHandle = HiiAddPackages (
                                   &gFileExploreFormSetGuid,
                                   BmmCallbackInfo->FeDriverHandle,
                                   FEBin,
                                   BdsDxeStrings,
                                   NULL
                                   );
  ASSERT (BmmCallbackInfo->FeHiiHandle != NULL);
  
  mBmmCallbackInfo = BmmCallbackInfo;
  
  return EFI_SUCCESS; 
}

/**
  Remvoe the intalled BootMaint and FileExplorer HiiPackages.

**/
VOID
FreeBMPackage (
  VOID
  )
{
  BMM_CALLBACK_DATA           *BmmCallbackInfo;

  BmmCallbackInfo = mBmmCallbackInfo;

  //
  // Remove our IFR data from HII database
  //
  HiiRemovePackages (BmmCallbackInfo->BmmHiiHandle);
  HiiRemovePackages (BmmCallbackInfo->FeHiiHandle);

  if (BmmCallbackInfo->FeDriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           BmmCallbackInfo->FeDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mFeHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &BmmCallbackInfo->FeConfigAccess,
           NULL
           );
  }

  if (BmmCallbackInfo->BmmDriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           BmmCallbackInfo->BmmDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mBmmHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &BmmCallbackInfo->BmmConfigAccess,
           NULL
           );
  }

  FreePool (BmmCallbackInfo->LoadContext);
  FreePool (BmmCallbackInfo);

  mBmmCallbackInfo = NULL; 
  
  return;
}

