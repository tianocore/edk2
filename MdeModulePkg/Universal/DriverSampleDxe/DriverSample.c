/** @file
This is an example of how a driver might export data to the HII protocol to be
later utilized by the Setup Protocol

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "DriverSample.h"

#define DISPLAY_ONLY_MY_ITEM  0x0002

CHAR16     VariableName[] = L"MyIfrNVData";
CHAR16     MyEfiVar[] = L"MyEfiVar";
EFI_HANDLE                      DriverHandle[2] = {NULL, NULL};
DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData = NULL;
EFI_EVENT                       mEvent;

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
    DRIVER_SAMPLE_FORMSET_GUID
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
    DRIVER_SAMPLE_INVENTORY_GUID
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
  Add empty function for event process function.

  @param Event    The Event need to be process
  @param Context  The context of the event.

**/
VOID
EFIAPI
DriverSampleInternalEmptyFunction (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
}

/**
  Notification function for keystrokes.

  @param[in] KeyData    The key that was pressed.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
NotificationFunction(
  IN EFI_KEY_DATA *KeyData
  )
{
  gBS->SignalEvent (mEvent);
  
  return EFI_SUCCESS;
}

/**
  Function to start monitoring for CTRL-C using SimpleTextInputEx. 

  @retval EFI_SUCCESS           The feature is enabled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough mnemory available.
**/
EFI_STATUS
EFIAPI
InternalStartMonitor(
  VOID
  )
{
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *SimpleEx;
  EFI_KEY_DATA                      KeyData;
  EFI_STATUS                        Status;
  EFI_HANDLE                        *Handles;
  UINTN                             HandleCount;
  UINTN                             HandleIndex;
  EFI_HANDLE                        NotifyHandle;

  Status = gBS->LocateHandleBuffer (
              ByProtocol,
              &gEfiSimpleTextInputExProtocolGuid,
              NULL,
              &HandleCount,
              &Handles
              );
  for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
    Status = gBS->HandleProtocol (Handles[HandleIndex], &gEfiSimpleTextInputExProtocolGuid, (VOID **) &SimpleEx);
    ASSERT_EFI_ERROR (Status);

    KeyData.KeyState.KeyToggleState = 0;
    KeyData.Key.ScanCode            = 0;
    KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
    KeyData.Key.UnicodeChar         = L'c';

    Status = SimpleEx->RegisterKeyNotify(
      SimpleEx,
      &KeyData,
      NotificationFunction,
      &NotifyHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
    
    KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
    Status = SimpleEx->RegisterKeyNotify(
      SimpleEx,
      &KeyData,
      NotificationFunction,
      &NotifyHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  return EFI_SUCCESS;
}

/**
  Function to stop monitoring for CTRL-C using SimpleTextInputEx.  

  @retval EFI_SUCCESS           The feature is enabled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough mnemory available.
**/
EFI_STATUS
EFIAPI
InternalStopMonitor(
  VOID
  )
{
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *SimpleEx;
  EFI_STATUS                        Status;
  EFI_HANDLE                        *Handles;
  EFI_KEY_DATA                      KeyData;  
  UINTN                             HandleCount;
  UINTN                             HandleIndex;
  EFI_HANDLE                        NotifyHandle;

  Status = gBS->LocateHandleBuffer (
                ByProtocol,
                &gEfiSimpleTextInputExProtocolGuid,
                NULL,
                &HandleCount,
                &Handles
                );
  for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
    Status = gBS->HandleProtocol (Handles[HandleIndex], &gEfiSimpleTextInputExProtocolGuid, (VOID **) &SimpleEx);
    ASSERT_EFI_ERROR (Status);

    KeyData.KeyState.KeyToggleState = 0;
    KeyData.Key.ScanCode            = 0;
    KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
    KeyData.Key.UnicodeChar         = L'c';

    Status = SimpleEx->RegisterKeyNotify(
      SimpleEx,
      &KeyData,
      NotificationFunction,
      &NotifyHandle);
    if (!EFI_ERROR (Status)) {
      Status = SimpleEx->UnregisterKeyNotify (SimpleEx, NotifyHandle);
    }

    KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
    Status = SimpleEx->RegisterKeyNotify(
      SimpleEx,
      &KeyData,
      NotificationFunction,
      &NotifyHandle);
    if (!EFI_ERROR (Status)) {
      Status = SimpleEx->UnregisterKeyNotify (SimpleEx, NotifyHandle);
    }
  }
  return EFI_SUCCESS;
}


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
                  &gDriverSampleFormSetGuid,
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
  if (CompareMem (EncodedPassword, PrivateData->Configuration.WhatIsThePassword2, PasswordMaxSize) != 0) {
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
                  &gDriverSampleFormSetGuid,
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
  if (HiiGetBrowserData (&gDriverSampleFormSetGuid, VariableName, sizeof (DRIVER_SAMPLE_CONFIGURATION), (UINT8 *) Configuration)) {
    //
    // Update password's clear text in the screen
    //
    CopyMem (Configuration->PasswordClearText, Password, StrSize (Password));

    //
    // Update uncommitted data of Browser
    //
    HiiSetBrowserData (
       &gDriverSampleFormSetGuid,
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
                  &gDriverSampleFormSetGuid,
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
  Get the value of <Number> in <BlockConfig> format, i.e. the value of OFFSET
  or WIDTH or VALUE.
  <BlockConfig> ::= 'OFFSET='<Number>&'WIDTH='<Number>&'VALUE'=<Number>

  This is a internal function.

  @param  StringPtr              String in <BlockConfig> format and points to the
                                 first character of <Number>.
  @param  Number                 The output value. Caller takes the responsibility
                                 to free memory.
  @param  Len                    Length of the <Number>, in characters.

  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to store neccessary
                                 structures.
  @retval EFI_SUCCESS            Value of <Number> is outputted in Number
                                 successfully.

**/
EFI_STATUS
GetValueOfNumber (
  IN EFI_STRING                    StringPtr,
  OUT UINT8                        **Number,
  OUT UINTN                        *Len
  )
{
  EFI_STRING               TmpPtr;
  UINTN                    Length;
  EFI_STRING               Str;
  UINT8                    *Buf;
  EFI_STATUS               Status;
  UINT8                    DigitUint8;
  UINTN                    Index;
  CHAR16                   TemStr[2];

  if (StringPtr == NULL || *StringPtr == L'\0' || Number == NULL || Len == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Buf = NULL;

  TmpPtr = StringPtr;
  while (*StringPtr != L'\0' && *StringPtr != L'&') {
    StringPtr++;
  }
  *Len   = StringPtr - TmpPtr;
  Length = *Len + 1;

  Str = (EFI_STRING) AllocateZeroPool (Length * sizeof (CHAR16));
  if (Str == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  CopyMem (Str, TmpPtr, *Len * sizeof (CHAR16));
  *(Str + *Len) = L'\0';

  Length = (Length + 1) / 2;
  Buf = (UINT8 *) AllocateZeroPool (Length);
  if (Buf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  
  Length = *Len;
  ZeroMem (TemStr, sizeof (TemStr));
  for (Index = 0; Index < Length; Index ++) {
    TemStr[0] = Str[Length - Index - 1];
    DigitUint8 = (UINT8) StrHexToUint64 (TemStr);
    if ((Index & 1) == 0) {
      Buf [Index/2] = DigitUint8;
    } else {
      Buf [Index/2] = (UINT8) ((DigitUint8 << 4) + Buf [Index/2]);
    }
  }

  *Number = Buf;
  Status  = EFI_SUCCESS;

Exit:
  if (Str != NULL) {
    FreePool (Str);
  }

  return Status;
}

/**
  Create altcfg string. 

  @param  Result               The request result string.
  @param  ConfigHdr            The request head info. <ConfigHdr> format.
  @param  Offset               The offset of the parameter int he structure.
  @param  Width                The width of the parameter.


  @retval  The string with altcfg info append at the end.
**/
EFI_STRING 
CreateAltCfgString (
  IN     EFI_STRING     Result,
  IN     EFI_STRING     ConfigHdr,
  IN     UINTN          Offset,
  IN     UINTN          Width
  )
{
  EFI_STRING StringPtr;
  EFI_STRING TmpStr;
  UINTN      NewLen;

  NewLen = StrLen (Result);
  //
  // String Len = ConfigResp + AltConfig + AltConfig + 1("\0")
  //
  NewLen = (NewLen + ((1 + StrLen (ConfigHdr) + 8 + 4) + (8 + 4 + 7 + 4 + 7 + 4)) * 2 + 1) * sizeof (CHAR16);
  StringPtr = AllocateZeroPool (NewLen);
  if (StringPtr == NULL) {
    return NULL;
  }

  TmpStr = StringPtr;
  if (Result != NULL) {
    StrCpy (StringPtr, Result);
    StringPtr += StrLen (Result);  
    FreePool (Result);
  }
  
  UnicodeSPrint (
  StringPtr, 
  (1 + StrLen (ConfigHdr) + 8 + 4 + 1) * sizeof (CHAR16), 
  L"&%s&ALTCFG=%04x", 
  ConfigHdr, 
  EFI_HII_DEFAULT_CLASS_STANDARD
  );
  StringPtr += StrLen (StringPtr);

  UnicodeSPrint (
    StringPtr, 
    (8 + 4 + 7 + 4 + 7 + 4 + 1) * sizeof (CHAR16),
    L"&OFFSET=%04x&WIDTH=%04x&VALUE=%04x", 
    Offset, 
    Width,
    DEFAULT_CLASS_STANDARD_VALUE
    );
  StringPtr += StrLen (StringPtr);  

  UnicodeSPrint (
  StringPtr, 
  (1 + StrLen (ConfigHdr) + 8 + 4 + 1) * sizeof (CHAR16), 
  L"&%s&ALTCFG=%04x", 
  ConfigHdr, 
  EFI_HII_DEFAULT_CLASS_MANUFACTURING
  );
  StringPtr += StrLen (StringPtr);

  UnicodeSPrint (
    StringPtr, 
    (8 + 4 + 7 + 4 + 7 + 4 + 1) * sizeof (CHAR16),
    L"&OFFSET=%04x&WIDTH=%04x&VALUE=%04x", 
    Offset, 
    Width,
    DEFAULT_CLASS_MANUFACTURING_VALUE
    );
  StringPtr += StrLen (StringPtr); 

  return TmpStr;
}

/**
  Check whether need to add the altcfg string. if need to add, add the altcfg 
  string.

  @param  RequestResult              The request result string.
  @param  ConfigRequestHdr           The request head info. <ConfigHdr> format.

**/
VOID 
AppendAltCfgString (
  IN OUT EFI_STRING                       *RequestResult,
  IN     EFI_STRING                       ConfigRequestHdr
  )
{
  EFI_STRING                          StringPtr;
  UINTN                               Length;
  UINT8                               *TmpBuffer;
  UINTN                               Offset;
  UINTN                               Width;
  UINTN                               BlockSize;
  UINTN                               ValueOffset;
  UINTN                               ValueWidth;
  EFI_STATUS                          Status;

  StringPtr = *RequestResult;
  StringPtr = StrStr (StringPtr, L"OFFSET");
  BlockSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  ValueOffset = OFFSET_OF (DRIVER_SAMPLE_CONFIGURATION, GetDefaultValueFromAccess);
  ValueWidth  = sizeof (((DRIVER_SAMPLE_CONFIGURATION *)0)->GetDefaultValueFromAccess);

  if (StringPtr == NULL) {
    return;
  }

  while (*StringPtr != 0 && StrnCmp (StringPtr, L"OFFSET=", StrLen (L"OFFSET=")) == 0) {
    StringPtr += StrLen (L"OFFSET=");
    //
    // Get Offset
    //
    Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
    if (EFI_ERROR (Status)) {
      return;
    }
    Offset = 0;
    CopyMem (
     &Offset,
     TmpBuffer,
     (((Length + 1) / 2) < sizeof (UINTN)) ? ((Length + 1) / 2) : sizeof (UINTN)
     );
    FreePool (TmpBuffer);

    StringPtr += Length;
    if (StrnCmp (StringPtr, L"&WIDTH=", StrLen (L"&WIDTH=")) != 0) {
      return;
    }
    StringPtr += StrLen (L"&WIDTH=");

    //
    // Get Width
    //
    Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
    if (EFI_ERROR (Status)) {
      return;
    }
    Width = 0;
    CopyMem (
     &Width,
     TmpBuffer,
     (((Length + 1) / 2) < sizeof (UINTN)) ? ((Length + 1) / 2) : sizeof (UINTN)
     );
    FreePool (TmpBuffer);

    StringPtr += Length;
    if (StrnCmp (StringPtr, L"&VALUE=", StrLen (L"&VALUE=")) != 0) {
      return;
    }
    StringPtr += StrLen (L"&VALUE=");

    //
    // Get Value
    //
    Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
    if (EFI_ERROR (Status)) {
      return;
    }
    StringPtr += Length;

    //
    // Calculate Value and convert it to hex string.
    //
    if (Offset + Width > BlockSize) {
      return;
    }

    if (Offset <= ValueOffset && Offset + Width >= ValueOffset + ValueWidth) {
      *RequestResult = CreateAltCfgString(*RequestResult, ConfigRequestHdr, ValueOffset, ValueWidth);
      return;
    }
  }
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
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
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
  BOOLEAN                          AllocatedRequest;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize the local variables.
  //
  ConfigRequestHdr  = NULL;
  ConfigRequest     = NULL;
  Size              = 0;
  *Progress         = Request;
  AllocatedRequest  = FALSE;

  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  // Get Buffer Storage data from EFI variable.
  // Try to get the current setting from variable.
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (
            VariableName,
            &gDriverSampleFormSetGuid,
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
    ConfigRequestHdr = HiiConstructConfigHdr (&gDriverSampleFormSetGuid, VariableName, PrivateData->DriverHandle[0]);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
    ConfigRequestHdr = NULL;
  } else {
    //
    // Check routing data in <ConfigHdr>.
    // Note: if only one Storage is used, then this checking could be skipped.
    //
    if (!HiiIsConfigHdrMatch (Request, &gDriverSampleFormSetGuid, NULL)) {
      return EFI_NOT_FOUND;
    }
    //
    // Check whether request for EFI Varstore. EFI varstore get data
    // through hii database, not support in this path.
    //
    if (HiiIsConfigHdrMatch(Request, &gDriverSampleFormSetGuid, MyEfiVar)) {
      return EFI_UNSUPPORTED;
    }
    //
    // Set Request to the unified request string.
    //
    ConfigRequest = Request;
    //
    // Check whether Request includes Request Element.
    //
    if (StrStr (Request, L"OFFSET") == NULL) {
      //
      // Check Request Element does exist in Reques String
      //
      StrPointer = StrStr (Request, L"PATH");
      if (StrPointer == NULL) {
        return EFI_INVALID_PARAMETER;
      }
      if (StrStr (StrPointer, L"&") == NULL) {
        Size = (StrLen (Request) + 32 + 1) * sizeof (CHAR16);
        ConfigRequest    = AllocateZeroPool (Size);
        ASSERT (ConfigRequest != NULL);
        AllocatedRequest = TRUE;
        UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", Request, (UINT64)BufferSize);
      }
    }
  }

  //
  // Check if requesting Name/Value storage
  //
  if (StrStr (ConfigRequest, L"OFFSET") == NULL) {
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
    BufferSize = (StrLen (ConfigRequest) +
      1 + sizeof (PrivateData->Configuration.NameValueVar0) * 2 +
      1 + sizeof (PrivateData->Configuration.NameValueVar1) * 2 +
      1 + sizeof (PrivateData->Configuration.NameValueVar2) * 2 + 1) * sizeof (CHAR16);
    *Results = AllocateZeroPool (BufferSize);
    ASSERT (*Results != NULL);
    StrCpy (*Results, ConfigRequest);
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
    
    Status = EFI_SUCCESS;
  } else {
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
    if (!EFI_ERROR (Status)) {
      ConfigRequestHdr = HiiConstructConfigHdr (&gDriverSampleFormSetGuid, VariableName, PrivateData->DriverHandle[0]);
      AppendAltCfgString(Results, ConfigRequestHdr);
    }
  }

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
  }

  if (ConfigRequestHdr != NULL) {
    FreePool (ConfigRequestHdr);
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
  if (!HiiIsConfigHdrMatch (Configuration, &gDriverSampleFormSetGuid, NULL)) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether request for EFI Varstore. EFI varstore get data
  // through hii database, not support in this path.
  //
  if (HiiIsConfigHdrMatch(Configuration, &gDriverSampleFormSetGuid, MyEfiVar)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (
            VariableName,
            &gDriverSampleFormSetGuid,
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
      &gDriverSampleFormSetGuid,
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
                  &gDriverSampleFormSetGuid,
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
  VOID                            *StartOpCodeHandle;
  VOID                            *OptionsOpCodeHandle;
  EFI_IFR_GUID_LABEL              *StartLabel;
  VOID                            *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL              *EndLabel;
  EFI_INPUT_KEY                   Key;
  DRIVER_SAMPLE_CONFIGURATION     *Configuration;
  MY_EFI_VARSTORE_DATA            *EfiData;
  EFI_FORM_ID                     FormId;
  
  if (((Value == NULL) && (Action != EFI_BROWSER_ACTION_FORM_OPEN) && (Action != EFI_BROWSER_ACTION_FORM_CLOSE))||
    (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }


  FormId = 0;
  Status = EFI_SUCCESS;
  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);

  switch (Action) {
  case EFI_BROWSER_ACTION_FORM_OPEN:
    {
      if (QuestionId == 0x1234) {
        //
        // Sample CallBack for UEFI FORM_OPEN action:
        //   Add Save action into Form 3 when Form 1 is opened.
        //   This will be done only in FORM_OPEN CallBack of question with ID 0x1234 from Form 1.
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
          &gDriverSampleFormSetGuid,  // Formset GUID
          0x3,                        // Form ID
          StartOpCodeHandle,          // Label for where to insert opcodes
          NULL                        // Insert data
          );

        HiiFreeOpCodeHandle (StartOpCodeHandle);
      }

      if (QuestionId == 0x1247) {
        Status = InternalStartMonitor ();
        ASSERT_EFI_ERROR (Status);
      }
    }
    break;

  case EFI_BROWSER_ACTION_FORM_CLOSE:
    {
      if (QuestionId == 0x5678) {
        //
        // Sample CallBack for UEFI FORM_CLOSE action:
        //   Show up a pop-up to specify Form 3 will be closed when exit Form 3.
        //
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"",
            L"You are going to leave third Form!",
            L"Press ESC or ENTER to continue ...",
            L"",
            NULL
            );
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));
      }

      if (QuestionId == 0x1247) {
        Status = InternalStopMonitor ();
        ASSERT_EFI_ERROR (Status);
      }
    }
    break;
    
  case EFI_BROWSER_ACTION_RETRIEVE:
    {
      if (QuestionId == 0x1248) {
        {
          if (Type != EFI_IFR_TYPE_REF) {
            return EFI_INVALID_PARAMETER;
          }
        
          Value->ref.FormId = 0x3;
        }
      }
    }
    break;

  case EFI_BROWSER_ACTION_DEFAULT_STANDARD:
    {
      switch (QuestionId) {
      case 0x1240:
        Value->u8 = DEFAULT_CLASS_STANDARD_VALUE;
      break;

      default:
        Status = EFI_UNSUPPORTED;
      break;
      }
    }
    break;

  case EFI_BROWSER_ACTION_DEFAULT_MANUFACTURING:
    {
      switch (QuestionId) {
      case 0x1240:
        Value->u8 = DEFAULT_CLASS_MANUFACTURING_VALUE;
      break;

      default:
        Status = EFI_UNSUPPORTED;      
      break;
      }
    }
    break;

  case EFI_BROWSER_ACTION_CHANGING:
  {
    switch (QuestionId) {
    case 0x1249:
      {
        if (Type != EFI_IFR_TYPE_REF) {
          return EFI_INVALID_PARAMETER;
        }

        Value->ref.FormId = 0x1234;
      }
    break;
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
                      &gDriverSampleFormSetGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                      sizeof (DRIVER_SAMPLE_CONFIGURATION),
                      &PrivateData->Configuration
                      );

      //
      // Set initial vlaue of dynamic created oneof Question in Form Browser
      //
      Configuration = AllocateZeroPool (sizeof (DRIVER_SAMPLE_CONFIGURATION));
      ASSERT (Configuration != NULL);
      if (HiiGetBrowserData (&gDriverSampleFormSetGuid, VariableName, sizeof (DRIVER_SAMPLE_CONFIGURATION), (UINT8 *) Configuration)) {
        Configuration->DynamicOneof = 2;

        //
        // Update uncommitted data of Browser
        //
        HiiSetBrowserData (
          &gDriverSampleFormSetGuid,
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

      HiiCreateTextOpCode (
        StartOpCodeHandle,
        STRING_TOKEN(STR_TEXT_SAMPLE_HELP),
        STRING_TOKEN(STR_TEXT_SAMPLE_HELP),
        STRING_TOKEN(STR_TEXT_SAMPLE_STRING)
      );

      HiiCreateDateOpCode (
        StartOpCodeHandle,
        0x8004,
        0x0,
        0x0,
        STRING_TOKEN(STR_DATE_SAMPLE_HELP),
        STRING_TOKEN(STR_DATE_SAMPLE_HELP),
        0,
        QF_DATE_STORAGE_TIME,
        NULL
        );

      HiiCreateTimeOpCode (
        StartOpCodeHandle,
        0x8005,
        0x0,
        0x0,
        STRING_TOKEN(STR_TIME_SAMPLE_HELP),
        STRING_TOKEN(STR_TIME_SAMPLE_HELP),
        0,
        QF_TIME_STORAGE_TIME,
        NULL
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
        &gDriverSampleFormSetGuid,  // Formset GUID
        0x1234,                     // Form ID
        StartOpCodeHandle,          // Label for where to insert opcodes
        EndOpCodeHandle             // Replace data
        );

      HiiFreeOpCodeHandle (StartOpCodeHandle);
      HiiFreeOpCodeHandle (OptionsOpCodeHandle);
      HiiFreeOpCodeHandle (EndOpCodeHandle);
      break;

    case 0x5678:
    case 0x1247:
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
      if (QuestionId == 0x5678) {
        StartLabel->Number       = LABEL_UPDATE2;
        FormId                   = 0x03;
        PrivateData->Configuration.DynamicRefresh++;
      } else if (QuestionId == 0x1247 ) {
        StartLabel->Number       = LABEL_UPDATE3;
        FormId                   = 0x06;
        PrivateData->Configuration.RefreshGuidCount++;
      }

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
        &gDriverSampleFormSetGuid,              // Formset GUID
        FormId,                        // Form ID
        StartOpCodeHandle,          // Label for where to insert opcodes
        NULL                        // Insert data
        );

      HiiFreeOpCodeHandle (StartOpCodeHandle);

      //
      // Refresh the Question value
      //
      Status = gRT->SetVariable(
                      VariableName,
                      &gDriverSampleFormSetGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                      sizeof (DRIVER_SAMPLE_CONFIGURATION),
                      &PrivateData->Configuration
                      );

      if (QuestionId == 0x5678) {
        //
        // Update uncommitted data of Browser
        //
        EfiData = AllocateZeroPool (sizeof (MY_EFI_VARSTORE_DATA));
        ASSERT (EfiData != NULL);
        if (HiiGetBrowserData (&gDriverSampleFormSetGuid, MyEfiVar, sizeof (MY_EFI_VARSTORE_DATA), (UINT8 *) EfiData)) {
          EfiData->Field8 = 111;
          HiiSetBrowserData (
            &gDriverSampleFormSetGuid,
            MyEfiVar,
            sizeof (MY_EFI_VARSTORE_DATA),
            (UINT8 *) EfiData,
            NULL
            );
        }
        FreePool (EfiData);
      }
      break;

    case 0x2000:
      //
      // Only used to update the state.
      //
      if ((Type == EFI_IFR_TYPE_STRING) && (Value->string == 0) && 
        (PrivateData->PasswordState == BROWSER_STATE_SET_PASSWORD)) {
        PrivateData->PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
        return EFI_INVALID_PARAMETER;
      }

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
  }
  break;

  case EFI_BROWSER_ACTION_CHANGED:
    switch (QuestionId) {
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
      
      case 0x1241:
      case 0x1246:
        //
        // User press "Submit current form and Exit now", request Browser to submit current form and exit
        //
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
        break;
      
      case 0x1242:
        //
        // User press "Discard current form now", request Browser to discard the uncommitted data.
        //
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD;
        break;
      
      case 0x1243:
        //
        // User press "Submit current form now", request Browser to save the uncommitted data.
        //
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        break;
      
      case 0x1244:
      case 0x1245:
        //
        // User press "Discard current form and Exit now", request Browser to discard the uncommitted data and exit.
        //
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
        break;
        
      default:
      break;
    }
  break;

  default:
    Status = EFI_UNSUPPORTED;
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
  MY_EFI_VARSTORE_DATA            *VarStoreConfig;

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
                   &gDriverSampleFormSetGuid,
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
                   &gDriverSampleInventoryGuid,
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
  // Update the device path string.
  //
  if (HiiSetString (HiiHandle[0], STRING_TOKEN (STR_DEVICE_PATH), (EFI_STRING) &mHiiVendorDevicePath0, NULL) == 0) {
    DriverSampleUnload (ImageHandle);
    return EFI_OUT_OF_RESOURCES;
  }
  
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
  ConfigRequestHdr = HiiConstructConfigHdr (&gDriverSampleFormSetGuid, VariableName, DriverHandle[0]);
  ASSERT (ConfigRequestHdr != NULL);

  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (VariableName, &gDriverSampleFormSetGuid, NULL, &BufferSize, Configuration);
  if (EFI_ERROR (Status)) {
    //
    // Store zero data Buffer Storage to EFI variable
    //
    Status = gRT->SetVariable(
                    VariableName,
                    &gDriverSampleFormSetGuid,
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
  // Initialize efi varstore configuration data
  //
  VarStoreConfig = &PrivateData->VarStoreConfig;
  ZeroMem (VarStoreConfig, sizeof (MY_EFI_VARSTORE_DATA));

  ConfigRequestHdr = HiiConstructConfigHdr (&gDriverSampleFormSetGuid, MyEfiVar, DriverHandle[0]);
  ASSERT (ConfigRequestHdr != NULL);

  BufferSize = sizeof (MY_EFI_VARSTORE_DATA);
  Status = gRT->GetVariable (MyEfiVar, &gDriverSampleFormSetGuid, NULL, &BufferSize, VarStoreConfig);
  if (EFI_ERROR (Status)) {
    //
    // Store zero data to EFI variable Storage.
    //
    Status = gRT->SetVariable(
                    MyEfiVar,
                    &gDriverSampleFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (MY_EFI_VARSTORE_DATA),
                    VarStoreConfig
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

  Status = gBS->CreateEventEx (
        EVT_NOTIFY_SIGNAL, 
        TPL_NOTIFY,
        DriverSampleInternalEmptyFunction,
        NULL,
        &gEfiIfrRefreshIdOpGuid,
        &mEvent
        );
  ASSERT_EFI_ERROR (Status);
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

  ASSERT (PrivateData != NULL);

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

  for (Index = 0; Index < NAME_VALUE_NAME_NUMBER; Index++) {
    if (PrivateData->NameValueName[Index] != NULL) {
      FreePool (PrivateData->NameValueName[Index]);
    }
  }
  FreePool (PrivateData);
  PrivateData = NULL;

  gBS->CloseEvent (mEvent);

  return EFI_SUCCESS;
}
