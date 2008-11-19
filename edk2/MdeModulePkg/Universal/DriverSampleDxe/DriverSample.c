/** @file
Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  DriverSample.c

Abstract:

  This is an example of how a driver might export data to the HII protocol to be
  later utilized by the Setup Protocol


**/


#include "DriverSample.h"

#define DISPLAY_ONLY_MY_ITEM  0x0002

EFI_GUID   mFormSetGuid = FORMSET_GUID;
EFI_GUID   mInventoryGuid = INVENTORY_GUID;

CHAR16     VariableName[] = L"MyIfrNVData";

/**
  Encode the password using a simple algorithm.
  
  @param Password The string to be encoded.
  @param MaxSize  The size of the string.
  
**/
VOID
EncodePassword (
  IN  CHAR16                      *Password,
  IN  UINT8                       MaxSize
  )
{
  UINTN   Index;
  UINTN   Loop;
  CHAR16  *Buffer;
  CHAR16  *Key;

  Key     = L"MAR10648567";
  Buffer  = AllocateZeroPool (MaxSize);
  ASSERT (Buffer != NULL);

  for (Index = 0; Key[Index] != 0; Index++) {
    for (Loop = 0; Loop < (UINT8) (MaxSize / 2); Loop++) {
      Buffer[Loop] = (CHAR16) (Password[Loop] ^ Key[Index]);
    }
  }

  CopyMem (Password, Buffer, MaxSize);

  gBS->FreePool (Buffer);
  return ;
}

/**
  Validate the user's password.
  
  @param PrivateData This driver's private context data.
  @param StringId    The user's input.
  
  @retval EFI_SUCCESS   The user's input matches the password.
  @retval EFI_NOT_READY The user's input does not match the password.
**/
EFI_STATUS
ValidatePassword (
  IN       DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData,
  IN       EFI_STRING_ID                   StringId
  )
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  UINTN                           BufferSize;
  CHAR16                          *Password;
  CHAR16                          *EncodedPassword;
  BOOLEAN                         OldPassword;

  //
  // Get encoded password first
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (
                  VariableName,
                  &mFormSetGuid,
                  NULL,
                  &BufferSize,
                  &PrivateData->Configuration
                  );
  if (EFI_ERROR (Status)) {
    //
    // Old password not exist, prompt for new password
    //
    return EFI_SUCCESS;
  }

  OldPassword = FALSE;
  //
  // Check whether we have any old password set
  //
  for (Index = 0; Index < 20; Index++) {
    if (PrivateData->Configuration.WhatIsThePassword2[Index] != 0) {
      OldPassword = TRUE;
      break;
    }
  }
  if (!OldPassword) {
    //
    // Old password not exist, return EFI_SUCCESS to prompt for new password
    //
    return EFI_SUCCESS;
  }

  //
  // Get user input password
  //
  BufferSize = 21 * sizeof (CHAR16);
  Password = AllocateZeroPool (BufferSize);
  ASSERT (Password != NULL);

  Status = HiiLibGetString (PrivateData->HiiHandle[0], StringId, Password, &BufferSize);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (Password);
    return Status;
  }

  //
  // Validate old password
  //
  EncodedPassword = AllocateCopyPool (21 * sizeof (CHAR16), Password);
  ASSERT (EncodedPassword != NULL);
  EncodePassword (EncodedPassword, 20 * sizeof (CHAR16));
  if (CompareMem (EncodedPassword, PrivateData->Configuration.WhatIsThePassword2, 20 * sizeof (CHAR16)) != 0) {
    //
    // Old password mismatch, return EFI_NOT_READY to prompt for error message
    //
    Status = EFI_NOT_READY;
  } else {
    Status = EFI_SUCCESS;
  }

  gBS->FreePool (Password);
  gBS->FreePool (EncodedPassword);

  return Status;
}

