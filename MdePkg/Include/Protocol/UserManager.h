/** @file
  UEFI 2.2 User Manager Protocol definition.

  This protocol manages user profiles.

  Copyright (c) 2009, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __USER_MANAGER_H__
#define __USER_MANAGER_H__

#include <Protocol/UserCredential.h>

///
/// Global ID for the User Manager Protocol
///
#define EFI_USER_MANAGER_PROTOCOL_GUID \
  { \
    0x6fd5b00c, 0xd426, 0x4283, { 0x98, 0x87, 0x6c, 0xf5, 0xcf, 0x1c, 0xb1, 0xfe } \
  }

#define EFI_EVENT_GROUP_USER_PROFILE_CHANGED \
  { \
    0xbaf1e6de, 0x209e, 0x4adb, { 0x8d, 0x96, 0xfd, 0x8b, 0x71, 0xf3, 0xf6, 0x83 } \
  }

typedef struct _EFI_USER_MANAGER_PROTOCOL  EFI_USER_MANAGER_PROTOCOL;

/**
  Create a new user profile.

  This function creates a new user profile with only a new user identifier attached and returns its 
  handle. The user profile is non-volatile, but the handle User can change across reboots.

  @param[in]  This               Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[out] User               On return, points to the new user profile handle. 
                                 The user profile handle is unique only during this boot.
 
  @retval EFI_SUCCESS            User profile was successfully created.
  @retval EFI_ACCESS_DENIED      Current user does not have sufficient permissions to create a user profile.
  @retval EFI_UNSUPPORTED        Creation of new user profiles is not supported.
  @retval EFI_INVALID_PARAMETER  The User parameter is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_CREATE)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  OUT      EFI_USER_PROFILE_HANDLE    *User
  );

/**
  Delete an existing user profile.

  @param[in] This                Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in] User                User profile handle. 

  @retval EFI_SUCCESS            User profile was successfully deleted.
  @retval EFI_ACCESS_DENIED      Current user does not have sufficient permissions to delete a user
                                 profile or there is only one user profile.
  @retval EFI_UNSUPPORTED        Deletion of new user profiles is not supported.
  @retval EFI_INVALID_PARAMETER  User does not refer to a valid user profile.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_DELETE)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  IN       EFI_USER_PROFILE_HANDLE    User
  );

/**
  Enumerate all of the enrolled users on the platform.

  This function returns the next enrolled user profile. To retrieve the first user profile handle, point 
  User at a NULL. Each subsequent call will retrieve another user profile handle until there are no 
  more, at which point User will point to NULL. 

  @param[in]     This            Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in,out] User            On entry, points to the previous user profile handle or NULL to 
                                 start enumeration. On exit, points to the next user profile handle
                                 or NULL if there are no more user profiles.

  @retval EFI_SUCCESS            Next enrolled user profile successfully returned. 
  @retval EFI_ACCESS_DENIED      Next enrolled user profile was not successfully returned.
  @retval EFI_INVALID_PARAMETER  The User parameter is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_GET_NEXT)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  IN OUT   EFI_USER_PROFILE_HANDLE    *User
  );

/**
  Return the current user profile handle.

  @param[in]  This               Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[out] CurrentUser        On return, points to the current user profile handle.

  @retval EFI_SUCCESS            Current user profile handle returned successfully. 
  @retval EFI_INVALID_PARAMETER  The CurrentUser parameter is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_CURRENT)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  OUT      EFI_USER_PROFILE_HANDLE    *CurrentUser
  );

/**
  Identify a user.

  Identify the user and, if authenticated, returns the user handle and changes the current user profile.
  All user information marked as private in a previously selected profile is no longer available for 
  inspection. 
  Whenever the current user profile is changed then the an event with the GUID 
  EFI_EVENT_GROUP_USER_PROFILE_CHANGED is signaled.

  @param[in]  This               Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[out] User               On return, points to the user profile handle for the current user profile.

  @retval EFI_SUCCESS            User was successfully identified.
  @retval EFI_ACCESS_DENIED      User was not successfully identified.
  @retval EFI_INVALID_PARAMETER  The User parameter is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_IDENTIFY)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  OUT      EFI_USER_PROFILE_HANDLE    *User
  );

/**
  Find a user using a user information record.

  This function searches all user profiles for the specified user information record. The search starts 
  with the user information record handle following UserInfo and continues until either the 
  information is found or there are no more user profiles.
  A match occurs when the Info.InfoType field matches the user information record type and the 
  user information record data matches the portion of Info passed the EFI_USER_INFO header.

  @param[in]     This      Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in,out] User      On entry, points to the previously returned user profile handle or NULL to start 
                           searching with the first user profile. On return, points to the user profile handle or 
                           NULL if not found.
  @param[in,out] UserInfo  On entry, points to the previously returned user information handle or NULL to start 
                           searching with the first. On return, points to the user information handle of the user 
                           information record or NULL if not found. Can be NULL, in which case only one user 
                           information record per user can be returned. 
  @param[in]     Info      Points to the buffer containing the user information to be compared to the user 
                           information record. If NULL, then only the user information record type is compared. 
                           If InfoSize is 0, then the user information record must be empty.

  @param[in]     InfoSize  The size of Info, in bytes. 

  @retval EFI_SUCCESS      User information was found. User points to the user profile handle and 
                           UserInfo points to the user information handle.
  @retval EFI_NOT_FOUND    User information was not found. User points to NULL and UserInfo points to NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_FIND)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  IN OUT   EFI_USER_PROFILE_HANDLE    *User,
  IN OUT   EFI_USER_INFO_HANDLE       *UserInfo OPTIONAL,
  IN CONST EFI_USER_INFO              *Info,
  IN       UINTN                      InfoSize
  );

/**
  Called by credential provider to notify of information change.

  This function allows the credential provider to notify the User Identity Manager when user status has 
  changed while deselected.
  If the User Identity Manager doesn't support asynchronous changes in credentials, then this function 
  should return EFI_UNSUPPORTED. 
  If the User Identity Manager supports this, it will call User() to get the user identifier and then 
  GetNextInfo() and GetInfo() in the User Credential Protocol to get all of the information 
  from the credential and add it.

  @param[in] This          Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in] Changed       Handle on which is installed an instance of the
                           EFI_USER_CREDENTIAL_PROTOCOL where the user has changed.

  @retval EFI_SUCCESS      The User Identity Manager has handled the notification.
  @retval EFI_NOT_READY    The function was called while the specified credential provider was not selected.
  @retval EFI_UNSUPPORTED  The User Identity Manager doesn't support asynchronous notifications.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_NOTIFY)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  IN       EFI_HANDLE                 Changed
  );

/**
  Return information attached to the user.

  This function returns user information. The format of the information is described in User 
  Information. The function may return EFI_ACCESS_DENIED if the information is marked private 
  and the handle specified by User is not the current user profile. The function may return 
  EFI_ACCESS_DENIED if the information is marked protected and the information is associated 
  with a credential provider for which the user has not been authenticated.

  @param[in]     This           Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in]     User           Handle of the user whose profile will be retrieved. 
  @param[in]     UserInfo       Handle of the user information data record.   
  @param[out]    Info           On entry, points to a buffer of at least *InfoSize bytes. On exit, holds the user 
                                information. If the buffer is too small to hold the information, then 
                                EFI_BUFFER_TOO_SMALL is returned and InfoSize is updated to contain the 
                                number of bytes actually required. 
  @param[in,out] InfoSize       On entry, points to the size of Info. On return, points to the size of the user 
                                information. 

  @retval EFI_SUCCESS           Information returned successfully.
  @retval EFI_ACCESS_DENIED     The information about the specified user cannot be accessed by the current user.
  @retval EFI_BUFFER_TOO_SMALL  The number of bytes specified by *InfoSize is too small to hold 
                                the returned data. The actual size required is returned in *InfoSize.
**/  
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_GET_INFO)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  IN       EFI_USER_PROFILE_HANDLE    User,
  IN       EFI_USER_INFO_HANDLE       UserInfo,
  OUT      EFI_USER_INFO              *Info,
  IN OUT   UINTN                      *InfoSize
  );

