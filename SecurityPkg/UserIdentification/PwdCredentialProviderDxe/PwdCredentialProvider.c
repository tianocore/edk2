/** @file
  Password Credential Provider driver implementation.
    
Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PwdCredentialProvider.h"

CREDENTIAL_TABLE            *mPwdTable      = NULL;
PWD_PROVIDER_CALLBACK_INFO  *mCallbackInfo  = NULL;
PASSWORD_CREDENTIAL_INFO    *mPwdInfoHandle = NULL;

HII_VENDOR_DEVICE_PATH      mHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    PWD_CREDENTIAL_PROVIDER_GUID
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

EFI_USER_CREDENTIAL2_PROTOCOL  gPwdCredentialProviderDriver = {
  PWD_CREDENTIAL_PROVIDER_GUID,
  EFI_USER_CREDENTIAL_CLASS_PASSWORD,
  CredentialEnroll,
  CredentialForm,
  CredentialTile,
  CredentialTitle,
  CredentialUser,
  CredentialSelect,
  CredentialDeselect,
  CredentialDefault,
  CredentialGetInfo,
  CredentialGetNextInfo,
  EFI_CREDENTIAL_CAPABILITIES_ENROLL,
  CredentialDelete
};


/**
  Get string by string id from HII Interface.


  @param[in] Id      String ID to get the string from.

  @retval  CHAR16 *  String from ID.
  @retval  NULL      If error occurs.

**/
CHAR16 *
GetStringById (
  IN EFI_STRING_ID             Id
  )
{
  //
  // Get the current string for the current Language.
  //
  return HiiGetString (mCallbackInfo->HiiHandle, Id, NULL);
}


/**
  Expand password table size.

**/
VOID
ExpandTableSize (
  VOID
  )
{
  CREDENTIAL_TABLE  *NewTable;
  UINTN             Count;

  Count = mPwdTable->MaxCount + PASSWORD_TABLE_INC;
  //
  // Create new credential table.
  //
  NewTable = (CREDENTIAL_TABLE *) AllocateZeroPool (
                                    sizeof (CREDENTIAL_TABLE) + 
                                    (Count - 1) * sizeof (PASSWORD_INFO)
                                    );
  ASSERT (NewTable != NULL); 

  NewTable->MaxCount    = Count;
  NewTable->Count       = mPwdTable->Count;
  NewTable->ValidIndex  = mPwdTable->ValidIndex;
  //
  // Copy old entries
  //
  CopyMem (
    &NewTable->UserInfo, 
    &mPwdTable->UserInfo, 
    mPwdTable->Count * sizeof (PASSWORD_INFO)
    );
  FreePool (mPwdTable);
  mPwdTable = NewTable;
}


/**
  Add, update or delete info in table, and sync with NV variable.

  @param[in]  Index     The index of the password in table. If index is found in
                        table, update the info, else add the into to table. 
  @param[in]  Info      The new password info to add into table.If Info is NULL, 
                        delete the info by Index.

  @retval EFI_INVALID_PARAMETER  Info is NULL when save the info.
  @retval EFI_SUCCESS            Modify the table successfully.
  @retval Others                 Failed to modify the table.

**/
EFI_STATUS
ModifyTable (
  IN  UINTN                                     Index,
  IN  PASSWORD_INFO                             * Info OPTIONAL
  )
{
  EFI_STATUS       Status;
  PASSWORD_INFO    *NewPasswordInfo;

  NewPasswordInfo = NULL;

  if (Index < mPwdTable->Count) {
    if (Info == NULL) {
      //
      // Delete the specified entry.
      //
      mPwdTable->Count--;
      if (Index != mPwdTable->Count) {
        NewPasswordInfo = &mPwdTable->UserInfo[mPwdTable->Count];
      } 
    } else {
      //
      // Update the specified entry.
      //
      NewPasswordInfo = Info;
    }
  } else {
    //
    // Add a new password info.
    //
    if (Info == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    if (mPwdTable->Count >= mPwdTable->MaxCount) {
      ExpandTableSize ();
    }

    NewPasswordInfo = Info;
    mPwdTable->Count++;
  }

  if (NewPasswordInfo != NULL) {
    CopyMem (&mPwdTable->UserInfo[Index], NewPasswordInfo, sizeof (PASSWORD_INFO));
  }

  //
  // Save the credential table.
  //
  Status = gRT->SetVariable (
                  L"PwdCredential",
                  &gPwdCredentialProviderGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  mPwdTable->Count * sizeof (PASSWORD_INFO),
                  &mPwdTable->UserInfo
                  );
  return Status;
}


/**
  Create a password table.

  @retval EFI_SUCCESS      Create a password table successfully.
  @retval Others           Failed to create a password.

**/
EFI_STATUS
InitCredentialTable (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       *Var;
  UINTN       VarSize;

  //
  // Get Password credential data from NV variable.
  //
  VarSize = 0;
  Var     = NULL;
  Status  = gRT->GetVariable (
                   L"PwdCredential", 
                   &gPwdCredentialProviderGuid, 
                   NULL, 
                   &VarSize,
                   Var
                   );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Var = AllocateZeroPool (VarSize);
    if (Var == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = gRT->GetVariable (
                    L"PwdCredential", 
                    &gPwdCredentialProviderGuid, 
                    NULL, 
                    &VarSize,
                    Var
                    );
  }
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }
  
  //
  // Create the password credential table.
  //
  mPwdTable = AllocateZeroPool (
                sizeof (CREDENTIAL_TABLE) - sizeof (PASSWORD_INFO) +
                PASSWORD_TABLE_INC * sizeof (PASSWORD_INFO) + 
                VarSize
                );
  if (mPwdTable == NULL) {
    FreePool (Var);
    return EFI_OUT_OF_RESOURCES;
  }

  mPwdTable->Count      = VarSize / sizeof (PASSWORD_INFO);
  mPwdTable->MaxCount   = mPwdTable->Count + PASSWORD_TABLE_INC;
  mPwdTable->ValidIndex = 0;
  if (Var != NULL) {
    CopyMem (mPwdTable->UserInfo, Var, VarSize);
    FreePool (Var);
  }
  return EFI_SUCCESS;
}