/**
  Encode the password using a simple algorithm.
  
  @param PrivateData This driver's private context data.
  @param StringId    The password from User.
  
  @retval  EFI_SUCESS The operation is successful.
  @return  Other value if gRT->SetVariable () fails.
  
**/
EFI_STATUS
SetPassword (
  IN DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData,
  IN EFI_STRING_ID                   StringId
  )
{
  EFI_STATUS                      Status;
  UINTN                           BufferSize;
  CHAR16                          *Password;
  DRIVER_SAMPLE_CONFIGURATION     *Configuration;

  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (
                  VariableName,
                  &mFormSetGuid,
                  NULL,
                  &BufferSize,
                  &PrivateData->Configuration
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get user input password
  //
  Password = &PrivateData->Configuration.WhatIsThePassword2[0];
  ZeroMem (Password, 20 * sizeof (CHAR16));
  Status = HiiLibGetString (PrivateData->HiiHandle[0], StringId, Password, &BufferSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Retrive uncommitted data from Browser
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Configuration = AllocateZeroPool (sizeof (DRIVER_SAMPLE_PRIVATE_DATA));
  ASSERT (Configuration != NULL);
  Status = GetBrowserData (&mFormSetGuid, VariableName, &BufferSize, (UINT8 *) Configuration);
  if (!EFI_ERROR (Status)) {
    //
    // Update password's clear text in the screen
    //
    CopyMem (Configuration->PasswordClearText, Password, 20 * sizeof (CHAR16));

    //
    // Update uncommitted data of Browser
    //
    BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
    Status = SetBrowserData (
               &mFormSetGuid,
               VariableName,
               BufferSize,
               (UINT8 *) Configuration,
               NULL
               );
  }
  gBS->FreePool (Configuration);

  //
  // Set password
  //
  EncodePassword (Password, 20 * sizeof (CHAR16));
  Status = gRT->SetVariable(
                  VariableName,
                  &mFormSetGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (DRIVER_SAMPLE_CONFIGURATION),
                  &PrivateData->Configuration
                  );
  return Status;
}


/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request                A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param  Results                A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
ExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  DRIVER_SAMPLE_PRIVATE_DATA       *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (
                  VariableName,
                  &mFormSetGuid,
                  NULL,
                  &BufferSize,
                  &PrivateData->Configuration
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Request == NULL) {
    //
    // Request is set to NULL, return all configurable elements together with ALTCFG
    //
    Status = ConstructConfigAltResp (
               NULL,
               NULL,
               Results,
               &mFormSetGuid,
               VariableName,
               PrivateData->DriverHandle[0],
               &PrivateData->Configuration,
               BufferSize,
               VfrMyIfrNVDataBlockName,
               2,
               STRING_TOKEN (STR_STANDARD_DEFAULT_PROMPT),
               VfrMyIfrNVDataDefault0000,
               STRING_TOKEN (STR_MANUFACTURE_DEFAULT_PROMPT),
               VfrMyIfrNVDataDefault0001
               );

    return Status;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!IsConfigHdrMatch (Request, &mFormSetGuid, VariableName)) {
    *Progress = Request;
    return EFI_NOT_FOUND;
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = HiiConfigRouting->BlockToConfig (
                                HiiConfigRouting,
                                Request,
                                (UINT8 *) &PrivateData->Configuration,
                                BufferSize,
                                Results,
                                Progress
                                );
  return Status;
}


/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration          A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
RouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  DRIVER_SAMPLE_PRIVATE_DATA       *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!IsConfigHdrMatch (Configuration, &mFormSetGuid, VariableName)) {
    *Progress = Configuration;
    return EFI_NOT_FOUND;
  }

  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (
                  VariableName,
                  &mFormSetGuid,
                  NULL,
                  &BufferSize,
                  &PrivateData->Configuration
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = HiiConfigRouting->ConfigToBlock (
                               HiiConfigRouting,
                               Configuration,
                               (UINT8 *) &PrivateData->Configuration,
                               &BufferSize,
                               Progress
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Store Buffer Storage back to EFI variable
  //
  Status = gRT->SetVariable(
                  VariableName,
                  &mFormSetGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (DRIVER_SAMPLE_CONFIGURATION),
                  &PrivateData->Configuration
                  );

  return Status;
}


/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
DriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData;
  EFI_STATUS                      Status;
  EFI_HII_UPDATE_DATA             UpdateData;
  IFR_OPTION                      *IfrOptionList;
  UINT8                           MyVar;

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);

  switch (QuestionId) {
  case 0x1234:
    //
    // Initialize the container for dynamic opcodes
    //
    IfrLibInitUpdateData (&UpdateData, 0x1000);

    IfrOptionList = AllocatePool (2 * sizeof (IFR_OPTION));
    ASSERT (IfrOptionList != NULL);

    IfrOptionList[0].Flags        = 0;
    IfrOptionList[0].StringToken  = STRING_TOKEN (STR_BOOT_OPTION1);
    IfrOptionList[0].Value.u8     = 1;
    IfrOptionList[1].Flags        = EFI_IFR_OPTION_DEFAULT;
    IfrOptionList[1].StringToken  = STRING_TOKEN (STR_BOOT_OPTION2);
    IfrOptionList[1].Value.u8     = 2;

      CreateActionOpCode (
        0x1237,                           // Question ID
        STRING_TOKEN(STR_EXIT_TEXT),      // Prompt text
        STRING_TOKEN(STR_EXIT_TEXT),      // Help text
        EFI_IFR_FLAG_CALLBACK,            // Question flag
        0,                                // Action String ID
        &UpdateData                       // Container for dynamic created opcodes
        );
    
      //
      // Prepare initial value for the dynamic created oneof Question
      //
      PrivateData->Configuration.DynamicOneof = 2;
      Status = gRT->SetVariable(
                      VariableName,
                      &mFormSetGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                      sizeof (DRIVER_SAMPLE_CONFIGURATION),
                      &PrivateData->Configuration
                      );
      CreateOneOfOpCode (
        0x8001,                           // Question ID (or call it "key")
        CONFIGURATION_VARSTORE_ID,        // VarStore ID
        DYNAMIC_ONE_OF_VAR_OFFSET,        // Offset in Buffer Storage
        STRING_TOKEN (STR_ONE_OF_PROMPT), // Question prompt text
        STRING_TOKEN (STR_ONE_OF_HELP),   // Question help text
        EFI_IFR_FLAG_CALLBACK,            // Question flag
        EFI_IFR_NUMERIC_SIZE_1,           // Data type of Question Value
        IfrOptionList,                    // Option list
        2,                                // Number of options in Option list
        &UpdateData                       // Container for dynamic created opcodes
        );
    
      CreateOrderedListOpCode (
        0x8002,                           // Question ID
        CONFIGURATION_VARSTORE_ID,        // VarStore ID
        DYNAMIC_ORDERED_LIST_VAR_OFFSET,  // Offset in Buffer Storage
        STRING_TOKEN (STR_BOOT_OPTIONS),  // Question prompt text
        STRING_TOKEN (STR_BOOT_OPTIONS),  // Question help text
        EFI_IFR_FLAG_RESET_REQUIRED,      // Question flag
        0,                                // Ordered list flag, e.g. EFI_IFR_UNIQUE_SET
        EFI_IFR_NUMERIC_SIZE_1,           // Data type of Question value
        5,                                // Maximum container
        IfrOptionList,                    // Option list
        2,                                // Number of options in Option list
        &UpdateData                       // Container for dynamic created opcodes
        );
    
      CreateGotoOpCode (
        1,                                // Target Form ID
        STRING_TOKEN (STR_GOTO_FORM1),    // Prompt text
        STRING_TOKEN (STR_GOTO_HELP),     // Help text
        0,                                // Question flag
        0x8003,                           // Question ID
        &UpdateData                       // Container for dynamic created opcodes
        );
    
      Status = IfrLibUpdateForm (
                 PrivateData->HiiHandle[0],  // HII handle
                 &mFormSetGuid,              // Formset GUID
                 0x1234,                     // Form ID
                 0x1234,                     // Label for where to insert opcodes
                 TRUE,                       // Append or replace
                 &UpdateData                 // Dynamic created opcodes
                 );
      gBS->FreePool (IfrOptionList);
      IfrLibFreeUpdateData (&UpdateData);
      break;
    
    case 0x5678:
      //
      // We will reach here once the Question is refreshed
      //
      IfrLibInitUpdateData (&UpdateData, 0x1000);
    
      IfrOptionList = AllocatePool (2 * sizeof (IFR_OPTION));
      ASSERT (IfrOptionList != NULL);
    
      CreateActionOpCode (
        0x1237,                           // Question ID
        STRING_TOKEN(STR_EXIT_TEXT),      // Prompt text
        STRING_TOKEN(STR_EXIT_TEXT),      // Help text
        EFI_IFR_FLAG_CALLBACK,            // Question flag
        0,                                // Action String ID
        &UpdateData                       // Container for dynamic created opcodes
        );
    
      Status = IfrLibUpdateForm (
                 PrivateData->HiiHandle[0],  // HII handle
                 &mFormSetGuid,              // Formset GUID
                 3,                          // Form ID
                 0x2234,                     // Label for where to insert opcodes
                 TRUE,                       // Append or replace
                 &UpdateData                 // Dynamic created opcodes
                 );
      IfrLibFreeUpdateData (&UpdateData);
    
      //
      // Refresh the Question value
      //
      PrivateData->Configuration.DynamicRefresh++;
      Status = gRT->SetVariable(
                      VariableName,
                      &mFormSetGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                      sizeof (DRIVER_SAMPLE_CONFIGURATION),
                      &PrivateData->Configuration
                      );
    
      //
      // Change an EFI Variable storage (MyEfiVar) asynchronous, this will cause
      // the first statement in Form 3 be suppressed
      //
      MyVar = 111;
      Status = gRT->SetVariable(
                      L"MyVar",
                      &mFormSetGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                      1,
                      &MyVar
                      );
    break;

  case 0x1237:
    //
    // User press "Exit now", request Browser to exit
    //
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
    break;

  case 0x1238:
    //
    // User press "Save now", request Browser to save the uncommitted data.
    //
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
    break;

  case 0x2000:
    //
    // When try to set a new password, user will be chanlleged with old password.
    // The Callback is responsible for validating old password input by user,
    // If Callback return EFI_SUCCESS, it indicates validation pass.
    //
    switch (PrivateData->PasswordState) {
    case BROWSER_STATE_VALIDATE_PASSWORD:
      Status = ValidatePassword (PrivateData, Value->string);
      if (Status == EFI_SUCCESS) {
        PrivateData->PasswordState = BROWSER_STATE_SET_PASSWORD;
      }
      break;

    case BROWSER_STATE_SET_PASSWORD:
      Status = SetPassword (PrivateData, Value->string);
      PrivateData->PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
      break;

    default:
      break;
    }

    break;

  default:
    break;
  }

  return Status;
}

