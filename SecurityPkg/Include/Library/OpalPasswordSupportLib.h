/** @file
  Header file of Opal password support library.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _OPAL_PASSWORD_SUPPORT_LIB_H_
#define _OPAL_PASSWORD_SUPPORT_LIB_H_

#include <Protocol/DevicePath.h>
#include <Library/TcgStorageOpalLib.h>


#pragma pack(1)

//
// Structure that is used to represent the available actions for an OpalDisk.
// The data can then be utilized to expose/hide certain actions available to an end user
// by the consumer of this library.
//
typedef struct {
    //
    // Indicates if the disk can support PSID Revert action.  should verify disk supports PSID authority
    //
    UINT16 PsidRevert : 1;

    //
    // Indicates if the disk can support Revert action
    //
    UINT16 Revert : 1;

    //
    // Indicates if the user must keep data for revert action.  It is true if no media encryption is supported.
    //
    UINT16 RevertKeepDataForced : 1;

    //
    // Indicates if the disk can support set Admin password
    //
    UINT16 AdminPass : 1;

    //
    // Indicates if the disk can support set User password.  This action requires that a user
    // password is first enabled.
    //
    UINT16 UserPass : 1;

    //
    // Indicates if unlock action is available.  Requires disk to be currently locked.
    //
    UINT16 Unlock : 1;

    //
    // Indicates if Secure Erase action is available.  Action requires admin credentials and media encryption support.
    //
    UINT16 SecureErase : 1;

    //
    // Indicates if Disable User action is available.  Action requires admin credentials.
    //
    UINT16 DisableUser : 1;
} OPAL_DISK_ACTIONS;

//
// Structure that is used to represent the Opal device with password info.
//
typedef struct {
  LIST_ENTRY                 Link;

  UINT8                      Password[32];
  UINT8                      PasswordLength;

  EFI_DEVICE_PATH_PROTOCOL   OpalDevicePath;
} OPAL_DISK_AND_PASSWORD_INFO;

#pragma pack()

/**

  The function performs determines the available actions for the OPAL_DISK provided.

  @param[in]   SupportedAttributes   The support attribute for the device.
  @param[in]   LockingFeature        The locking status for the device.
  @param[in]   OwnerShip             The ownership for the device.
  @param[out]  AvalDiskActions       Pointer to fill-out with appropriate disk actions.

**/
TCG_RESULT
EFIAPI
OpalSupportGetAvailableActions(
  IN  OPAL_DISK_SUPPORT_ATTRIBUTE      *SupportedAttributes,
  IN  TCG_LOCKING_FEATURE_DESCRIPTOR   *LockingFeature,
  IN  UINT16                           OwnerShip,
  OUT OPAL_DISK_ACTIONS                *AvalDiskActions
  );

/**
  Enable Opal Feature for the input device.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      Msid               Msid
  @param[in]      MsidLength         Msid Length
  @param[in]      Password           Admin password
  @param[in]      PassLength         Length of password in bytes
  @param[in]      DevicePath         The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportEnableOpalFeature(
  IN OPAL_SESSION              *Session,
  IN VOID                      *Msid,
  IN UINT32                    MsidLength,
  IN VOID                      *Password,
  IN UINT32                    PassLength,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Creates a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY, then reverts device using Admin SP Revert method.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      Psid               PSID of device to revert.
  @param[in]      PsidLength         Length of PSID in bytes.
  @param[in]      DevicePath         The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportPsidRevert(
  IN OPAL_SESSION              *Session,
  IN VOID                      *Psid,
  IN UINT32                    PsidLength,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Opens a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY, then reverts the device using the RevertSP method.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      KeepUserData       TRUE to keep existing Data on the disk, or FALSE to erase it
  @param[in]      Password           Admin password
  @param[in]      PasswordLength     Length of password in bytes
  @param[in]      Msid               Msid
  @param[in]      MsidLength         Msid Length
  @param[out]     PasswordFailed     indicates if password failed (start session didn't work)
  @param[in]      DevicePath         The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportRevert(
  IN  OPAL_SESSION              *Session,
  IN  BOOLEAN                   KeepUserData,
  IN  VOID                      *Password,
  IN  UINT32                    PasswordLength,
  IN  VOID                      *Msid,
  IN  UINT32                    MsidLength,
  OUT BOOLEAN                   *PasswordFailed,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Set new password.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      OldPassword        Current admin password
  @param[in]      OldPasswordLength  Length of current admin password in bytes
  @param[in]      NewPassword        New admin password to set
  @param[in]      NewPasswordLength  Length of new password in bytes
  @param[in]      DevicePath         The device path for the opal devcie.
  @param[in]      SetAdmin           Whether set admin password or user password.
                                     TRUE for admin, FALSE for user.

**/
TCG_RESULT
EFIAPI
OpalSupportSetPassword(
  IN OPAL_SESSION              *Session,
  IN VOID                      *OldPassword,
  IN UINT32                    OldPasswordLength,
  IN VOID                      *NewPassword,
  IN UINT32                    NewPasswordLength,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN BOOLEAN                   SetAdmin
  );

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY and disables the User1 authority.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      Password           Admin password
  @param[in]      PasswordLength     Length of password in bytes
  @param[out]     PasswordFailed     Indicates if password failed (start session didn't work)
  @param[in]      DevicePath         The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportDisableUser(
  IN  OPAL_SESSION              *Session,
  IN  VOID                      *Password,
  IN  UINT32                    PasswordLength,
  OUT BOOLEAN                   *PasswordFailed,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_USER1_AUTHORITY or OPAL_LOCKING_SP_ADMIN1_AUTHORITY
  and updates the global locking range ReadLocked and WriteLocked columns to FALSE.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      Password           Admin or user password
  @param[in]      PasswordLength     Length of password in bytes
  @param[in]      DevicePath         The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportUnlock(
  IN OPAL_SESSION               *Session,
  IN VOID                       *Password,
  IN UINT32                     PasswordLength,
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  );

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_USER1_AUTHORITY or OPAL_LOCKING_SP_ADMIN1_AUTHORITY
  and updates the global locking range ReadLocked and WriteLocked columns to TRUE.

  @param[in]      Session             The opal session for the opal device.
  @param[in]      Password            Admin or user password
  @param[in]      PasswordLength      Length of password in bytes
  @param[in]      DevicePath          The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportLock(
  IN OPAL_SESSION               *Session,
  IN VOID                       *Password,
  IN UINT32                     PasswordLength,
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  );

/**
  Check if the password is full zero.

  @param[in]   Password       Points to the Data Buffer

  @retval      TRUE           This password string is full zero.
  @retval      FALSE          This password string is not full zero.

**/
LIST_ENTRY *
EFIAPI
OpalSupportGetOpalDeviceList (
  VOID
  );

/**
  Transfer the password to the smm driver.

  @param[in]  DevicePath     The device path for the opal devcie.
  @param      PasswordLen    The input password length.
  @param      Password       Input password buffer.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Error occured.

**/
EFI_STATUS
EFIAPI
OpalSupportSendPasword(
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  UINTN                       PasswordLen,
  VOID                        *Password
  );

#endif // _OPAL_CORE_H_
