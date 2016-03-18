/** @file
  The functions for identification policy modification.
    
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UserProfileManager.h"


/**
  Verify the new identity policy in the current implementation. The same credential
  provider can't appear twice in one identity policy.

  @param[in] NewGuid       Points to the credential provider guid.
  
  @retval TRUE     The NewGuid was found in the identity policy.
  @retval FALSE    The NewGuid was not found.

**/
BOOLEAN
ProviderAlreadyInPolicy (
  IN EFI_GUID                                      *NewGuid
  )
{
  UINTN                         Offset;
  EFI_USER_INFO_IDENTITY_POLICY *Identity;
  EFI_INPUT_KEY                 Key;

  Offset = 0;
  while (Offset < mUserInfo.NewIdentityPolicyLen) {
    Identity = (EFI_USER_INFO_IDENTITY_POLICY *) (mUserInfo.NewIdentityPolicy + Offset);
    if (Identity->Type == EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER) {
      if (CompareGuid (NewGuid, (EFI_GUID *) (Identity + 1))) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"This Credential Provider Are Already Used!",
          L"",
          L"Press Any Key to Continue ...",
          NULL
          );
        return TRUE;
      }
    }
    Offset += Identity->Length;
  }
  
  return FALSE;
}


/**
  Add the user's credential record in the provider.

  @param[in]  Identity     Identity policy item including credential provider.
  @param[in]  User         Points to user profile.

  @retval EFI_SUCCESS      Add or delete record successfully.
  @retval Others           Fail to add or delete record.

**/
EFI_STATUS
EnrollUserOnProvider (
  IN  EFI_USER_INFO_IDENTITY_POLICY              *Identity,
  IN  EFI_USER_PROFILE_HANDLE                    User 
  )
{
  UINTN                          Index;
  EFI_USER_CREDENTIAL2_PROTOCOL  *UserCredential;
  
  //
  // Find the specified credential provider.
  //
  for (Index = 0; Index < mProviderInfo->Count; Index++) {
    UserCredential = mProviderInfo->Provider[Index];
    if (CompareGuid ((EFI_GUID *)(Identity + 1), &UserCredential->Identifier)) {
      return UserCredential->Enroll (UserCredential, User);
    }
  }

  return EFI_NOT_FOUND;  
}


/**
  Delete the User's credential record on the provider.

  @param[in]  Identity     Point to EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER user info.
  @param[in]  User         Points to user profile.

  @retval EFI_SUCCESS      Delete User's credential record successfully.
  @retval Others           Fail to add or delete record.

**/
EFI_STATUS
DeleteUserOnProvider (
  IN  EFI_USER_INFO_IDENTITY_POLICY              *Identity,
  IN  EFI_USER_PROFILE_HANDLE                    User 
  )
{
  UINTN                          Index;
  EFI_USER_CREDENTIAL2_PROTOCOL  *UserCredential;
  
  //
  // Find the specified credential provider.
  //
  for (Index = 0; Index < mProviderInfo->Count; Index++) {
    UserCredential = mProviderInfo->Provider[Index];
    if (CompareGuid ((EFI_GUID *)(Identity + 1), &UserCredential->Identifier)) {
      return UserCredential->Delete (UserCredential, User);
    }
  }

  return EFI_NOT_FOUND;  
}


/**
  Delete User's credental from all the providers that exist in User's identity policy.
  
  @param[in]  IdentityPolicy     Point to User's identity policy.
  @param[in]  IdentityPolicyLen  The length of the identity policy.
  @param[in]  User               Points to user profile.

**/
VOID
DeleteCredentialFromProviders (
  IN     UINT8                                *IdentityPolicy,
  IN     UINTN                                 IdentityPolicyLen,
  IN     EFI_USER_PROFILE_HANDLE               User 
  )
{
  EFI_USER_INFO_IDENTITY_POLICY    *Identity;
  UINTN                            Offset;

  Offset = 0;
  while (Offset < IdentityPolicyLen) {
    Identity = (EFI_USER_INFO_IDENTITY_POLICY *) (IdentityPolicy + Offset);
    if (Identity->Type == EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER) {
      //
      // Delete the user on this provider.
      //
      DeleteUserOnProvider (Identity, User);
    }
    Offset += Identity->Length;
  }

}


