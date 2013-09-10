/** @file
  The header file for user profile manager driver.
    
Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_USER_PROFILE_MANAGER_H__
#define __EFI_USER_PROFILE_MANAGER_H__

#include <Uefi.h>

#include <Guid/GlobalVariable.h>
#include <Guid/MdeModuleHii.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/UserCredential2.h>
#include <Protocol/UserManager.h>

#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>

#include "UserProfileManagerData.h"

#define  USER_NAME_LENGTH          17

//
// Credential Provider Information.
//
typedef struct {
  UINTN                         Count;
  EFI_USER_CREDENTIAL2_PROTOCOL *Provider[1];
} CREDENTIAL_PROVIDER_INFO;

//
// User profile information structure.
//
typedef struct {
  UINT64    UsageCount;
  EFI_TIME  CreateDate;
  EFI_TIME  UsageDate;
  UINTN     AccessPolicyLen;
  UINTN     IdentityPolicyLen;
  UINTN     NewIdentityPolicyLen;    
  UINT8     *AccessPolicy;
  UINT8     *IdentityPolicy;
  UINT8     *NewIdentityPolicy;
  CHAR16    UserName[USER_NAME_LENGTH];
  BOOLEAN   CreateDateExist;
  BOOLEAN   UsageDateExist;
  BOOLEAN   AccessPolicyModified;
  BOOLEAN   IdentityPolicyModified;
  BOOLEAN   NewIdentityPolicyModified;
} USER_INFO;

//
// User access information structure.
//
typedef struct {
  UINTN  LoadPermitLen;
  UINTN  LoadForbidLen;
  UINTN  ConnectPermitLen;
  UINTN  ConnectForbidLen;
  UINT8  *LoadPermit;
  UINT8  *LoadForbid;
  UINT8  *ConnectPermit;
  UINT8  *ConnectForbid;
  UINT32 AccessBootOrder;
  UINT8  AccessRight;
  UINT8  AccessSetup;
} USER_INFO_ACCESS;

#define USER_PROFILE_MANAGER_SIGNATURE  SIGNATURE_32 ('U', 'P', 'M', 'S')

typedef struct {
  UINTN                           Signature;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;
} USER_PROFILE_MANAGER_CALLBACK_INFO;

//
// HII specific Vendor Device Path definition.
//
typedef struct {
  VENDOR_DEVICE_PATH        VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  End;
} HII_VENDOR_DEVICE_PATH;

//
// This is the generated IFR binary data for each formset defined in VFR.
//
extern UINT8                               UserProfileManagerVfrBin[];

//
// This is the generated String package data for .UNI file.
//
extern UINT8                               UserProfileManagerStrings[];

//
// The user manager protocol, used in several function.
//
extern EFI_USER_MANAGER_PROTOCOL           *mUserManager;

//
// The credential providers database in system.
//
extern CREDENTIAL_PROVIDER_INFO            *mProviderInfo;

//
// The variables used to update identity policy.
//
extern UINT8                               mProviderChoice;
extern UINT8                               mConncetLogical;

//
// The variables used to update access policy.
//
extern USER_INFO_ACCESS                    mAccessInfo;

//
// The user information used to record all data in UI.
//
extern USER_INFO                           mUserInfo;

extern USER_PROFILE_MANAGER_CALLBACK_INFO  *mCallbackInfo;

extern EFI_USER_PROFILE_HANDLE             mModifyUser;

/**
  Get string by string id from HII Interface.


  @param[in] Id      String ID to get the string from.

  @retval  CHAR16 *  String from ID.
  @retval  NULL      If error occurs.

**/
CHAR16 *
GetStringById (
  IN EFI_STRING_ID             Id
  );

/**
  Add a new user profile into the user profile database.

**/
VOID
CallAddUser (
  VOID
  );

/**
  Display user select form; can select a user to modify.

**/
VOID
SelectUserToModify  (
  VOID
  );

/**
  Display user select form, cab select a user to delete.

**/
VOID
SelectUserToDelete (
  VOID
  );

/**
  Delete the user specified by UserIndex in user profile database.

  @param[in]  UserIndex    The index of user in the user name list to be deleted.

**/
VOID
DeleteUser (
  IN UINT8                                      UserIndex
  );

/**
  Add a username item in form.

  @param[in]  User             Points to the user profile whose username is added. 
  @param[in]  Index            The index of the user in the user name list.
  @param[in]  OpCodeHandle     Points to container for dynamic created opcodes.

**/
VOID
AddUserToForm (
  IN  EFI_USER_PROFILE_HANDLE                   User,
  IN  UINT16                                    Index,
  IN  VOID                                      *OpCodeHandle
  );

