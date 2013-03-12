/** @file
  This driver is a configuration tool for adding, deleting or modifying user 
  profiles, including gathering the necessary information to ascertain their 
  identity in the future, updating user access policy and identification 
  policy, etc.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UserProfileManager.h"

EFI_USER_MANAGER_PROTOCOL *mUserManager           = NULL;
CREDENTIAL_PROVIDER_INFO  *mProviderInfo          = NULL;
UINT8                     mProviderChoice;
UINT8                     mConncetLogical;
USER_INFO_ACCESS          mAccessInfo;
USER_INFO                 mUserInfo;
USER_PROFILE_MANAGER_CALLBACK_INFO  *mCallbackInfo;
HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    USER_PROFILE_MANAGER_GUID
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
  This function gets all the credential providers in the system and saved them 
  to mProviderInfo.

  @retval EFI_SUCESS     Init credential provider database successfully.
  @retval Others         Fail to init credential provider database.
  
**/
EFI_STATUS
InitProviderInfo (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuf;
  UINTN       Index;  
  
  //
  // Try to find all the user credential provider driver.
  //
  HandleCount = 0;
  HandleBuf   = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUserCredential2ProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuf
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Get provider infomation.
  //
  if (mProviderInfo != NULL) {
    FreePool (mProviderInfo);
  }
  mProviderInfo = AllocateZeroPool (
                    sizeof (CREDENTIAL_PROVIDER_INFO) - 
                    sizeof (EFI_USER_CREDENTIAL2_PROTOCOL *) +
                    HandleCount * sizeof (EFI_USER_CREDENTIAL2_PROTOCOL *)
                    );
  if (mProviderInfo == NULL) {
    FreePool (HandleBuf);
    return EFI_OUT_OF_RESOURCES;
  }

  mProviderInfo->Count = HandleCount;
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuf[Index],
                    &gEfiUserCredential2ProtocolGuid,
                    (VOID **) &mProviderInfo->Provider[Index]
                    );
    if (EFI_ERROR (Status)) {
      FreePool (HandleBuf);
      FreePool (mProviderInfo);
      mProviderInfo = NULL;
      return Status;
    }
  }

  FreePool (HandleBuf);
  return EFI_SUCCESS;
}