/**
  Remove the provider specified by Offset from the new user identification record.
  
  @param[in]  IdentityPolicy    Point to user identity item in new identification policy.
  @param[in]  Offset            The item offset in the new identification policy.

**/
VOID
DeleteProviderFromPolicy (
  IN     EFI_USER_INFO_IDENTITY_POLICY         *IdentityPolicy,
  IN     UINTN                                 Offset
  )
{
  UINTN                         RemainingLen;
  UINTN                         DeleteLen;

  if (IdentityPolicy->Length == mUserInfo.NewIdentityPolicyLen) {
    //
    // Only one credential provider in the identification policy.
    // Set the new policy to be TRUE after removed the provider.
    //
    IdentityPolicy->Type           = EFI_USER_INFO_IDENTITY_TRUE;
    IdentityPolicy->Length         = sizeof (EFI_USER_INFO_IDENTITY_POLICY);
    mUserInfo.NewIdentityPolicyLen = IdentityPolicy->Length;
    return ;
  }

  DeleteLen = IdentityPolicy->Length + sizeof(EFI_USER_INFO_IDENTITY_POLICY);
  if ((Offset + IdentityPolicy->Length) != mUserInfo.NewIdentityPolicyLen) {
    //
    // This provider is not the last item in the identification policy, delete it and the connector.
    //    
    RemainingLen = mUserInfo.NewIdentityPolicyLen - Offset - DeleteLen;
    CopyMem ((UINT8 *) IdentityPolicy, (UINT8 *) IdentityPolicy + DeleteLen, RemainingLen);
  }
  mUserInfo.NewIdentityPolicyLen -= DeleteLen;  
}


/**
  Add a new provider to the mUserInfo.NewIdentityPolicy.

  It is invoked when 'add option' in UI is pressed.

  @param[in] NewGuid       Points to the credential provider guid.
  
**/
VOID
AddProviderToPolicy (
  IN  EFI_GUID                                  *NewGuid
  )
{
  UINT8                         *NewPolicyInfo;
  UINTN                         NewPolicyInfoLen;
  EFI_USER_INFO_IDENTITY_POLICY *Policy;

  //
  // Allocate memory for the new identity policy.
  //
  NewPolicyInfoLen = mUserInfo.NewIdentityPolicyLen + sizeof (EFI_USER_INFO_IDENTITY_POLICY) + sizeof (EFI_GUID);
  if (mUserInfo.NewIdentityPolicyLen > 0) {
    //
    // It is not the first provider in the policy. Add a connector before provider.
    //
    NewPolicyInfoLen += sizeof (EFI_USER_INFO_IDENTITY_POLICY);
  }
  NewPolicyInfo = AllocateZeroPool (NewPolicyInfoLen);
  if (NewPolicyInfo == NULL) {
    return ;
  }

  NewPolicyInfoLen = 0;
  if (mUserInfo.NewIdentityPolicyLen > 0) {
    //
    // Save orginal policy.
    //
    CopyMem (NewPolicyInfo, mUserInfo.NewIdentityPolicy, mUserInfo.NewIdentityPolicyLen);

    //
    // Save logical connector.
    //
    Policy = (EFI_USER_INFO_IDENTITY_POLICY *) (NewPolicyInfo + mUserInfo.NewIdentityPolicyLen);
    if (mConncetLogical == 0) {
      Policy->Type = EFI_USER_INFO_IDENTITY_AND;
    } else {
      Policy->Type = EFI_USER_INFO_IDENTITY_OR;
    }

    Policy->Length   = sizeof (EFI_USER_INFO_IDENTITY_POLICY);
    NewPolicyInfoLen = mUserInfo.NewIdentityPolicyLen + Policy->Length;
    FreePool (mUserInfo.NewIdentityPolicy);
  }
  
  //
  // Save credential provider.
  //
  Policy = (EFI_USER_INFO_IDENTITY_POLICY *) (NewPolicyInfo + NewPolicyInfoLen);
  Policy->Length = sizeof (EFI_USER_INFO_IDENTITY_POLICY) + sizeof (EFI_GUID);
  Policy->Type   = EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER;
  CopyGuid ((EFI_GUID *) (Policy + 1), NewGuid);
  NewPolicyInfoLen += Policy->Length;

  //
  // Update identity policy choice.
  //
  mUserInfo.NewIdentityPolicy         = NewPolicyInfo;
  mUserInfo.NewIdentityPolicyLen      = NewPolicyInfoLen;
  mUserInfo.NewIdentityPolicyModified = TRUE;
}