/**
  Display modify user information form

  In this form, username, create Date, usage date, usage count, identity policy,
  and access policy are displayed.

  @param[in] UserIndex       The index of the user in display list to modify.
  
**/
VOID
ModifyUserInfo (
  IN UINT8                                      UserIndex
  );

/**
  Get the username from user input and update username string in Hii 
  database with it.

**/
VOID
ModifyUserName (
  VOID
  );

/**
  Display the form of modifying user identity policy.

**/
VOID
ModifyIdentityPolicy (
  VOID
  );

/**
  Update the mUserInfo.NewIdentityPolicy and UI when 'add option' is pressed.

**/
VOID
AddIdentityPolicyItem (
  VOID
  );

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
  );

/**
  Display modify user access policy form

  In this form, access right, access setu,p and access boot order are dynamically
  added. Load devicepath and connect devicepath are displayed too.
  
**/
VOID
ModidyAccessPolicy (
  VOID
  );

/**
  Collect all the access policy data to mUserInfo.AccessPolicy, 
  and save it to user profile.

**/
VOID
SaveAccessPolicy (
  VOID
  );

/**
  Get current user's access rights.

  @param[out]  AccessRight   Points to the buffer used for user's access rights.

  @retval EFI_SUCCESS        Get current user access rights successfully.
  @retval others             Fail to get current user access rights.

**/
EFI_STATUS
GetAccessRight (
  OUT  UINT32                                    *AccessRight
  );

/**
  Display the permit load device path in the loadable device path list.

**/
VOID
DisplayLoadPermit(
  VOID
  );

/**
  Display the forbid load device path list (mAccessInfo.LoadForbid).

**/
VOID
DisplayLoadForbid (
  VOID
  );

/**
  Display the permit connect device path.

**/
VOID
DisplayConnectPermit (
  VOID
  );

/**
  Display the forbid connect device path list.

**/
VOID
DisplayConnectForbid (
  VOID
  );

/**
  Delete the specified device path by DriverIndex from the forbid device path 
  list (mAccessInfo.LoadForbid).

  @param[in]  DriverIndex   The index of driver in a forbidden device path list.
  
**/
VOID
DeleteFromForbidLoad (
  IN  UINT16                                    DriverIndex
  );
  
/**
  Add the specified device path by DriverIndex to the forbid device path 
  list (mAccessInfo.LoadForbid).

  @param[in]  DriverIndex   The index of driver saved in driver options.
  
**/
VOID
AddToForbidLoad (
  IN  UINT16                                    DriverIndex
  );

/**
  Get user name from the popup windows.
  
  @param[in, out]  UserNameLen   On entry, point to the buffer lengh of UserName.
                                 On exit, point to the input user name length.
  @param[out]      UserName      The buffer to hold the input user name.
 
  @retval EFI_ABORTED            It is given up by pressing 'ESC' key.
  @retval EFI_NOT_READY          Not a valid input at all.
  @retval EFI_SUCCESS            Get a user name successfully.

**/
EFI_STATUS
GetUserNameInput (
  IN OUT  UINTN         *UserNameLen,
     OUT  CHAR16        *UserName
  );

/**
  Find the specified info in User profile by the InfoType.

  @param[in]  User         Handle of the user whose information will be searched.
  @param[in]  InfoType     The user information type to find.
  @param[out] UserInfo     Points to user information handle found.
  
  @retval EFI_SUCCESS      Find the user information successfully.
  @retval Others           Fail to find the user information.

**/
EFI_STATUS
FindInfoByType (
  IN  EFI_USER_PROFILE_HANDLE                   User,
  IN  UINT8                                     InfoType,
  OUT EFI_USER_INFO_HANDLE                      *UserInfo
  );

/**
  Convert the identity policy to a unicode string and update the Hii database
  IpStringId string with it.

  @param[in]  Ip         Points to identity policy.
  @param[in]  IpLen      The identity policy length.
  @param[in]  IpStringId String ID in the HII database to be replaced.

**/
VOID
ResolveIdentityPolicy (
  IN  UINT8                                     *Ip,
  IN  UINTN                                     IpLen,
  IN  EFI_STRING_ID                             IpStringId
  );

/**
  Expand access policy memory size.

  @param[in] ValidLen       The valid access policy length.
  @param[in] ExpandLen      The length that is needed to expand.
    
**/
VOID
ExpandMemory (
  IN      UINTN                                 ValidLen,
  IN      UINTN                                 ExpandLen
  );

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
  );
  
#endif
