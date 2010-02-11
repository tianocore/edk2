/** @file
This is an example of how a driver might export data to the HII protocol to be
later utilized by the Setup Protocol

Copyright (c) 2004 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "DriverSample.h"

#define DISPLAY_ONLY_MY_ITEM  0x0002

EFI_GUID   mFormSetGuid = FORMSET_GUID;
EFI_GUID   mInventoryGuid = INVENTORY_GUID;

CHAR16     VariableName[] = L"MyIfrNVData";
EFI_HANDLE                      DriverHandle[2] = {NULL, NULL};
DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData = NULL;

HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePath0 = {
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
    // {C153B68D-EBFC-488e-B110-662867745B87}
    //
    { 0xc153b68d, 0xebfc, 0x488e, { 0xb1, 0x10, 0x66, 0x28, 0x67, 0x74, 0x5b, 0x87 } }
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

HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePath1 = {
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
    // {06F37F07-0C48-40e9-8436-0A08A0BB76B0}
    //
    { 0x6f37f07, 0xc48, 0x40e9, { 0x84, 0x36, 0xa, 0x8, 0xa0, 0xbb, 0x76, 0xb0 } }
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

/**
  Encode the password using a simple algorithm.

  @param Password The string to be encoded.
  @param MaxSize  The size of the string.

**/
VOID
EncodePassword (
  IN  CHAR16                      *Password,
  IN  UINTN                       MaxSize
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

  FreePool (Buffer);
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
  UINTN                           PasswordMaxSize;
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
  PasswordMaxSize = sizeof (PrivateData->Configuration.WhatIsThePassword2);
  //
  // Check whether we have any old password set
  //
  for (Index = 0; Index < PasswordMaxSize / sizeof (UINT16); Index++) {
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
  Password = HiiGetString (PrivateData->HiiHandle[0], StringId, NULL);
  if (Password == NULL) {
    return EFI_NOT_READY;
  }
  if (StrSize (Password) > PasswordMaxSize) {
    FreePool (Password);
    return EFI_NOT_READY;
  }

  //
  // Validate old password
  //
  EncodedPassword = AllocateZeroPool (PasswordMaxSize);
  ASSERT (EncodedPassword != NULL);
  StrnCpy (EncodedPassword, Password, StrLen (Password));
  EncodePassword (EncodedPassword, StrLen (EncodedPassword) * sizeof (CHAR16));
  if (CompareMem (EncodedPassword, PrivateData->Configuration.WhatIsThePassword2, StrLen (EncodedPassword) * sizeof (CHAR16)) != 0) {
    //
    // Old password mismatch, return EFI_NOT_READY to prompt for error message
    //
    Status = EFI_NOT_READY;
  } else {
    Status = EFI_SUCCESS;
  }

  FreePool (Password);
  FreePool (EncodedPassword);

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
  CHAR16                          *Password;
  CHAR16                          *TempPassword;
  UINTN                           PasswordSize;
  DRIVER_SAMPLE_CONFIGURATION     *Configuration;
  UINTN                           BufferSize;

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
  PasswordSize = sizeof (PrivateData->Configuration.WhatIsThePassword2);
  ZeroMem (Password, PasswordSize);

  TempPassword = HiiGetString (PrivateData->HiiHandle[0], StringId, NULL);
  if (TempPassword == NULL) {
    return EFI_NOT_READY;
  }
  if (StrSize (TempPassword) > PasswordSize) {
    FreePool (TempPassword);
    return EFI_NOT_READY;
  }
  StrnCpy (Password, TempPassword, StrLen (TempPassword));
  FreePool (TempPassword);

  //
  // Retrive uncommitted data from Browser
  //
  Configuration = AllocateZeroPool (sizeof (DRIVER_SAMPLE_CONFIGURATION));
  ASSERT (Configuration != NULL);
  if (HiiGetBrowserData (&mFormSetGuid, VariableName, sizeof (DRIVER_SAMPLE_CONFIGURATION), (UINT8 *) Configuration)) {
    //
    // Update password's clear text in the screen
    //
    CopyMem (Configuration->PasswordClearText, Password, StrSize (Password));

    //
    // Update uncommitted data of Browser
    //
    HiiSetBrowserData (
       &mFormSetGuid,
       VariableName,
       sizeof (DRIVER_SAMPLE_CONFIGURATION),
       (UINT8 *) Configuration,
       NULL
       );
  }

  //
  // Free Configuration Buffer
  //
  FreePool (Configuration);


  //
  // Set password
  //
  EncodePassword (Password, StrLen (Password) * 2);
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
 Update names of Name/Value storage to current language.

 @param PrivateData   Points to the driver private data.

 @retval EFI_SUCCESS   All names are successfully updated.
 @retval EFI_NOT_FOUND Failed to get Name from HII database.

**/
EFI_STATUS
LoadNameValueNames (
  IN DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData
  )
{
  UINTN      Index;

  //
  // Get Name/Value name string of current language
  //
  for (Index = 0; Index < NAME_VALUE_NAME_NUMBER; Index++) {
    PrivateData->NameValueName[Index] = HiiGetString (
                                         PrivateData->HiiHandle[0],
                                         PrivateData->NameStringId[Index],
                                         NULL
                                         );
    if (PrivateData->NameValueName[Index] == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
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
  EFI_STRING                       ConfigRequest;
  EFI_STRING                       ConfigRequestHdr;
  UINTN                            Size;
  EFI_STRING                       Value;
  UINTN                            ValueStrLen;
  CHAR16                           BackupChar;
  CHAR16                           *StrPointer;


  if (Progress == NULL || Results == NULL || Request == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize the local variables.
  //
  ConfigRequestHdr  = NULL;
  ConfigRequest     = NULL;
  Size              = 0;
  *Progress         = Request;

  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  // Get Buffer Storage data from EFI variable.
  // Try to get the current setting from variable.
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
    return EFI_NOT_FOUND;
  }

  if (Request == NULL) {
    //
    // Request is set to NULL, construct full request string.
    //

    //
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&mFormSetGuid, VariableName, PrivateData->DriverHandle[0]);
    Size = (StrLen (ConfigRequest) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  } else {
    //
    // Check routing data in <ConfigHdr>.
    // Note: if only one Storage is used, then this checking could be skipped.
    //
    if (!HiiIsConfigHdrMatch (Request, &mFormSetGuid, NULL)) {
      return EFI_NOT_FOUND;
    }
    ConfigRequest = Request;

    //
    // Check if requesting Name/Value storage
    //
    if (StrStr (Request, L"OFFSET") == NULL) {
      //
      // Update Name/Value storage Names
      //
      Status = LoadNameValueNames (PrivateData);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      //
      // Allocate memory for <ConfigResp>, e.g. Name0=0x11, Name1=0x1234, Name2="ABCD"
      // <Request>   ::=<ConfigHdr>&Name0&Name1&Name2
      // <ConfigResp>::=<ConfigHdr>&Name0=11&Name1=1234&Name2=0041004200430044
      //
      BufferSize = (StrLen (Request) +
        1 + sizeof (PrivateData->Configuration.NameValueVar0) * 2 +
        1 + sizeof (PrivateData->Configuration.NameValueVar1) * 2 +
        1 + sizeof (PrivateData->Configuration.NameValueVar2) * 2 + 1) * sizeof (CHAR16);
      *Results = AllocateZeroPool (BufferSize);
      ASSERT (*Results != NULL);
      StrCpy (*Results, Request);
      Value = *Results;

      //
      // Append value of NameValueVar0, type is UINT8
      //
      if ((Value = StrStr (*Results, PrivateData->NameValueName[0])) != NULL) {
        Value += StrLen (PrivateData->NameValueName[0]);
        ValueStrLen = ((sizeof (PrivateData->Configuration.NameValueVar0) * 2) + 1);
        CopyMem (Value + ValueStrLen, Value, StrSize (Value));

        BackupChar = Value[ValueStrLen];
        *Value++   = L'=';
        Value += UnicodeValueToString (
                   Value, 
                   PREFIX_ZERO | RADIX_HEX, 
                   PrivateData->Configuration.NameValueVar0, 
                   sizeof (PrivateData->Configuration.NameValueVar0) * 2
                   );
        *Value = BackupChar;
      }

      //
      // Append value of NameValueVar1, type is UINT16
      //
      if ((Value = StrStr (*Results, PrivateData->NameValueName[1])) != NULL) {
        Value += StrLen (PrivateData->NameValueName[1]);
        ValueStrLen = ((sizeof (PrivateData->Configuration.NameValueVar1) * 2) + 1);
        CopyMem (Value + ValueStrLen, Value, StrSize (Value));

        BackupChar = Value[ValueStrLen];
        *Value++   = L'=';
        Value += UnicodeValueToString (
                  Value, 
                  PREFIX_ZERO | RADIX_HEX, 
                  PrivateData->Configuration.NameValueVar1, 
                  sizeof (PrivateData->Configuration.NameValueVar1) * 2
                  );
        *Value = BackupChar;
      }

      //
      // Append value of NameValueVar2, type is CHAR16 *
      //
      if ((Value = StrStr (*Results, PrivateData->NameValueName[2])) != NULL) {
        Value += StrLen (PrivateData->NameValueName[2]);
        ValueStrLen = StrLen (PrivateData->Configuration.NameValueVar2) * 4 + 1;
        CopyMem (Value + ValueStrLen, Value, StrSize (Value));

        *Value++ = L'=';
        //
        // Convert Unicode String to Config String, e.g. "ABCD" => "0041004200430044"
        //
        StrPointer = (CHAR16 *) PrivateData->Configuration.NameValueVar2;
        for (; *StrPointer != L'\0'; StrPointer++) {
          Value += UnicodeValueToString (Value, PREFIX_ZERO | RADIX_HEX, *StrPointer, 4);
        }
      }

      Progress = (EFI_STRING *) Request + StrLen (Request);
      return EFI_SUCCESS;
    }
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = HiiConfigRouting->BlockToConfig (
                                HiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) &PrivateData->Configuration,
                                BufferSize,
                                Results,
                                Progress
                                );

  if (Request == NULL) {
    FreePool (ConfigRequest);
    *Progress = NULL;
  }

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
  CHAR16                           *Value;
  CHAR16                           *StrPtr;
  CHAR16                           TemStr[5];
  UINT8                            *DataBuffer;
  UINT8                            DigitUint8;
  UINTN                            Index;
  CHAR16                           *StrBuffer;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;
  *Progress = Configuration;

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &mFormSetGuid, NULL)) {
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
  // Check if configuring Name/Value storage
  //
  if (StrStr (Configuration, L"OFFSET") == NULL) {
    //
    // Update Name/Value storage Names
    //
    Status = LoadNameValueNames (PrivateData);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Convert value for NameValueVar0
    //
    if ((Value = StrStr (Configuration, PrivateData->NameValueName[0])) != NULL) {
      //
      // Skip "Name="
      //
      Value += StrLen (PrivateData->NameValueName[0]);
      Value++;
      //
      // Get Value String
      //
      StrPtr = StrStr (Value, L"&");
      if (StrPtr == NULL) {
        StrPtr = Value + StrLen (Value);
      }
      //
      // Convert Value to Buffer data
      //
      DataBuffer = (UINT8 *) &PrivateData->Configuration.NameValueVar0;
      ZeroMem (TemStr, sizeof (TemStr));
      for (Index = 0, StrPtr --; StrPtr >= Value; StrPtr --, Index ++) {
        TemStr[0] = *StrPtr;
        DigitUint8 = (UINT8) StrHexToUint64 (TemStr);
        if ((Index & 1) == 0) {
          DataBuffer [Index/2] = DigitUint8;
        } else {
          DataBuffer [Index/2] = (UINT8) ((UINT8) (DigitUint8 << 4) + DataBuffer [Index/2]);
        }
      }
    }

    //
    // Convert value for NameValueVar1
    //
    if ((Value = StrStr (Configuration, PrivateData->NameValueName[1])) != NULL) {
      //
      // Skip "Name="
      //
      Value += StrLen (PrivateData->NameValueName[1]);
      Value++;
      //
      // Get Value String
      //
      StrPtr = StrStr (Value, L"&");
      if (StrPtr == NULL) {
        StrPtr = Value + StrLen (Value);
      }
      //
      // Convert Value to Buffer data
      //
      DataBuffer = (UINT8 *) &PrivateData->Configuration.NameValueVar1;
      ZeroMem (TemStr, sizeof (TemStr));
      for (Index = 0, StrPtr --; StrPtr >= Value; StrPtr --, Index ++) {
        TemStr[0] = *StrPtr;
        DigitUint8 = (UINT8) StrHexToUint64 (TemStr);
        if ((Index & 1) == 0) {
          DataBuffer [Index/2] = DigitUint8;
        } else {
          DataBuffer [Index/2] = (UINT8) ((UINT8) (DigitUint8 << 4) + DataBuffer [Index/2]);
        }
      }
    }

    //
    // Convert value for NameValueVar2
    //
    if ((Value = StrStr (Configuration, PrivateData->NameValueName[2])) != NULL) {
      //
      // Skip "Name="
      //
      Value += StrLen (PrivateData->NameValueName[2]);
      Value++;
      //
      // Get Value String
      //
      StrPtr = StrStr (Value, L"&");
      if (StrPtr == NULL) {
        StrPtr = Value + StrLen (Value);
      }
      //
      // Convert Config String to Unicode String, e.g "0041004200430044" => "ABCD"
      //
      StrBuffer = (CHAR16 *) PrivateData->Configuration.NameValueVar2;
      ZeroMem (TemStr, sizeof (TemStr));
      while (Value < StrPtr) {
        StrnCpy (TemStr, Value, 4);
        *(StrBuffer++) = (CHAR16) StrHexToUint64 (TemStr);
        Value += 4;
      }
      *StrBuffer = L'\0';
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
  UINT8                           MyVar;
  VOID                            *StartOpCodeHandle;
  VOID                            *OptionsOpCodeHandle;
  EFI_IFR_GUID_LABEL              *StartLabel;
  VOID                            *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL              *EndLabel;
  EFI_INPUT_KEY                   Key;
  DRIVER_SAMPLE_CONFIGURATION     *Configuration;
  UINTN                           MyVarSize;

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    //
    // On FORM_OPEN event, update the form on-the-fly
    //
    PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);

    //
    // Initialize the container for dynamic opcodes
    //
    StartOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (StartOpCodeHandle != NULL);

    //
    // Create Hii Extend Label OpCode as the start opcode
    //
    StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
    StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    StartLabel->Number       = LABEL_UPDATE2;

    HiiCreateActionOpCode (
      StartOpCodeHandle,                // Container for dynamic created opcodes
      0x1238,                           // Question ID
      STRING_TOKEN(STR_SAVE_TEXT),      // Prompt text
      STRING_TOKEN(STR_SAVE_TEXT),      // Help text
      EFI_IFR_FLAG_CALLBACK,            // Question flag
      0                                 // Action String ID
    );

    HiiUpdateForm (
      PrivateData->HiiHandle[0],  // HII handle
      &mFormSetGuid,              // Formset GUID
      0x3,                        // Form ID
      StartOpCodeHandle,          // Label for where to insert opcodes
      NULL                        // Insert data
      );

    HiiFreeOpCodeHandle (StartOpCodeHandle);
    return EFI_SUCCESS;
  }

  if (Action == EFI_BROWSER_ACTION_FORM_CLOSE) {
    //
    // On FORM_CLOSE event, show up a pop-up
    //
    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"",
        L"You are going to leave the Form!",
        L"Press ESC or ENTER to continue ...",
        L"",
        NULL
        );
    } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

    return EFI_SUCCESS;
  }

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Type == EFI_IFR_TYPE_STRING) && (Value->string == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);

  switch (QuestionId) {
  case 0x1234:
    //
    // Initialize the container for dynamic opcodes
    //
    StartOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (StartOpCodeHandle != NULL);

    EndOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (EndOpCodeHandle != NULL);

    //
    // Create Hii Extend Label OpCode as the start opcode
    //
    StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
    StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    StartLabel->Number       = LABEL_UPDATE1;

    //
    // Create Hii Extend Label OpCode as the end opcode
    //
    EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
    EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    EndLabel->Number       = LABEL_END;

    HiiCreateActionOpCode (
      StartOpCodeHandle,                // Container for dynamic created opcodes
      0x1237,                           // Question ID
      STRING_TOKEN(STR_EXIT_TEXT),      // Prompt text
      STRING_TOKEN(STR_EXIT_TEXT),      // Help text
      EFI_IFR_FLAG_CALLBACK,            // Question flag
      0                                 // Action String ID
    );

    //
    // Create Option OpCode
    //
    OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (OptionsOpCodeHandle != NULL);

    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      STRING_TOKEN (STR_BOOT_OPTION1),
      0,
      EFI_IFR_NUMERIC_SIZE_1,
      1
      );

    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      STRING_TOKEN (STR_BOOT_OPTION2),
      0,
      EFI_IFR_NUMERIC_SIZE_1,
      2
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

    //
    // Set initial vlaue of dynamic created oneof Question in Form Browser
    //
    Configuration = AllocateZeroPool (sizeof (DRIVER_SAMPLE_CONFIGURATION));
    ASSERT (Configuration != NULL);
    if (HiiGetBrowserData (&mFormSetGuid, VariableName, sizeof (DRIVER_SAMPLE_CONFIGURATION), (UINT8 *) Configuration)) {
      Configuration->DynamicOneof = 2;

      //
      // Update uncommitted data of Browser
      //
      HiiSetBrowserData (
        &mFormSetGuid,
        VariableName,
        sizeof (DRIVER_SAMPLE_CONFIGURATION),
        (UINT8 *) Configuration,
        NULL
        );
    }
    FreePool (Configuration);

    HiiCreateOneOfOpCode (
      StartOpCodeHandle,                         // Container for dynamic created opcodes
      0x8001,                                    // Question ID (or call it "key")
      CONFIGURATION_VARSTORE_ID,                 // VarStore ID
      (UINT16) DYNAMIC_ONE_OF_VAR_OFFSET,        // Offset in Buffer Storage
      STRING_TOKEN (STR_ONE_OF_PROMPT),          // Question prompt text
      STRING_TOKEN (STR_ONE_OF_HELP),            // Question help text
      EFI_IFR_FLAG_CALLBACK,                     // Question flag
      EFI_IFR_NUMERIC_SIZE_1,                    // Data type of Question Value
      OptionsOpCodeHandle,                       // Option Opcode list
      NULL                                       // Default Opcode is NULl
      );

    HiiCreateOrderedListOpCode (
      StartOpCodeHandle,                         // Container for dynamic created opcodes
      0x8002,                                    // Question ID
      CONFIGURATION_VARSTORE_ID,                 // VarStore ID
      (UINT16) DYNAMIC_ORDERED_LIST_VAR_OFFSET,  // Offset in Buffer Storage
      STRING_TOKEN (STR_BOOT_OPTIONS),           // Question prompt text
      STRING_TOKEN (STR_BOOT_OPTIONS),           // Question help text
      EFI_IFR_FLAG_RESET_REQUIRED,               // Question flag
      0,                                         // Ordered list flag, e.g. EFI_IFR_UNIQUE_SET
      EFI_IFR_NUMERIC_SIZE_1,                    // Data type of Question value
      5,                                         // Maximum container
      OptionsOpCodeHandle,                       // Option Opcode list
      NULL                                       // Default Opcode is NULl
      );

    HiiCreateGotoOpCode (
      StartOpCodeHandle,                // Container for dynamic created opcodes
      1,                                // Target Form ID
      STRING_TOKEN (STR_GOTO_FORM1),    // Prompt text
      STRING_TOKEN (STR_GOTO_HELP),     // Help text
      0,                                // Question flag
      0x8003                            // Question ID
      );

    HiiUpdateForm (
      PrivateData->HiiHandle[0],  // HII handle
      &mFormSetGuid,              // Formset GUID
      0x1234,                     // Form ID
      StartOpCodeHandle,          // Label for where to insert opcodes
      EndOpCodeHandle             // Replace data
      );

    HiiFreeOpCodeHandle (StartOpCodeHandle);
    HiiFreeOpCodeHandle (OptionsOpCodeHandle);
    HiiFreeOpCodeHandle (EndOpCodeHandle);
    break;

  case 0x5678:
    //
    // We will reach here once the Question is refreshed
    //

    //
    // Initialize the container for dynamic opcodes
    //
    StartOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (StartOpCodeHandle != NULL);

    //
    // Create Hii Extend Label OpCode as the start opcode
    //
    StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
    StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    StartLabel->Number       = LABEL_UPDATE2;

    HiiCreateActionOpCode (
      StartOpCodeHandle,                // Container for dynamic created opcodes
      0x1237,                           // Question ID
      STRING_TOKEN(STR_EXIT_TEXT),      // Prompt text
      STRING_TOKEN(STR_EXIT_TEXT),      // Help text
      EFI_IFR_FLAG_CALLBACK,            // Question flag
      0                                 // Action String ID
    );

    HiiUpdateForm (
      PrivateData->HiiHandle[0],  // HII handle
      &mFormSetGuid,              // Formset GUID
      0x3,                        // Form ID
      StartOpCodeHandle,          // Label for where to insert opcodes
      NULL                        // Insert data
      );

    HiiFreeOpCodeHandle (StartOpCodeHandle);

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
    MyVarSize = 1;
    MyVar = 111;
    Status = gRT->SetVariable(
                    L"MyVar",
                    &mFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    MyVarSize,
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

  case 0x1111:
    //
    // EfiVarstore question takes sample action (print value as debug information) 
    // after read/write question.
    //
    MyVarSize = 1;
    Status = gRT->GetVariable(
                    L"MyVar",
                    &mFormSetGuid,
                    NULL,
                    &MyVarSize,
                    &MyVar
                    );
    ASSERT_EFI_ERROR (Status);
    DEBUG ((DEBUG_INFO, "EfiVarstore question: Tall value is %d with value width %d\n", MyVar, MyVarSize));
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
  EFI_HII_HANDLE                  HiiHandle[2];
  EFI_SCREEN_DESCRIPTOR           Screen;
  EFI_HII_DATABASE_PROTOCOL       *HiiDatabase;
  EFI_HII_STRING_PROTOCOL         *HiiString;
  EFI_FORM_BROWSER2_PROTOCOL      *FormBrowser2;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
  CHAR16                          *NewString;
  UINTN                           BufferSize;
  DRIVER_SAMPLE_CONFIGURATION     *Configuration;
  BOOLEAN                         ActionFlag;
  EFI_STRING                      ConfigRequestHdr;

  //
  // Initialize the local variables.
  //
  ConfigRequestHdr = NULL;
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
  PrivateData = AllocateZeroPool (sizeof (DRIVER_SAMPLE_PRIVATE_DATA));
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

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DriverHandle[0],
                  &gEfiDevicePathProtocolGuid,
                  &mHiiVendorDevicePath0,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &PrivateData->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  PrivateData->DriverHandle[0] = DriverHandle[0];

  //
  // Publish our HII data
  //
  HiiHandle[0] = HiiAddPackages (
                   &mFormSetGuid,
                   DriverHandle[0],
                   DriverSampleStrings,
                   VfrBin,
                   NULL
                   );
  if (HiiHandle[0] == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->HiiHandle[0] = HiiHandle[0];

  //
  // Publish another Fromset
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DriverHandle[1],
                  &gEfiDevicePathProtocolGuid,
                  &mHiiVendorDevicePath1,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  PrivateData->DriverHandle[1] = DriverHandle[1];

  HiiHandle[1] = HiiAddPackages (
                   &mInventoryGuid,
                   DriverHandle[1],
                   DriverSampleStrings,
                   InventoryBin,
                   NULL
                   );
  if (HiiHandle[1] == NULL) {
    DriverSampleUnload (ImageHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->HiiHandle[1] = HiiHandle[1];

  //
  // Very simple example of how one would update a string that is already
  // in the HII database
  //
  NewString = L"700 Mhz";

  if (HiiSetString (HiiHandle[0], STRING_TOKEN (STR_CPU_STRING2), NewString, NULL) == 0) {
    DriverSampleUnload (ImageHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  HiiSetString (HiiHandle[0], 0, NewString, NULL);

  //
  // Initialize Name/Value name String ID
  //
  PrivateData->NameStringId[0] = STR_NAME_VALUE_VAR_NAME0;
  PrivateData->NameStringId[1] = STR_NAME_VALUE_VAR_NAME1;
  PrivateData->NameStringId[2] = STR_NAME_VALUE_VAR_NAME2;

  //
  // Initialize configuration data
  //
  Configuration = &PrivateData->Configuration;
  ZeroMem (Configuration, sizeof (DRIVER_SAMPLE_CONFIGURATION));

  //
  // Try to read NV config EFI variable first
  //
  ConfigRequestHdr = HiiConstructConfigHdr (&mFormSetGuid, VariableName, DriverHandle[0]);
  ASSERT (ConfigRequestHdr != NULL);

  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (VariableName, &mFormSetGuid, NULL, &BufferSize, Configuration);
  if (EFI_ERROR (Status)) {
    //
    // Store zero data Buffer Storage to EFI variable
    //
    Status = gRT->SetVariable(
                    VariableName,
                    &mFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (DRIVER_SAMPLE_CONFIGURATION),
                    Configuration
                    );
    ASSERT (Status == EFI_SUCCESS);
    //
    // EFI variable for NV config doesn't exit, we should build this variable
    // based on default values stored in IFR
    //
    ActionFlag = HiiSetToDefaults (ConfigRequestHdr, EFI_HII_DEFAULT_CLASS_STANDARD);
    ASSERT (ActionFlag);
  } else {
    //
    // EFI variable does exist and Validate Current Setting
    //
    ActionFlag = HiiValidateSettings (ConfigRequestHdr);
    ASSERT (ActionFlag);
  }

  FreePool (ConfigRequestHdr);


  //
  // In default, this driver is built into Flash device image,
  // the following code doesn't run.
  //

  //
  // Example of how to display only the item we sent to HII
  // When this driver is not built into Flash device image,
  // it need to call SendForm to show front page by itself.
  //
  if (DISPLAY_ONLY_MY_ITEM <= 1) {
    //
    // Have the browser pull out our copy of the data, and only display our data
    //
    Status = FormBrowser2->SendForm (
                             FormBrowser2,
                             &(HiiHandle[DISPLAY_ONLY_MY_ITEM]),
                             1,
                             NULL,
                             0,
                             NULL,
                             NULL
                             );

    HiiRemovePackages (HiiHandle[0]);

    HiiRemovePackages (HiiHandle[1]);
  }

  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
DriverSampleUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  UINTN Index;
  if (DriverHandle[0] != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
            DriverHandle[0],
            &gEfiDevicePathProtocolGuid,
            &mHiiVendorDevicePath0,
            &gEfiHiiConfigAccessProtocolGuid,
            &PrivateData->ConfigAccess,
            NULL
           );
    DriverHandle[0] = NULL;
  }

  if (DriverHandle[1] != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
            DriverHandle[1],
            &gEfiDevicePathProtocolGuid,
            &mHiiVendorDevicePath1,
            NULL
           );
    DriverHandle[1] = NULL;
  }

  if (PrivateData->HiiHandle[0] != NULL) {
    HiiRemovePackages (PrivateData->HiiHandle[0]);
  }

  if (PrivateData->HiiHandle[1] != NULL) {
    HiiRemovePackages (PrivateData->HiiHandle[1]);
  }

  if (PrivateData != NULL) {
    for (Index = 0; Index < NAME_VALUE_NAME_NUMBER; Index++) {
      if (PrivateData->NameValueName[Index] != NULL) {
        FreePool (PrivateData->NameValueName[Index]);
      }
    }
    FreePool (PrivateData);
    PrivateData = NULL;
  }

  return EFI_SUCCESS;
}