/**
  This function replaces the old identity policy with a new identity policy.

  This function delete the user identity policy information.
  If enroll new credential failed, recover the old identity policy.

  @retval EFI_SUCCESS      Modify user identity policy successfully.
  @retval Others           Fail to modify user identity policy.

**/
EFI_STATUS
UpdateCredentialProvider (
  )
{
  EFI_STATUS                    Status;
  EFI_USER_INFO_IDENTITY_POLICY *Identity;
  UINTN                         Offset;

  //
  // Delete the old identification policy.
  //
  DeleteCredentialFromProviders (mUserInfo.IdentityPolicy, mUserInfo.IdentityPolicyLen, mModifyUser);

  //
  // Add the new identification policy.
  //
  Offset  = 0;
  while (Offset < mUserInfo.NewIdentityPolicyLen) {
    Identity = (EFI_USER_INFO_IDENTITY_POLICY *) (mUserInfo.NewIdentityPolicy + Offset);
    if (Identity->Type == EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER) {
      //
      // Enroll the user on this provider
      //
      Status = EnrollUserOnProvider (Identity, mModifyUser);
      if (EFI_ERROR (Status)) {
        //
        // Failed to enroll the user by new identification policy.
        // So removed the credential provider from the identification policy
        //
        DeleteProviderFromPolicy (Identity, Offset);
        continue;
      }
    }
    Offset += Identity->Length;
  }

  return EFI_SUCCESS;
}


/**
  Check whether the identity policy is valid.

  @param[in]  PolicyInfo          Point to the identity policy.
  @param[in]  PolicyInfoLen       The policy length.

  @retval TRUE     The policy is a valid identity policy.
  @retval FALSE    The policy is not a valid identity policy.
  
**/
BOOLEAN
CheckNewIdentityPolicy (
  IN  UINT8                                     *PolicyInfo,
  IN  UINTN                                     PolicyInfoLen
  )
{
  EFI_USER_INFO_IDENTITY_POLICY *Identity;
  EFI_INPUT_KEY                 Key;
  UINTN                         Offset;
  UINT32                        OpCode;
  
  //
  // Check policy expression.
  //
  OpCode  = EFI_USER_INFO_IDENTITY_FALSE;
  Offset  = 0;
  while (Offset < PolicyInfoLen) {
    //
    // Check identification policy according to type
    //
    Identity = (EFI_USER_INFO_IDENTITY_POLICY *) (PolicyInfo + Offset);
    switch (Identity->Type) {
      
    case EFI_USER_INFO_IDENTITY_TRUE:
      break;

    case EFI_USER_INFO_IDENTITY_OR:
      if (OpCode == EFI_USER_INFO_IDENTITY_AND) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid Identity Policy, Mixed Connector Unsupport!",
          L"",
          L"Press Any Key to Continue ...",
          NULL
          );
        return FALSE;
      }

      OpCode = EFI_USER_INFO_IDENTITY_OR;
      break;

    case EFI_USER_INFO_IDENTITY_AND:
      if (OpCode == EFI_USER_INFO_IDENTITY_OR) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid Identity Policy, Mixed Connector Unsupport!",
          L"",
          L"Press Any Key to Continue ...",
          NULL
          );
        return FALSE;
      }

      OpCode = EFI_USER_INFO_IDENTITY_AND;
      break;

    case EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER:
      break;

    default:
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Unsupport parameter",
        L"",
        L"Press Any Key to Continue ...",
        NULL
        );
      return FALSE;
    }
    Offset += Identity->Length;
  }

  return TRUE;
}