/**
  Add or update user information.

  This function changes user information.  If NULL is pointed to by UserInfo, then a new user 
  information record is created and its handle is returned in UserInfo. Otherwise, the existing one is 
  replaced.
  If EFI_USER_INFO_EXCLUSIVE is specified in Info and a user information record of the same 
  type already exists in the user profile, then EFI_ACCESS_DENIED will be returned and 
  UserInfo will point to the handle of the existing record.

  @param[in]     This             Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in]     User             Handle of the user whose profile will be retrieved. 
  @param[in,out] UserInfo         Handle of the user information data record.   
  @param[in]     Info             On entry, points to a buffer of at least *InfoSize bytes. On exit, holds the user 
                                  information. If the buffer is too small to hold the information, then 
                                  EFI_BUFFER_TOO_SMALL is returned and InfoSize is updated to contain the 
                                  number of bytes actually required. 
  @param[in]     InfoSize         On entry, points to the size of Info. On return, points to the size of the user 
                                  information. 

  @retval EFI_SUCCESS             Information returned successfully.
  @retval EFI_ACCESS_DENIED       The record is exclusive.
  @retval EFI_SECURITY_VIOLATION  The current user does not have permission to change the specified 
                                  user profile or user information record.
**/  
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_SET_INFO)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  IN       EFI_USER_PROFILE_HANDLE    User,
  IN OUT   EFI_USER_INFO_HANDLE       *UserInfo,
  IN CONST EFI_USER_INFO              *Info,
  IN       UINTN                      InfoSize
  );