/**
  Hash the password to get credential.

  @param[in]   Password       Points to the input password.
  @param[in]   PasswordSize   The size of password, in bytes.
  @param[out]  Credential     Points to the hashed result.

  @retval      TRUE           Hash the password successfully.
  @retval      FALSE          Failed to hash the password.
                 
**/
BOOLEAN
GenerateCredential (
  IN      CHAR16                              *Password,
  IN      UINTN                               PasswordSize,
     OUT  UINT8                               *Credential
  )
{
  BOOLEAN           Status;
  UINTN             HashSize;
  VOID              *Hash;
  
  HashSize = Sha1GetContextSize ();
  Hash     = AllocatePool (HashSize);
  ASSERT (Hash != NULL);
  
  Status = Sha1Init (Hash);
  if (!Status) {
    goto Done;
  }
  
  Status = Sha1Update (Hash, Password, PasswordSize);
  if (!Status) {
    goto Done;
  }
  
  Status = Sha1Final (Hash, Credential);
  
Done:
  FreePool (Hash);
  return Status;
}


/**
  Get password from user input.

  @param[in]   FirstPwd       If True, prompt to input the first password.
                              If False, prompt to input password again.
  @param[out]  Credential     Points to the input password.

**/
VOID
GetPassword (
  IN  BOOLEAN                               FirstPwd,
  OUT CHAR8                                 *Credential
  )
{
  EFI_INPUT_KEY Key;
  CHAR16        PasswordMask[CREDENTIAL_LEN + 1];
  CHAR16        Password[CREDENTIAL_LEN];
  UINTN         PasswordLen;
  CHAR16        *QuestionStr;
  CHAR16        *LineStr;
  
  PasswordLen = 0;
  while (TRUE) {
    PasswordMask[PasswordLen]     = L'_';
    PasswordMask[PasswordLen + 1] = L'\0';
    LineStr = GetStringById (STRING_TOKEN (STR_DRAW_A_LINE));
    if (FirstPwd) {
      QuestionStr = GetStringById (STRING_TOKEN (STR_INPUT_PASSWORD));
    } else {
      QuestionStr = GetStringById (STRING_TOKEN (STR_INPUT_PASSWORD_AGAIN));
    }
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      QuestionStr,
      LineStr,
      PasswordMask,
      NULL
      );
    FreePool (QuestionStr);
    FreePool (LineStr);
    
    //
    // Check key stroke
    //
    if (Key.ScanCode == SCAN_NULL) {
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        break;
      } else if (Key.UnicodeChar == CHAR_BACKSPACE) {
        if (PasswordLen > 0) {
          PasswordLen--;
        }
      } else if ((Key.UnicodeChar == CHAR_NULL) || 
                 (Key.UnicodeChar == CHAR_TAB) || 
                 (Key.UnicodeChar == CHAR_LINEFEED)) {
        continue;
      } else {
        Password[PasswordLen] = Key.UnicodeChar;
        PasswordMask[PasswordLen] = L'*';
        PasswordLen++;
        if (PasswordLen == CREDENTIAL_LEN) {
          break;
        }
      }
    }
  }
  
  PasswordLen = PasswordLen * sizeof (CHAR16);
  GenerateCredential (Password, PasswordLen, (UINT8 *)Credential);
}