/**
  Save the identity policy and update UI with it.
  
  This funciton will verify the new identity policy, in current implementation, 
  the identity policy can be:  T, P & P & P & ..., P | P | P | ...
  Here, "T" means "True", "P" means "Credential Provider", "&" means "and", "|" means "or".
  Other identity policies are not supported.  

**/
VOID
SaveIdentityPolicy (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_USER_INFO_HANDLE          UserInfo;
  EFI_USER_INFO                 *Info;

  if (!mUserInfo.NewIdentityPolicyModified || (mUserInfo.NewIdentityPolicyLen == 0)) {
    return;
  }

  //
  // Check policy expression.
  //
  if (!CheckNewIdentityPolicy (mUserInfo.NewIdentityPolicy, mUserInfo.NewIdentityPolicyLen)) {
    return;
  }

  Status = FindInfoByType (mModifyUser, EFI_USER_INFO_IDENTITY_POLICY_RECORD, &UserInfo);
  if (EFI_ERROR (Status)) {
    return ;
  }
  
  //
  // Update the informantion on credential provider.
  //
  Status = UpdateCredentialProvider ();
  if (EFI_ERROR (Status)) {
    return ;
  }
  
  //
  // Save new identification policy.
  //
  Info = AllocateZeroPool (sizeof (EFI_USER_INFO) + mUserInfo.NewIdentityPolicyLen);
  ASSERT (Info != NULL);

  Info->InfoType    = EFI_USER_INFO_IDENTITY_POLICY_RECORD;
  Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | EFI_USER_INFO_PUBLIC | EFI_USER_INFO_EXCLUSIVE;
  Info->InfoSize    = (UINT32) (sizeof (EFI_USER_INFO) + mUserInfo.NewIdentityPolicyLen);
  CopyMem ((UINT8 *) (Info + 1), mUserInfo.NewIdentityPolicy, mUserInfo.NewIdentityPolicyLen);

  Status = mUserManager->SetInfo (mUserManager, mModifyUser, &UserInfo, Info, Info->InfoSize);
  FreePool (Info);
   
  //
  // Update the mUserInfo.IdentityPolicy by mUserInfo.NewIdentityPolicy
  //
  if (mUserInfo.IdentityPolicy != NULL) {
    FreePool (mUserInfo.IdentityPolicy);
  }
  mUserInfo.IdentityPolicy    = mUserInfo.NewIdentityPolicy;
  mUserInfo.IdentityPolicyLen = mUserInfo.NewIdentityPolicyLen;

  mUserInfo.NewIdentityPolicy         = NULL;
  mUserInfo.NewIdentityPolicyLen      = 0;
  mUserInfo.NewIdentityPolicyModified = FALSE;   

  //
  // Update identity policy choice.
  //
  ResolveIdentityPolicy (mUserInfo.IdentityPolicy, mUserInfo.IdentityPolicyLen, STRING_TOKEN (STR_IDENTIFY_POLICY_VAL));
}


/**
  Update the mUserInfo.NewIdentityPolicy, and UI when 'add option' is pressed.

**/
VOID
AddIdentityPolicyItem (
  VOID
  )
{
  if (mProviderInfo->Count == 0) {
    return ;
  }
  
  //
  // Check the identity policy.
  //
  if (ProviderAlreadyInPolicy (&mProviderInfo->Provider[mProviderChoice]->Identifier)) {
    return;
  }

  //
  // Add it to identification policy
  //
  AddProviderToPolicy (&mProviderInfo->Provider[mProviderChoice]->Identifier);

  //
  // Update identity policy choice.
  //
  ResolveIdentityPolicy (mUserInfo.NewIdentityPolicy, mUserInfo.NewIdentityPolicyLen, STRING_TOKEN (STR_IDENTIFY_POLICY_VALUE));
}


