/** @file
  The header file for User identify Manager driver.
    
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _USER_IDENTIFY_MANAGER_H_
#define _USER_IDENTIFY_MANAGER_H_

#include <Uefi.h>

#include <Guid/GlobalVariable.h>
#include <Guid/MdeModuleHii.h>

#include <Protocol/FormBrowser2.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/UserCredential2.h>
#include <Protocol/UserManager.h>
#include <Protocol/DeferredImageLoad.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>

#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>

#include "UserIdentifyManagerData.h"

//
// This is the generated IFR binary data for each formset defined in VFR.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist.
//
extern UINT8                UserIdentifyManagerVfrBin[];

//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist.
//
extern UINT8                UserIdentifyManagerStrings[];

#define   USER_NUMBER_INC           32
#define   DEFAULT_PROFILE_SIZE      512
#define   INFO_PAYLOAD_SIZE         64

//
// Credential Provider Information.
//
typedef struct {
  UINTN                         Count;
  EFI_USER_CREDENTIAL2_PROTOCOL *Provider[1];
} CREDENTIAL_PROVIDER_INFO;

//
// Internal user profile entry.
//
typedef struct {
  UINTN   MaxProfileSize;
  UINTN   UserProfileSize;
  CHAR16  UserVarName[9];
  UINT8   *ProfileInfo;
} USER_PROFILE_ENTRY;

//
// Internal user profile database.
//
typedef struct {
  UINTN                   UserProfileNum;
  UINTN                   MaxProfileNum;
  EFI_USER_PROFILE_HANDLE UserProfile[1];
} USER_PROFILE_DB;

#define USER_MANAGER_SIGNATURE  SIGNATURE_32 ('U', 'I', 'M', 'S')

typedef struct {
  UINTN                           Signature;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_HANDLE                  HiiHandle;

  //
  // Consumed protocol.
  //
  EFI_HII_DATABASE_PROTOCOL       *HiiDatabase;
  EFI_HII_STRING_PROTOCOL         *HiiString;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
  EFI_FORM_BROWSER2_PROTOCOL      *FormBrowser2;

  //
  // Produced protocol.
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;
} USER_MANAGER_CALLBACK_INFO;

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH        VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  End;
} HII_VENDOR_DEVICE_PATH;

/**
  Register an event notification function for the user profile changed.

  @param[in]  ImageHandle     Image handle this driver.

**/
VOID
LoadDeferredImageInit (
  IN EFI_HANDLE        ImageHandle
  );


/**
  This function creates a new user profile with only
  a new user identifier attached and returns its handle.
  The user profile is non-volatile, but the handle User
  can change across reboots.

  @param[in]   This               Protocol EFI_USER_MANAGER_PROTOCOL instance
                                  pointer.
  @param[out]  User               Handle of a new user profile.

  @retval EFI_SUCCESS             User profile was successfully created.
  @retval EFI_ACCESS_DENIED       Current user does not have sufficient permissions
                                  to create a user profile.
  @retval EFI_UNSUPPORTED         Creation of new user profiles is not supported.
  @retval EFI_INVALID_PARAMETER   User is NULL.

**/
EFI_STATUS
EFIAPI
UserProfileCreate (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  OUT       EFI_USER_PROFILE_HANDLE             *User
  );


/**
  Delete an existing user profile.

  @param  This                    Protocol EFI_USER_MANAGER_PROTOCOL instance
                                  pointer.
  @param  User                    User profile handle.

  @retval EFI_SUCCESS             User profile was successfully deleted.
  @retval EFI_ACCESS_DENIED       Current user does not have sufficient permissions
                                  to delete a user profile or there is only one
                                  user profile.
  @retval EFI_UNSUPPORTED         Deletion of new user profiles is not supported.
  @retval EFI_INVALID_PARAMETER   User does not refer to a valid user profile.

**/
EFI_STATUS
EFIAPI
UserProfileDelete (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_USER_PROFILE_HANDLE             User
  );


/**
  Get next user profile from the user profile database.

  @param[in]       This           Protocol EFI_USER_MANAGER_PROTOCOL instance
                                  pointer.
  @param[in, out]  User           User profile handle.

  @retval EFI_SUCCESS             Next enrolled user profile successfully returned.
  @retval EFI_INVALID_PARAMETER   User is NULL.

**/
EFI_STATUS
EFIAPI
UserProfileGetNext (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN OUT    EFI_USER_PROFILE_HANDLE             *User
  );


/**
  This function returns the current user profile handle.

  @param[in]  This                Protocol EFI_USER_MANAGER_PROTOCOL instance pointer.
  @param[out]  CurrentUser        User profile handle.

  @retval EFI_SUCCESS             Current user profile handle returned successfully.
  @retval EFI_INVALID_PARAMETER   CurrentUser is NULL.

**/
EFI_STATUS
EFIAPI
UserProfileCurrent (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  OUT       EFI_USER_PROFILE_HANDLE             *CurrentUser
  );


/**
  Identify the user and, if authenticated, returns the user handle and changes
  the current user profile.

  @param  This                    Protocol EFI_USER_MANAGER_PROTOCOL instance pointer.
  @param  CurrentUser             User profile handle.

  @retval EFI_SUCCESS             User was successfully identified.
  @retval EFI_INVALID_PARAMETER   User is NULL.
  @retval EFI_ACCESS_DENIED       User was not successfully identified.

**/
EFI_STATUS
EFIAPI
UserProfileIdentify (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  OUT       EFI_USER_PROFILE_HANDLE             *User
  );