/**
  This function processes changes in user profile configuration.

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
  @retval Others                 Fail to handle the action.

**/
EFI_STATUS
EFIAPI
UserProfileManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL      *This,
  IN  EFI_BROWSER_ACTION                        Action,
  IN  EFI_QUESTION_ID                           QuestionId,
  IN  UINT8                                     Type,
  IN  EFI_IFR_TYPE_VALUE                        *Value,
  OUT EFI_BROWSER_ACTION_REQUEST                *ActionRequest
  )
{
  EFI_STATUS               Status;
  EFI_INPUT_KEY            Key;
  UINT32                   CurrentAccessRight;
  CHAR16                   *QuestionStr;
  CHAR16                   *PromptStr;
  VOID                     *StartOpCodeHandle;
  VOID                     *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL       *StartLabel;
  EFI_IFR_GUID_LABEL       *EndLabel;
  EFI_USER_PROFILE_HANDLE  CurrentUser;

  Status = EFI_SUCCESS;

  switch (Action) {
  case EFI_BROWSER_ACTION_FORM_OPEN:
    {
      //
      // Update user manage Form when user manage Form is opened.
      // This will be done only in FORM_OPEN CallBack of question with QUESTIONID_USER_MANAGE from user manage Form.
      //
      if (QuestionId != QUESTIONID_USER_MANAGE) {
        return EFI_SUCCESS;
      }
  
      //
      // Get current user
      //
      CurrentUser = NULL;
      mUserManager->Current (mUserManager, &CurrentUser);
      if (CurrentUser == NULL) {
        DEBUG ((DEBUG_ERROR, "Error: current user does not exist!\n"));
        return EFI_NOT_READY;
      }
      
      //
      // Get current user's right information.
      //
      Status = GetAccessRight (&CurrentAccessRight);
      if (EFI_ERROR (Status)) {
        CurrentAccessRight = EFI_USER_INFO_ACCESS_ENROLL_SELF;
      }
  
      //
      // Init credential provider information.
      //
      Status = InitProviderInfo ();
      if (EFI_ERROR (Status)) {
        return Status;
      }
      
      //
      // Initialize the container for dynamic opcodes.
      //
      StartOpCodeHandle = HiiAllocateOpCodeHandle ();
      ASSERT (StartOpCodeHandle != NULL);
  
      EndOpCodeHandle = HiiAllocateOpCodeHandle ();
      ASSERT (EndOpCodeHandle != NULL);
  
      //
      // Create Hii Extend Label OpCode.
      //
      StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                            StartOpCodeHandle,
                                            &gEfiIfrTianoGuid,
                                            NULL,
                                            sizeof (EFI_IFR_GUID_LABEL)
                                            );
      StartLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
      StartLabel->Number        = LABEL_USER_MANAGE_FUNC;
  
      EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                          EndOpCodeHandle,
                                          &gEfiIfrTianoGuid,
                                          NULL,
                                          sizeof (EFI_IFR_GUID_LABEL)
                                          );
      EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
      EndLabel->Number        = LABEL_END;
  
      //
      // Add user profile option.
      //
      if ((CurrentAccessRight == EFI_USER_INFO_ACCESS_MANAGE) ||
          (CurrentAccessRight == EFI_USER_INFO_ACCESS_ENROLL_OTHERS)
          ) {
        HiiCreateActionOpCode (
          StartOpCodeHandle,                  // Container for dynamic created opcodes
          KEY_ADD_USER,                       // Question ID
          STRING_TOKEN (STR_ADD_USER_TITLE),  // Prompt text
          STRING_TOKEN (STR_ADD_USER_HELP),   // Help text
          EFI_IFR_FLAG_CALLBACK,              // Question flag
          0                                   // Action String ID
          );
      }
      
      //
      // Add modify user profile option.
      //
      HiiCreateGotoOpCode (
        StartOpCodeHandle,                    // Container for dynamic created opcodes
        FORMID_MODIFY_USER,                   // Target Form ID
        STRING_TOKEN (STR_MODIFY_USER_TITLE), // Prompt text
        STRING_TOKEN (STR_MODIFY_USER_HELP),  // Help text
        EFI_IFR_FLAG_CALLBACK,                // Question flag
        KEY_MODIFY_USER                       // Question ID
        );
  
      //
      // Add delete user profile option
      //
      if (CurrentAccessRight == EFI_USER_INFO_ACCESS_MANAGE) {
        HiiCreateGotoOpCode (
          StartOpCodeHandle,                    // Container for dynamic created opcodes
          FORMID_DEL_USER,                      // Target Form ID
          STRING_TOKEN (STR_DELETE_USER_TITLE), // Prompt text
          STRING_TOKEN (STR_DELETE_USER_HELP),  // Help text
          EFI_IFR_FLAG_CALLBACK,                // Question flag
          KEY_DEL_USER                          // Question ID
          );
      }
  
      HiiUpdateForm (
        mCallbackInfo->HiiHandle,               // HII handle
        &gUserProfileManagerGuid,               // Formset GUID
        FORMID_USER_MANAGE,                     // Form ID
        StartOpCodeHandle,                      // Label for where to insert opcodes
        EndOpCodeHandle                         // Replace data
        );
  
      HiiFreeOpCodeHandle (StartOpCodeHandle);
      HiiFreeOpCodeHandle (EndOpCodeHandle);
  
      return EFI_SUCCESS;
    }
    break;

  case EFI_BROWSER_ACTION_FORM_CLOSE:
    Status = EFI_SUCCESS;
    break;

  case EFI_BROWSER_ACTION_CHANGED:
  {  
    //
    // Handle the request from form.
    //
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
    }
    
    //
    // Judge first 2 bits.
    //
    switch (QuestionId & KEY_FIRST_FORM_MASK) {
    //
    // Add user profile operation.
    //
    case KEY_ADD_USER:
      CallAddUser ();
      break;

    //
    // Delete user profile operation.
    //
    case KEY_DEL_USER:
      //
      // Judge next 2 bits.
      //
      switch (QuestionId & KEY_SECOND_FORM_MASK) {
      //
      // Delete specified user profile.
      //
      case KEY_SELECT_USER:
        DeleteUser ((UINT8) QuestionId);
        //
        // Update select user form after delete a user.
        //
        SelectUserToDelete ();
        break;

      default:
        break;
      }
      break;

    //
    // Modify user profile operation.
    //
    case KEY_MODIFY_USER:
      //
      // Judge next 2 bits.
      //
      switch (QuestionId & KEY_SECOND_FORM_MASK) {
      //
      // Enter user profile information form.
      //
      case KEY_SELECT_USER:
        //
        // Judge next 3 bits.
        //
        switch (QuestionId & KEY_MODIFY_INFO_MASK) {
        //
        // Modify user name.
        //
        case KEY_MODIFY_NAME:
          ModifyUserName ();
          //
          // Update username in parent form.
          //
          SelectUserToModify ();
          break;

        //
        // Modify identity policy.
        //
        case KEY_MODIFY_IP:
          //
          // Judge next 3 bits
          //
          switch (QuestionId & KEY_MODIFY_IP_MASK) {
          //
          // Change credential provider option.
          //
          case KEY_MODIFY_PROV:         
            mProviderChoice = Value->u8;
            break;

          //
          // Change logical connector.
          //
          case KEY_MODIFY_CONN:
            mConncetLogical = Value->u8;
            break;

          //
          // Save option.
          //
          case KEY_ADD_IP_OP:
            AddIdentityPolicyItem ();
            break;

          //
          // Return to user profile information form.
          //
          case KEY_IP_RETURN_UIF:
            SaveIdentityPolicy ();
            *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
            break;

          default:
            break;
          }
          break;

        //
        // Modify access policy.
        //
        case KEY_MODIFY_AP:
          //
          // Judge next 3 bits.
          //
          switch (QuestionId & KEY_MODIFY_AP_MASK) {
          //
          // Change access right choice.
          //
          case KEY_MODIFY_RIGHT:
            mAccessInfo.AccessRight = Value->u8;
            break;

          //
          // Change setup choice.
          //
          case KEY_MODIFY_SETUP:
            mAccessInfo.AccessSetup= Value->u8;
            break;

          //
          // Change boot order choice.
          //
          case KEY_MODIFY_BOOT:
            mAccessInfo.AccessBootOrder = Value->u32;
            break;

          //
          // Return to user profile information form.
          //
          case KEY_AP_RETURN_UIF:
            SaveAccessPolicy ();
            *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
            break;

          default:
            break;
          }
          break;

        default:
          break;
        }
        break;

      //
      // Access policy device path modified.
      //
      case KEY_MODIFY_AP_DP:
        //
        // Judge next 2 bits.
        //
        switch (QuestionId & KEY_MODIFY_DP_MASK) {
        //
        // Load permit device path modified.
        //
        case KEY_LOAD_PERMIT_MODIFY:
          QuestionStr = GetStringById (STRING_TOKEN (STR_MOVE_TO_FORBID_LIST));
          PromptStr   = GetStringById (STRING_TOKEN (STR_PRESS_KEY_CONTINUE));
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
          if (Key.UnicodeChar != CHAR_CARRIAGE_RETURN) {
            break;
          }

          AddToForbidLoad ((UINT16)(QuestionId & (KEY_MODIFY_DP_MASK - 1)));
          DisplayLoadPermit ();
          break;

        //
        // Load forbid device path modified.
        //
        case KEY_LOAD_FORBID_MODIFY:
          QuestionStr = GetStringById (STRING_TOKEN (STR_MOVE_TO_PERMIT_LIST));
          PromptStr   = GetStringById (STRING_TOKEN (STR_PRESS_KEY_CONTINUE));
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
          if (Key.UnicodeChar != CHAR_CARRIAGE_RETURN) {
            break;
          }

          DeleteFromForbidLoad ((UINT16)(QuestionId & (KEY_MODIFY_DP_MASK - 1)));
          DisplayLoadForbid ();
          break;

        //
        // Connect permit device path modified.
        //
        case KEY_CONNECT_PERMIT_MODIFY:
          break;

        //
        // Connect forbid device path modified.
        //
        case KEY_CONNECT_FORBID_MODIFY:
          break;

        default:
          break;
        }
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


  case EFI_BROWSER_ACTION_CHANGING:
  {  
    //
    // Handle the request from form.
    //
    if (Value == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    
    //
    // Judge first 2 bits.
    //
    switch (QuestionId & KEY_FIRST_FORM_MASK) {
    //
    // Delete user profile operation.
    //
    case KEY_DEL_USER:
      //
      // Judge next 2 bits.
      //
      switch (QuestionId & KEY_SECOND_FORM_MASK) {
      //
      // Enter delete user profile form.
      //
      case KEY_ENTER_NEXT_FORM:
        SelectUserToDelete ();
        break;

      default:
        break;
      }
      break;

    //
    // Modify user profile operation.
    //
    case KEY_MODIFY_USER:
      //
      // Judge next 2 bits.
      //
      switch (QuestionId & KEY_SECOND_FORM_MASK) {
      //
      // Enter modify user profile form.
      //
      case KEY_ENTER_NEXT_FORM:
        SelectUserToModify ();
        break;

      //
      // Enter user profile information form.
      //
      case KEY_SELECT_USER:
        //
        // Judge next 3 bits.
        //
        switch (QuestionId & KEY_MODIFY_INFO_MASK) {
        //
        // Display user information form.
        //
        case KEY_ENTER_NEXT_FORM:
          ModifyUserInfo ((UINT8) QuestionId);
          break;

        //
        // Modify identity policy.
        //
        case KEY_MODIFY_IP:
          //
          // Judge next 3 bits
          //
          switch (QuestionId & KEY_MODIFY_IP_MASK) {
          //
          // Display identity policy modify form.
          //
          case KEY_ENTER_NEXT_FORM:
            ModifyIdentityPolicy ();
            break;

          default:
            break;
          }
          break;

        //
        // Modify access policy.
        //
        case KEY_MODIFY_AP:
          //
          // Judge next 3 bits.
          //
          switch (QuestionId & KEY_MODIFY_AP_MASK) {
          //
          // Display access policy modify form.
          //
          case KEY_ENTER_NEXT_FORM:
            ModidyAccessPolicy ();
            break;
          //
          // Load device path form.
          //
          case KEY_MODIFY_LOAD:
            //
            // Judge next 2 bits.
            //
            switch (QuestionId & KEY_DISPLAY_DP_MASK) {
            //
            // Permit load device path.
            //
            case KEY_PERMIT_MODIFY:
              DisplayLoadPermit ();
              break;
          
            //
            // Forbid load device path.
            //
            case KEY_FORBID_MODIFY:
              DisplayLoadForbid ();
              break;
          
            default:
              break;
            }
            break;
            
          //
          // Connect device path form.
          //
          case KEY_MODIFY_CONNECT:
            //
            // Judge next 2 bits.
            //
            switch (QuestionId & KEY_DISPLAY_DP_MASK) {
            //
            // Permit connect device path.
            //
            case KEY_PERMIT_MODIFY:
              DisplayConnectPermit ();
              break;
          
            //
            // Forbid connect device path.
            //
            case KEY_FORBID_MODIFY:
              DisplayConnectForbid ();
              break;
          
            default:
              break;
            }
            break;

          default:
            break;
          }
          break;

        default:
          break;
        }
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

  default:
    //
    // All other action return unsupported.
    //
    Status = EFI_UNSUPPORTED;
    break;
  }


  return Status;
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
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
UserProfileManagerInit (
  IN EFI_HANDLE                       ImageHandle,
  IN EFI_SYSTEM_TABLE                 *SystemTable
  )
{
  EFI_STATUS                          Status;
  USER_PROFILE_MANAGER_CALLBACK_INFO  *CallbackInfo;

  Status = gBS->LocateProtocol (
                  &gEfiUserManagerProtocolGuid,
                  NULL,
                  (VOID **) &mUserManager
                  );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }
  
  //
  // Initialize driver private data.
  //
  ZeroMem (&mUserInfo, sizeof (mUserInfo));
  ZeroMem (&mAccessInfo, sizeof (mAccessInfo));

  CallbackInfo = AllocateZeroPool (sizeof (USER_PROFILE_MANAGER_CALLBACK_INFO));
  ASSERT (CallbackInfo != NULL);  

  CallbackInfo->Signature                   = USER_PROFILE_MANAGER_SIGNATURE;
  CallbackInfo->ConfigAccess.ExtractConfig  = FakeExtractConfig;
  CallbackInfo->ConfigAccess.RouteConfig    = FakeRouteConfig;
  CallbackInfo->ConfigAccess.Callback       = UserProfileManagerCallback;
  CallbackInfo->DriverHandle                = NULL;
  
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
                              &gUserProfileManagerGuid,
                              CallbackInfo->DriverHandle,
                              UserProfileManagerStrings,
                              UserProfileManagerVfrBin,
                              NULL
                              );
  ASSERT (CallbackInfo->HiiHandle != NULL);                              
  mCallbackInfo = CallbackInfo;

  return Status;
}

  