/**
  Main entry for this driver.
  
  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
DriverSampleInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_STATUS                      SavedStatus;
  EFI_HII_PACKAGE_LIST_HEADER     *PackageList;
  EFI_HII_HANDLE                  HiiHandle[2];
  EFI_HANDLE                      DriverHandle[2];
  DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData;
  EFI_SCREEN_DESCRIPTOR           Screen;
  EFI_HII_DATABASE_PROTOCOL       *HiiDatabase;
  EFI_HII_STRING_PROTOCOL         *HiiString;
  EFI_FORM_BROWSER2_PROTOCOL      *FormBrowser2;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
  CHAR16                          *NewString;
  UINTN                           BufferSize;
  DRIVER_SAMPLE_CONFIGURATION     *Configuration;
  BOOLEAN                         ExtractIfrDefault;

  //
  // Initialize the library and our protocol.
  //

  //
  // Initialize screen dimensions for SendForm().
  // Remove 3 characters from top and bottom
  //
  ZeroMem (&Screen, sizeof (EFI_SCREEN_DESCRIPTOR));
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &Screen.RightColumn, &Screen.BottomRow);

  Screen.TopRow     = 3;
  Screen.BottomRow  = Screen.BottomRow - 3;

  //
  // Initialize driver private data
  //
  PrivateData = AllocatePool (sizeof (DRIVER_SAMPLE_PRIVATE_DATA));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->Signature = DRIVER_SAMPLE_PRIVATE_SIGNATURE;

  PrivateData->ConfigAccess.ExtractConfig = ExtractConfig;
  PrivateData->ConfigAccess.RouteConfig = RouteConfig;
  PrivateData->ConfigAccess.Callback = DriverCallback;
  PrivateData->PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;

  //
  // Locate Hii Database protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **) &HiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiDatabase = HiiDatabase;

  //
  // Locate HiiString protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, (VOID **) &HiiString);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiString = HiiString;

  //
  // Locate Formbrowser2 protocol
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &FormBrowser2);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->FormBrowser2 = FormBrowser2;

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **) &HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiConfigRouting = HiiConfigRouting;

  //
  // Install Config Access protocol
  //
  Status = HiiLibCreateHiiDriverHandle (&DriverHandle[0]);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->DriverHandle[0] = DriverHandle[0];

  Status = gBS->InstallProtocolInterface (
                  &DriverHandle[0],
                  &gEfiHiiConfigAccessProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &PrivateData->ConfigAccess
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  PackageList = HiiLibPreparePackageList (
                  2,
                  &mFormSetGuid,
                  DriverSampleStrings,
                  VfrBin
                  );
  if (PackageList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HiiDatabase->NewPackageList (
                          HiiDatabase,
                          PackageList,
                          DriverHandle[0],
                          &HiiHandle[0]
                          );
  gBS->FreePool (PackageList);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiHandle[0] = HiiHandle[0];

  //
  // Publish another Fromset
  //
  Status = HiiLibCreateHiiDriverHandle (&DriverHandle[1]);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->DriverHandle[1] = DriverHandle[1];

  PackageList = HiiLibPreparePackageList (
                  2,
                  &mInventoryGuid,
                  DriverSampleStrings,
                  InventoryBin
                  );
  if (PackageList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HiiDatabase->NewPackageList (
                          HiiDatabase,
                          PackageList,
                          DriverHandle[1],
                          &HiiHandle[1]
                          );
  gBS->FreePool (PackageList);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiHandle[1] = HiiHandle[1];

  //
  // Very simple example of how one would update a string that is already
  // in the HII database
  //
  NewString = L"700 Mhz";

  Status = HiiLibSetString (HiiHandle[0], STRING_TOKEN (STR_CPU_STRING2), NewString);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize configuration data
  //
  Configuration = &PrivateData->Configuration;
  ZeroMem (Configuration, sizeof (DRIVER_SAMPLE_CONFIGURATION));

  //
  // Try to read NV config EFI variable first
  //
  ExtractIfrDefault = TRUE;
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (VariableName, &mFormSetGuid, NULL, &BufferSize, Configuration);
  if (!EFI_ERROR (Status) && (BufferSize == sizeof (DRIVER_SAMPLE_CONFIGURATION))) {
    ExtractIfrDefault = FALSE;
  }

  if (ExtractIfrDefault) {
    //
    // EFI variable for NV config doesn't exit, we should build this variable
    // based on default values stored in IFR
    //
    BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
    Status = IfrLibExtractDefault (Configuration, &BufferSize, 1, VfrMyIfrNVDataDefault0000);

    if (!EFI_ERROR (Status)) {
      gRT->SetVariable(
             VariableName,
             &mFormSetGuid,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
             sizeof (DRIVER_SAMPLE_CONFIGURATION),
             Configuration
             );
    }
  }

  //
  // Example of how to display only the item we sent to HII
  //
  if (DISPLAY_ONLY_MY_ITEM == 0x0001) {
    //
    // Have the browser pull out our copy of the data, and only display our data
    //
    //    Status = FormConfig->SendForm (FormConfig, TRUE, HiiHandle, NULL, NULL, NULL, &Screen, NULL);
    //
    Status = FormBrowser2->SendForm (
                             FormBrowser2,
                             HiiHandle,
                             1,
                             NULL,
                             0,
                             NULL,
                             NULL
                             );
    SavedStatus = Status;

    Status = HiiDatabase->RemovePackageList (HiiDatabase, HiiHandle[0]);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = HiiDatabase->RemovePackageList (HiiDatabase, HiiHandle[1]);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    return SavedStatus;
  } else {
    //
    // Have the browser pull out all the data in the HII Database and display it.
    //
    //    Status = FormConfig->SendForm (FormConfig, TRUE, 0, NULL, NULL, NULL, NULL, NULL);
    //
  }

  return EFI_SUCCESS;
}