/**
  Find a user using a user information record.

  This function searches all user profiles for the specified user information record.
  The search starts with the user information record handle following UserInfo and 
  continues until either the information is found or there are no more user profiles.
  A match occurs when the Info.InfoType field matches the user information record
  type and the user information record data matches the portion of Info passed the 
  EFI_USER_INFO header.

  @param[in]      This     Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in, out] User     On entry, points to the previously returned user profile 
                           handle, or NULL to start searching with the first user profile.
                           On return, points to the user profile handle, or NULL if not
                           found.
  @param[in, out] UserInfo On entry, points to the previously returned user information
                           handle, or NULL to start searching with the first. On return, 
                           points to the user information handle of the user information
                           record, or NULL if not found. Can be NULL, in which case only 
                           one user information record per user can be returned. 
  @param[in]      Info     Points to the buffer containing the user information to be 
                           compared to the user information record. If NULL, then only 
                           the user information record type is compared. If InfoSize is 0, 
                           then the user information record must be empty.

  @param[in]      InfoSize The size of Info, in bytes. 

  @retval EFI_SUCCESS      User information was found. User points to the user profile handle,
                           and UserInfo points to the user information handle.
  @retval EFI_NOT_FOUND    User information was not found. User points to NULL and UserInfo 
                           points to NULL.
  
**/
EFI_STATUS
EFIAPI
UserProfileFind (
  IN     CONST EFI_USER_MANAGER_PROTOCOL        *This,
  IN OUT EFI_USER_PROFILE_HANDLE                *User,
  IN OUT EFI_USER_INFO_HANDLE                   *UserInfo OPTIONAL,
  IN     CONST EFI_USER_INFO                    *Info,
  IN     UINTN                                  InfoSize
  );


/**
  This function returns user information.

  @param  This                    Protocol EFI_USER_MANAGER_PROTOCOL instance
                                  pointer.
  @param  User                    Handle of the user whose profile will be
                                  retrieved.
  @param  UserInfo                Handle of the user information data record.
  @param  Info                    On entry, points to a buffer of at least
                                  *InfoSize bytes.  On exit, holds the user
                                  information.
  @param  InfoSize                On entry, points to the size of Info. On return,
                                  points to the size of the user information.

  @retval EFI_SUCCESS             Information returned successfully.
  @retval EFI_ACCESS_DENIED       The information about the specified user cannot
                                  be accessed  by the current user.
                                  EFI_BUFFER_TOO_SMALL- The number of bytes
                                  specified by *InfoSize is too small to hold the
                                  returned data.

**/
EFI_STATUS
EFIAPI
UserProfileGetInfo (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_USER_PROFILE_HANDLE             User,
  IN        EFI_USER_INFO_HANDLE                UserInfo,
  OUT       EFI_USER_INFO                       *Info,
  IN OUT    UINTN                               *InfoSize
  );


/**
  This function changes user information.

  @param  This                    Protocol EFI_USER_MANAGER_PROTOCOL instance
                                  pointer.
  @param  User                    Handle of the user whose profile will be
                                  retrieved.
  @param  UserInfo                Handle of the user information data record.
  @param  Info                    Points to the user information.
  @param  InfoSize                The size of Info, in bytes.

  @retval EFI_SUCCESS             User profile information was successfully
                                  changed/added.
  @retval EFI_ACCESS_DENIED       The record is exclusive.
  @retval EFI_SECURITY_VIOLATION  The current user does not have permission to
                                  change  the specified user profile or user
                                  information record.

**/
EFI_STATUS
EFIAPI
UserProfileSetInfo (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_USER_PROFILE_HANDLE             User,
  IN OUT    EFI_USER_INFO_HANDLE                *UserInfo,
  IN CONST  EFI_USER_INFO                       *Info,
  IN        UINTN                               InfoSize
  );


/**
  This function allows the credential provider to notify the User Identity Manager
  when user status has changed while deselected.

  @param  This                    Protocol EFI_USER_MANAGER_PROTOCOL instance
                                  pointer.
  @param  Changed                 Points to the instance of the
                                  EFI_USER_CREDENTIAL_PROTOCOL  where the user has
                                  changed.

  @retval EFI_SUCCESS             The User Identity Manager has handled the
                                  notification.
  @retval EFI_NOT_READY           The function was called while the specified
                                  credential  provider was not selected.
  @retval EFI_UNSUPPORTED         The User Identity Manager doesn't support
                                  asynchronous  notifications.

**/
EFI_STATUS
EFIAPI
UserProfileNotify (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_HANDLE                          Changed
  );


/**
  Delete the user information attached to the user profile specified by the UserInfo.

  @param  This                    Protocol EFI_USER_MANAGER_PROTOCOL instance pointer.
  @param  User                    Handle of the user whose profile will be retrieved.
  @param  UserInfo                Handle of the user information data record.

  @retval EFI_SUCCESS             User information deleted successfully.
  @retval EFI_ACCESS_DENIED       The current user does not have permission to
                                  delete this user in-formation.
  @retval EFI_NOT_FOUND           User information record UserInfo does not exist
                                  in the user pro-file.

**/
EFI_STATUS
EFIAPI
UserProfileDeleteInfo (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_USER_PROFILE_HANDLE             User,
  IN        EFI_USER_INFO_HANDLE                UserInfo
  );


/**
  This function returns the next user information record.

  @param  This                    Protocol EFI_USER_MANAGER_PROTOCOL instance pointer.
  @param  User                    Handle of the user whose profile will be retrieved.
  @param  UserInfo                Handle of the user information data record.

  @retval EFI_SUCCESS             User information returned.
  @retval EFI_NOT_FOUND           No more user information found.

**/
EFI_STATUS
EFIAPI
UserProfileGetNextInfo (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_USER_PROFILE_HANDLE             User,
  IN OUT    EFI_USER_INFO_HANDLE                *UserInfo
  );
  
#endif