/**
  Delete user information.

  Delete the user information attached to the user profile specified by the UserInfo.

  @param[in] This            Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in] User            Handle of the user whose information will be deleted.
  @param[in] UserInfo        Handle of the user information to remove.

  @retval EFI_SUCCESS        User information deleted successfully.
  @retval EFI_NOT_FOUND      User information record UserInfo does not exist in the user profile.
  @retval EFI_ACCESS_DENIED  The current user does not have permission to delete this user information. 
**/ 
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_DELETE_INFO)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  IN       EFI_USER_PROFILE_HANDLE    User,
  IN       EFI_USER_INFO_HANDLE       UserInfo
  );

/**
  Enumerate user information of all the enrolled users on the platform.

  This function returns the next user information record. To retrieve the first user information record 
  handle, point UserInfo at a NULL. Each subsequent call will retrieve another user information 
  record handle until there are no more, at which point UserInfo will point to NULL. 

  Note: in-consistency between code and the UEFI 2.3 specification that the type of the User parameter
  is EFI_USER_PROFILE_HANDLE. It should be spec error and wait for spec update.

  @param[in]     This      Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in]     User      Handle of the user whose information will be deleted.
  @param[in,out] UserInfo  Handle of the user information to remove.

  @retval EFI_SUCCESS      User information returned.
  @retval EFI_NOT_FOUND    No more user information found.
**/ 
typedef
EFI_STATUS
(EFIAPI *EFI_USER_PROFILE_GET_NEXT_INFO)(
  IN CONST EFI_USER_MANAGER_PROTOCOL  *This,
  IN       EFI_USER_PROFILE_HANDLE    User,
  IN OUT   EFI_USER_INFO_HANDLE       *UserInfo
  );

///
/// This protocol provides the services used to manage user profiles.
///
struct _EFI_USER_MANAGER_PROTOCOL {
  EFI_USER_PROFILE_CREATE         Create;
  EFI_USER_PROFILE_DELETE         Delete;
  EFI_USER_PROFILE_GET_NEXT       GetNext;
  EFI_USER_PROFILE_CURRENT        Current;
  EFI_USER_PROFILE_IDENTIFY       Identify;
  EFI_USER_PROFILE_FIND           Find;
  EFI_USER_PROFILE_NOTIFY         Notify;
  EFI_USER_PROFILE_GET_INFO       GetInfo;
  EFI_USER_PROFILE_SET_INFO       SetInfo;
  EFI_USER_PROFILE_DELETE_INFO    DeleteInfo;
  EFI_USER_PROFILE_GET_NEXT_INFO  GetNextInfo;
};

extern EFI_GUID gEfiUserManagerProtocolGuid;
extern EFI_GUID gEfiEventUserProfileChangedGuid;

#endif