/**
  Check whether the password can be found on this provider.

  @param[in]  Password           The password to be found.

  @retval EFI_SUCCESS            Found password sucessfully.
  @retval EFI_NOT_FOUND          Fail to find the password.

**/
EFI_STATUS
CheckPassword (
  IN CHAR8                                      *Password
  )
{
  UINTN      Index;
  CHAR8      *Pwd;
  
  //
  // Check password credential.
  //
  mPwdTable->ValidIndex = 0;
  for (Index = 0; Index < mPwdTable->Count; Index++) {
    Pwd = mPwdTable->UserInfo[Index].Password;
    if (CompareMem (Pwd, Password, CREDENTIAL_LEN) == 0) {
      mPwdTable->ValidIndex = Index + 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  Find a user infomation record by the information record type.

  This function searches all user information records of User from beginning 
  until either the information is found, or there are no more user infomation
  records. A match occurs when a Info.InfoType field matches the user information
  record type.

  @param[in]     User      Points to the user profile record to search.                          
  @param[in]     InfoType  The infomation type to be searched.
  @param[out]    Info      Points to the user info found, the caller is responsible
                           to free.
  
  @retval EFI_SUCCESS      Find the user information successfully.
  @retval Others           Fail to find the user information.

**/
EFI_STATUS
FindUserInfoByType (
  IN      EFI_USER_PROFILE_HANDLE               User,
  IN      UINT8                                 InfoType,
  OUT     EFI_USER_INFO                         **Info
  )
{
  EFI_STATUS                 Status;
  EFI_USER_INFO              *UserInfo;
  UINTN                      UserInfoSize;
  EFI_USER_INFO_HANDLE       UserInfoHandle;
  EFI_USER_MANAGER_PROTOCOL  *UserManager;
  
  //
  // Find user information by information type.
  //
  if (Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (
                  &gEfiUserManagerProtocolGuid,
                  NULL,
                  (VOID **) &UserManager
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get each user information.
  //

  UserInfoHandle = NULL;
  UserInfo       = NULL;
  UserInfoSize   = 0;
  while (TRUE) {
    Status = UserManager->GetNextInfo (UserManager, User, &UserInfoHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Get information.
    //
    Status = UserManager->GetInfo (
                            UserManager,
                            User,
                            UserInfoHandle,
                            UserInfo,
                            &UserInfoSize
                            );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      if (UserInfo != NULL) {
        FreePool (UserInfo);
      }
      UserInfo = AllocateZeroPool (UserInfoSize);
      if (UserInfo == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      Status = UserManager->GetInfo (
                              UserManager,
                              User,
                              UserInfoHandle,
                              UserInfo,
                              &UserInfoSize
                              );
    }
    if (EFI_ERROR (Status)) {
      break;
    }

    ASSERT (UserInfo != NULL);
    if (UserInfo->InfoType == InfoType) {
      *Info = UserInfo;
      return EFI_SUCCESS;
    }    
  }

  if (UserInfo != NULL) {
    FreePool (UserInfo);
  }
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
CredentialDriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS    Status;
  EFI_INPUT_KEY Key;
  CHAR8         Password[CREDENTIAL_LEN];
  CHAR16        *PromptStr;

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if (QuestionId == KEY_GET_PASSWORD) {
      //
      // Get and check password.
      //
      GetPassword (TRUE, Password);
      Status = CheckPassword (Password);
      if (EFI_ERROR (Status)) {
        PromptStr = GetStringById (STRING_TOKEN (STR_PASSWORD_INCORRECT));
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"",
          PromptStr,
          L"",
          NULL
          );
        FreePool (PromptStr);
        return Status;
      }
      *ActionRequest  = EFI_BROWSER_ACTION_REQUEST_EXIT;
    }    
    return EFI_SUCCESS;
  }

  //
  // All other action return unsupported.
  //
  return EFI_UNSUPPORTED;
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
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_NOT_FOUND;
}

/**
  This function initialize the data mainly used in form browser.

  @retval EFI_SUCCESS          Initialize form data successfully.
  @retval Others               Fail to Initialize form data.

**/
EFI_STATUS
InitFormBrowser (
  VOID
  )
{
  EFI_STATUS                  Status;
  PWD_PROVIDER_CALLBACK_INFO  *CallbackInfo;

  //
  // Initialize driver private data.
  //
  CallbackInfo = AllocateZeroPool (sizeof (PWD_PROVIDER_CALLBACK_INFO));
  if (CallbackInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CallbackInfo->Signature                   = PWD_PROVIDER_SIGNATURE;
  CallbackInfo->ConfigAccess.ExtractConfig  = FakeExtractConfig;
  CallbackInfo->ConfigAccess.RouteConfig    = FakeRouteConfig;
  CallbackInfo->ConfigAccess.Callback       = CredentialDriverCallback;
  CallbackInfo->DriverHandle  = NULL;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &CallbackInfo->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &CallbackInfo->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish HII data.
  //
  CallbackInfo->HiiHandle = HiiAddPackages (
                              &gPwdCredentialProviderGuid,
                              CallbackInfo->DriverHandle,
                              PwdCredentialProviderStrings,
                              PwdCredentialProviderVfrBin,
                              NULL
                              );
  if (CallbackInfo->HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  mCallbackInfo = CallbackInfo;

  return Status;
}


/**
  Enroll a user on a credential provider.

  This function enrolls a user on this credential provider. If the user exists on 
  this credential provider, update the user information on this credential provider; 
  otherwise add the user information on credential provider.
  
  @param[in] This                Points to this instance of EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in] User                The user profile to enroll.
 
  @retval EFI_SUCCESS            User profile was successfully enrolled.
  @retval EFI_ACCESS_DENIED      Current user profile does not permit enrollment on the
                                 user profile handle. Either the user profile cannot enroll
                                 on any user profile or cannot enroll on a user profile 
                                 other than the current user profile.
  @retval EFI_UNSUPPORTED        This credential provider does not support enrollment in
                                 the pre-OS.
  @retval EFI_DEVICE_ERROR       The new credential could not be created because of a device
                                 error.
  @retval EFI_INVALID_PARAMETER  User does not refer to a valid user profile handle.
  
**/
EFI_STATUS
EFIAPI
CredentialEnroll (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN        EFI_USER_PROFILE_HANDLE             User
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  PASSWORD_INFO             PwdInfo;
  EFI_USER_INFO             *UserInfo;
  CHAR8                     Password[CREDENTIAL_LEN];
  EFI_INPUT_KEY             Key;
  UINT8                     *UserId;
  CHAR16                    *QuestionStr;
  CHAR16                    *PromptStr;

  if ((This == NULL) || (User == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get User Identifier.
  //
  UserInfo = NULL;
  Status = FindUserInfoByType (
             User,
             EFI_USER_INFO_IDENTIFIER_RECORD,
             &UserInfo
             );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (PwdInfo.UserId, (UINT8 *) (UserInfo + 1), sizeof (EFI_USER_INFO_IDENTIFIER)); 
  FreePool (UserInfo);

  //
  // Get password from user.
  //  
  while (TRUE) {
    //
    // Input password.
    //
    GetPassword (TRUE, PwdInfo.Password);

    //
    // Input password again.
    //
    GetPassword (FALSE, Password);

    //
    // Compare the two password consistency.
    //
    if (CompareMem (PwdInfo.Password, Password, CREDENTIAL_LEN) == 0) {
      break;
    } 

    QuestionStr = GetStringById (STRING_TOKEN (STR_PASSWORD_MISMATCH));
    PromptStr   = GetStringById (STRING_TOKEN (STR_INPUT_PASSWORD_AGAIN));    
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      QuestionStr,
      L"",
      PromptStr,
      NULL
      );
    FreePool (QuestionStr);
    FreePool (PromptStr);
  }

  //
  // Check whether User is ever enrolled in the provider.
  // 
  for (Index = 0; Index < mPwdTable->Count; Index++) {
    UserId = (UINT8 *) &mPwdTable->UserInfo[Index].UserId;
    if (CompareMem (UserId, (UINT8 *) &PwdInfo.UserId, sizeof (EFI_USER_INFO_IDENTIFIER)) == 0) {
      //
      // User already exists, update the password.
      //      
      break;
    }
  }
   
  //
  // Enroll the User to the provider.
  //
  Status = ModifyTable (Index, &PwdInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  Returns the user interface information used during user identification.

  This function returns information about the form used when interacting with the
  user during user identification. The form is the first enabled form in the form-set
  class EFI_HII_USER_CREDENTIAL_FORMSET_GUID installed on the HII handle HiiHandle. If 
  the user credential provider does not require a form to identify the user, then this
  function should return EFI_NOT_FOUND.

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[out] Hii        On return, holds the HII database handle.
  @param[out] FormSetId  On return, holds the identifier of the form set which contains
                         the form used during user identification.
  @param[out] FormId     On return, holds the identifier of the form used during user 
                         identification.
                         
  @retval EFI_SUCCESS            Form returned successfully.
  @retval EFI_NOT_FOUND          Form not returned.
  @retval EFI_INVALID_PARAMETER  Hii is NULL or FormSetId is NULL or FormId is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialForm (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  OUT       EFI_HII_HANDLE                      *Hii,
  OUT       EFI_GUID                            *FormSetId,
  OUT       EFI_FORM_ID                         *FormId
  )
{
  if ((This == NULL) || (Hii == NULL) || 
      (FormSetId == NULL) || (FormId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Hii       = mCallbackInfo->HiiHandle;
  *FormId    = FORMID_GET_PASSWORD_FORM;
  CopyGuid (FormSetId, &gPwdCredentialProviderGuid);
  
  return EFI_SUCCESS;
}


/**
  Returns bitmap used to describe the credential provider type.

  This optional function returns a bitmap that is less than or equal to the number
  of pixels specified by Width and Height. If no such bitmap exists, then EFI_NOT_FOUND
  is returned. 

  @param[in]      This    Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in, out] Width   On entry, points to the desired bitmap width. If NULL then no 
                          bitmap information will be returned. On exit, points to the 
                          width of the bitmap returned.
  @param[in, out] Height  On entry, points to the desired bitmap height. If NULL then no
                          bitmap information will be returned. On exit, points to the 
                          height of the bitmap returned
  @param[out]     Hii     On return, holds the HII database handle. 
  @param[out]     Image   On return, holds the HII image identifier. 
 
  @retval EFI_SUCCESS            Image identifier returned successfully.
  @retval EFI_NOT_FOUND          Image identifier not returned.
  @retval EFI_INVALID_PARAMETER  Hii is NULL or Image is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialTile (
  IN  CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN  OUT    UINTN                               *Width,
  IN  OUT    UINTN                               *Height,
      OUT    EFI_HII_HANDLE                      *Hii,
      OUT    EFI_IMAGE_ID                        *Image
  )
{  
  if ((This == NULL) || (Hii == NULL) || (Image == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_NOT_FOUND;
}


/**
  Returns string used to describe the credential provider type.

  This function returns a string which describes the credential provider. If no
  such string exists, then EFI_NOT_FOUND is returned. 

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[out] Hii        On return, holds the HII database handle.
  @param[out] String     On return, holds the HII string identifier.
 
  @retval EFI_SUCCESS            String identifier returned successfully.
  @retval EFI_NOT_FOUND          String identifier not returned.
  @retval EFI_INVALID_PARAMETER  Hii is NULL or String is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialTitle (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  OUT       EFI_HII_HANDLE                      *Hii,
  OUT       EFI_STRING_ID                       *String
  )
{
  if ((This == NULL) || (Hii == NULL) || (String == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Set Hii handle and String ID.
  //
  *Hii    = mCallbackInfo->HiiHandle;
  *String = STRING_TOKEN (STR_CREDENTIAL_TITLE);

  return EFI_SUCCESS;
}


/**
  Return the user identifier associated with the currently authenticated user.

  This function returns the user identifier of the user authenticated by this credential
  provider. This function is called after the credential-related information has been 
  submitted on a form, OR after a call to Default() has returned that this credential is
  ready to log on.

  @param[in]  This           Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in]  User           The user profile handle of the user profile currently being 
                             considered by the user identity manager. If NULL, then no user
                             profile is currently under consideration.
  @param[out] Identifier     On return, points to the user identifier. 
 
  @retval EFI_SUCCESS            User identifier returned successfully.
  @retval EFI_NOT_READY          No user identifier can be returned.
  @retval EFI_ACCESS_DENIED      The user has been locked out of this user credential.
  @retval EFI_INVALID_PARAMETER  This is NULL, or Identifier is NULL.
  @retval EFI_NOT_FOUND          User is not NULL, and the specified user handle can't be
                                 found in user profile database
  
**/
EFI_STATUS
EFIAPI
CredentialUser (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN        EFI_USER_PROFILE_HANDLE             User,
  OUT       EFI_USER_INFO_IDENTIFIER            *Identifier
  )
{
  EFI_STATUS    Status;
  UINTN         Index;
  EFI_USER_INFO *UserInfo;
  UINT8         *UserId;
  UINT8         *NewUserId;
  CHAR8         *Pwd;
  CHAR8         *NewPwd;

  if ((This == NULL) || (Identifier == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mPwdTable->ValidIndex == 0) {
    //
    // No password input, or the input password doesn't match
    // anyone in PwdTable.
    //
    return EFI_NOT_READY;
  }
  
  if (User == NULL) {
    //
    // Return the user ID whose password matches the input password.
    // 
    CopyMem (
      Identifier, 
      &mPwdTable->UserInfo[mPwdTable->ValidIndex - 1].UserId, 
      sizeof (EFI_USER_INFO_IDENTIFIER)
      );    
    return EFI_SUCCESS;
  }
  
  //
  // Get the User's ID.
  //
  Status = FindUserInfoByType (
             User,
             EFI_USER_INFO_IDENTIFIER_RECORD,
             &UserInfo
             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Check whether the input password matches one in PwdTable.
  //
  for (Index = 0; Index < mPwdTable->Count; Index++) {
    UserId    = (UINT8 *) &mPwdTable->UserInfo[Index].UserId;
    NewUserId = (UINT8 *) (UserInfo + 1);
    if (CompareMem (UserId, NewUserId, sizeof (EFI_USER_INFO_IDENTIFIER)) == 0) {
      Pwd    = mPwdTable->UserInfo[Index].Password;
      NewPwd = mPwdTable->UserInfo[mPwdTable->ValidIndex - 1].Password;
      if (CompareMem (Pwd, NewPwd, CREDENTIAL_LEN) == 0) {
        CopyMem (Identifier, UserId, sizeof (EFI_USER_INFO_IDENTIFIER));
        FreePool (UserInfo);
        return EFI_SUCCESS;
      } 
    }
  }

  FreePool (UserInfo);  
  return EFI_NOT_READY;
}


/**
  Indicate that user interface interaction has begun for the specified credential.

  This function is called when a credential provider is selected by the user. If 
  AutoLogon returns FALSE, then the user interface will be constructed by the User
  Identity Manager. 

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[out] AutoLogon  On return, points to the credential provider's capabilities 
                         after the credential provider has been selected by the user. 
 
  @retval EFI_SUCCESS            Credential provider successfully selected.
  @retval EFI_INVALID_PARAMETER  AutoLogon is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialSelect (
  IN  CONST  EFI_USER_CREDENTIAL2_PROTOCOL   *This,
  OUT        EFI_CREDENTIAL_LOGON_FLAGS      *AutoLogon
  )
{
  if ((This == NULL) || (AutoLogon == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  *AutoLogon = 0;

  return EFI_SUCCESS;
}


/**
  Indicate that user interface interaction has ended for the specified credential.

  This function is called when a credential provider is deselected by the user.

  @param[in] This        Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
 
  @retval EFI_SUCCESS    Credential provider successfully deselected.
  
**/
EFI_STATUS
EFIAPI
CredentialDeselect (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This
  )
{
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}


/**
  Return the default logon behavior for this user credential.

  This function reports the default login behavior regarding this credential provider.  

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[out] AutoLogon  On return, holds whether the credential provider should be used
                         by default to automatically log on the user.  
 
  @retval EFI_SUCCESS            Default information successfully returned.
  @retval EFI_INVALID_PARAMETER  AutoLogon is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialDefault (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  OUT       EFI_CREDENTIAL_LOGON_FLAGS          *AutoLogon
  )
{
  if ((This == NULL) || (AutoLogon == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  *AutoLogon = 0;
  
  return EFI_SUCCESS;
}


/**
  Return information attached to the credential provider.

  This function returns user information. 

  @param[in]      This          Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in]      UserInfo      Handle of the user information data record. 
  @param[out]     Info          On entry, points to a buffer of at least *InfoSize bytes. On
                                exit, holds the user information. If the buffer is too small
                                to hold the information, then EFI_BUFFER_TOO_SMALL is returned
                                and InfoSize is updated to contain the number of bytes actually
                                required.
  @param[in, out] InfoSize      On entry, points to the size of Info. On return, points to the 
                                size of the user information. 
 
  @retval EFI_SUCCESS           Information returned successfully.
  @retval EFI_BUFFER_TOO_SMALL  The size specified by InfoSize is too small to hold all of the
                                user information. The size required is returned in *InfoSize.
  @retval EFI_INVALID_PARAMETER Info is NULL or InfoSize is NULL.
  @retval EFI_NOT_FOUND         The specified UserInfo does not refer to a valid user info handle. 
                                
**/
EFI_STATUS
EFIAPI
CredentialGetInfo (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN        EFI_USER_INFO_HANDLE                UserInfo,
  OUT       EFI_USER_INFO                       *Info,
  IN OUT    UINTN                               *InfoSize
  )
{
  EFI_USER_INFO            *CredentialInfo;
  UINTN                    Index;
  
  if ((This == NULL) || (InfoSize == NULL) || (Info == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UserInfo == NULL) || (mPwdInfoHandle == NULL)) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Find information handle in credential info table.
  //
  for (Index = 0; Index < mPwdInfoHandle->Count; Index++) {
    CredentialInfo = mPwdInfoHandle->Info[Index];
    if (UserInfo == (EFI_USER_INFO_HANDLE)CredentialInfo) {
      //
      // The handle is found, copy the user info.
      //
      if (CredentialInfo->InfoSize > *InfoSize) {
        *InfoSize = CredentialInfo->InfoSize;
        return EFI_BUFFER_TOO_SMALL;
      }
      CopyMem (Info, CredentialInfo, CredentialInfo->InfoSize);      
      return EFI_SUCCESS; 
    }
  }
  
  return EFI_NOT_FOUND;
}


/**
  Enumerate all of the user informations on the credential provider.

  This function returns the next user information record. To retrieve the first user
  information record handle, point UserInfo at a NULL. Each subsequent call will retrieve
  another user information record handle until there are no more, at which point UserInfo
  will point to NULL. 

  @param[in]      This     Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in, out] UserInfo On entry, points to the previous user information handle or NULL
                           to start enumeration. On exit, points to the next user information
                           handle or NULL if there is no more user information.
 
  @retval EFI_SUCCESS            User information returned.
  @retval EFI_NOT_FOUND          No more user information found.
  @retval EFI_INVALID_PARAMETER  UserInfo is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialGetNextInfo (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN OUT    EFI_USER_INFO_HANDLE                *UserInfo
  )
{
  EFI_USER_INFO            *Info;
  CHAR16                   *ProvNameStr;
  UINTN                    InfoLen;
  UINTN                    Index;
  UINTN                    ProvStrLen;
    
  if ((This == NULL) || (UserInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mPwdInfoHandle == NULL) {
    //
    // Initilized user info table. There are 4 user info records in the table.
    //
    InfoLen  = sizeof (PASSWORD_CREDENTIAL_INFO) + (4 - 1) * sizeof (EFI_USER_INFO *);
    mPwdInfoHandle = AllocateZeroPool (InfoLen);
    if (mPwdInfoHandle == NULL) {
      *UserInfo = NULL;
      return EFI_NOT_FOUND;
    }

    //
    // The first information, Credential Provider info.
    //
    InfoLen = sizeof (EFI_USER_INFO) + sizeof (EFI_GUID);
    Info    = AllocateZeroPool (InfoLen);
    ASSERT (Info != NULL);
    
    Info->InfoType    = EFI_USER_INFO_CREDENTIAL_PROVIDER_RECORD;
    Info->InfoSize    = (UINT32) InfoLen;
    Info->InfoAttribs = EFI_USER_INFO_PROTECTED;
    CopyGuid (&Info->Credential, &gPwdCredentialProviderGuid);
    CopyGuid ((EFI_GUID *)(Info + 1), &gPwdCredentialProviderGuid);
    
    mPwdInfoHandle->Info[0] = Info;
    mPwdInfoHandle->Count++;

    //
    // The second information, Credential Provider name info.
    //
    ProvNameStr = GetStringById (STRING_TOKEN (STR_PROVIDER_NAME));
    ProvStrLen  = StrSize (ProvNameStr);
    InfoLen     = sizeof (EFI_USER_INFO) + ProvStrLen;
    Info        = AllocateZeroPool (InfoLen);
    ASSERT (Info != NULL);
    
    Info->InfoType    = EFI_USER_INFO_CREDENTIAL_PROVIDER_NAME_RECORD;
    Info->InfoSize    = (UINT32) InfoLen;
    Info->InfoAttribs = EFI_USER_INFO_PROTECTED;
    CopyGuid (&Info->Credential, &gPwdCredentialProviderGuid);
    CopyMem ((UINT8*)(Info + 1), ProvNameStr, ProvStrLen);
    FreePool (ProvNameStr);

    mPwdInfoHandle->Info[1] = Info;
    mPwdInfoHandle->Count++;

    //
    // The third information, Credential Provider type info.
    //
    InfoLen = sizeof (EFI_USER_INFO) + sizeof (EFI_GUID);
    Info    = AllocateZeroPool (InfoLen);
    ASSERT (Info != NULL);
      
    Info->InfoType    = EFI_USER_INFO_CREDENTIAL_TYPE_RECORD;
    Info->InfoSize    = (UINT32) InfoLen;
    Info->InfoAttribs = EFI_USER_INFO_PROTECTED;
    CopyGuid (&Info->Credential, &gPwdCredentialProviderGuid);
    CopyGuid ((EFI_GUID *)(Info + 1), &gEfiUserCredentialClassPasswordGuid);
    
    mPwdInfoHandle->Info[2] = Info;
    mPwdInfoHandle->Count++;
 
    //
    // The fourth information, Credential Provider type name info.
    //
    ProvNameStr = GetStringById (STRING_TOKEN (STR_PROVIDER_TYPE_NAME));
    ProvStrLen  = StrSize (ProvNameStr);
    InfoLen     = sizeof (EFI_USER_INFO) + ProvStrLen;
    Info        = AllocateZeroPool (InfoLen);
    ASSERT (Info != NULL);
    
    Info->InfoType    = EFI_USER_INFO_CREDENTIAL_PROVIDER_NAME_RECORD;
    Info->InfoSize    = (UINT32) InfoLen;
    Info->InfoAttribs = EFI_USER_INFO_PROTECTED;
    CopyGuid (&Info->Credential, &gPwdCredentialProviderGuid);
    CopyMem ((UINT8*)(Info + 1), ProvNameStr, ProvStrLen);
    FreePool (ProvNameStr);
    
    mPwdInfoHandle->Info[3] = Info;
    mPwdInfoHandle->Count++;
  }
  
  if (*UserInfo == NULL) {
    //
    // Return the first info handle.
    //
    *UserInfo = (EFI_USER_INFO_HANDLE) mPwdInfoHandle->Info[0];
    return EFI_SUCCESS;
  }
  
  //
  // Find information handle in credential info table.
  //
  for (Index = 0; Index < mPwdInfoHandle->Count; Index++) {
    Info = mPwdInfoHandle->Info[Index];
    if (*UserInfo == (EFI_USER_INFO_HANDLE)Info) {
      //
      // The handle is found, get the next one.
      //
      if (Index == mPwdInfoHandle->Count - 1) {
        //
        // Already last one.
        //
        *UserInfo = NULL;
        return EFI_NOT_FOUND;
      }
    
      Index++;
      *UserInfo = (EFI_USER_INFO_HANDLE)mPwdInfoHandle->Info[Index];
      return EFI_SUCCESS;            
    }
  }

  *UserInfo = NULL;
  return EFI_NOT_FOUND;
}

/**
  Delete a user on this credential provider.

  This function deletes a user on this credential provider. 

  @param[in]     This            Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in]     User            The user profile handle to delete.

  @retval EFI_SUCCESS            User profile was successfully deleted.
  @retval EFI_ACCESS_DENIED      Current user profile does not permit deletion on the user profile handle. 
                                 Either the user profile cannot delete on any user profile or cannot delete 
                                 on a user profile other than the current user profile. 
  @retval EFI_UNSUPPORTED        This credential provider does not support deletion in the pre-OS.
  @retval EFI_DEVICE_ERROR       The new credential could not be deleted because of a device error.
  @retval EFI_INVALID_PARAMETER  User does not refer to a valid user profile handle.
**/
EFI_STATUS
EFIAPI
CredentialDelete (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN        EFI_USER_PROFILE_HANDLE             User
  )
{
  EFI_STATUS                Status;
  EFI_USER_INFO             *UserInfo;
  UINT8                     *UserId;
  UINT8                     *NewUserId;
  UINTN                     Index;
  
  if ((This == NULL) || (User == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get User Identifier.
  //
  UserInfo = NULL;
  Status = FindUserInfoByType (
             User,
             EFI_USER_INFO_IDENTIFIER_RECORD,
             &UserInfo
             );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the user by user identifier in mPwdTable.
  // 
  for (Index = 0; Index < mPwdTable->Count; Index++) {
    UserId    = (UINT8 *) &mPwdTable->UserInfo[Index].UserId;
    NewUserId = (UINT8 *) (UserInfo + 1);
    if (CompareMem (UserId, NewUserId, sizeof (EFI_USER_INFO_IDENTIFIER)) == 0) {
      //
      // Found the user, delete it.
      //
      ModifyTable (Index, NULL);
      break;
    }
  }

  FreePool (UserInfo);
  return EFI_SUCCESS;
}


/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
PasswordProviderInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // It is NOT robust enough to be included in production.
  //
  #error "This implementation is just a sample, please comment this line if you really want to use this driver."

  //
  // Init credential table.
  //
  Status = InitCredentialTable ();
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Init Form Browser.
  //
  Status = InitFormBrowser ();
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Install protocol interfaces for the password credential provider.
  //
  Status = gBS->InstallProtocolInterface (
                  &mCallbackInfo->DriverHandle,
                  &gEfiUserCredential2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gPwdCredentialProviderDriver
                  );
  return Status;
}
