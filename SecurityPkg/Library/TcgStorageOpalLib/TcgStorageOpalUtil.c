/** @file
  Public API for Opal Core library.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TcgStorageOpalLib.h>
#include "TcgStorageOpalLibInternal.h"

#define OPAL_MSID_LENGTH  128

/**
  Creates a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY, then reverts device using Admin SP Revert method.

  @param[in]      Session,           The session info for one opal device.
  @param[in]      Psid               PSID of device to revert.
  @param[in]      PsidLength         Length of PSID in bytes.

**/
TCG_RESULT
EFIAPI
OpalUtilPsidRevert (
  OPAL_SESSION  *Session,
  const VOID    *Psid,
  UINT32        PsidLength
  )
{
  UINT8       MethodStatus;
  TCG_RESULT  Ret;
  UINT32      RemovalTimeOut;

  NULL_CHECK (Session);
  NULL_CHECK (Psid);

  RemovalTimeOut = GetRevertTimeOut (Session);
  DEBUG ((DEBUG_INFO, "OpalUtilPsidRevert: Timeout value = %d\n", RemovalTimeOut));

  Ret = OpalStartSession (
          Session,
          OPAL_UID_ADMIN_SP,
          TRUE,
          PsidLength,
          Psid,
          OPAL_ADMIN_SP_PSID_AUTHORITY,
          &MethodStatus
          );
  if ((Ret == TcgResultSuccess) && (MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS)) {
    Ret = OpalPyrite2PsidRevert (Session, RemovalTimeOut);
    if (Ret != TcgResultSuccess) {
      //
      // If revert was successful, session was already ended by TPer, so only end session on failure
      //
      OpalEndSession (Session);
    }
  }

  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    Ret = TcgResultFailure;
  }

  return Ret;
}

/**
  Opens a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_SID_AUTHORITY,
  sets the OPAL_UID_ADMIN_SP_C_PIN_SID column with the new password,
  and activates the locking SP to copy SID PIN to Admin1 Locking SP PIN

  @param[in]      Session,           The session info for one opal device.
  @param[in]      GeneratedSid       Generated SID of disk
  @param[in]      SidLength          Length of generatedSid in bytes
  @param[in]      Password           New admin password to set
  @param[in]      PassLength         Length of password in bytes

**/
TCG_RESULT
EFIAPI
OpalUtilSetAdminPasswordAsSid (
  OPAL_SESSION  *Session,
  const VOID    *GeneratedSid,
  UINT32        SidLength,
  const VOID    *Password,
  UINT32        PassLength
  )
{
  UINT8       MethodStatus;
  TCG_RESULT  Ret;

  NULL_CHECK (Session);
  NULL_CHECK (GeneratedSid);
  NULL_CHECK (Password);

  Ret = OpalStartSession (
          Session,
          OPAL_UID_ADMIN_SP,
          TRUE,
          SidLength,
          GeneratedSid,
          OPAL_ADMIN_SP_SID_AUTHORITY,
          &MethodStatus
          );
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "start session with admin SP as SID authority failed: Ret=%d MethodStatus=%u\n", Ret, MethodStatus));
    goto done;
  }

  //
  // 1. Update SID = new Password
  //
  Ret = OpalSetPassword (
          Session,
          OPAL_UID_ADMIN_SP_C_PIN_SID,
          Password,
          PassLength,
          &MethodStatus
          );

  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    OpalEndSession (Session);
    DEBUG ((DEBUG_INFO, "set Password failed: Ret=%d MethodStatus=%u\n", Ret, MethodStatus));
    goto done;
  }

  //
  // 2. Activate locking SP
  //
  Ret = OpalActivateLockingSp (Session, &MethodStatus);
  OpalEndSession (Session);
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "activate locking SP failed: Ret=%d MethodStatus=%u\n", Ret, MethodStatus));
    goto done;
  }

done:
  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    Ret = TcgResultFailure;
  }

  return Ret;
}

/**

  Opens a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY,
  and updates the specified locking range with the provided column values

  @param[in]      Session,               The session info for one opal device.
  @param[in]      Password           New admin password to set
  @param[in]      PassLength         Length of password in bytes
  @param[in]      LockingRangeUid    Locking range UID to set values
  @param[in]      RangeStart         Value to set RangeStart column for Locking Range
  @param[in]      RangeLength        Value to set RangeLength column for Locking Range
  @param[in]      ReadLockEnabled    Value to set readLockEnabled column for Locking Range
  @param[in]      WriteLockEnabled   Value to set writeLockEnabled column for Locking Range
  @param[in]      ReadLocked         Value to set ReadLocked column for Locking Range
  @param[in]      WriteLocked        Value to set WriteLocked column for Locking Range

**/
TCG_RESULT
EFIAPI
OpalUtilSetOpalLockingRange (
  OPAL_SESSION  *Session,
  const VOID    *Password,
  UINT32        PassLength,
  TCG_UID       LockingRangeUid,
  UINT64        RangeStart,
  UINT64        RangeLength,
  BOOLEAN       ReadLockEnabled,
  BOOLEAN       WriteLockEnabled,
  BOOLEAN       ReadLocked,
  BOOLEAN       WriteLocked
  )
{
  UINT8       MethodStatus;
  TCG_RESULT  Ret;

  NULL_CHECK (Session);
  NULL_CHECK (Password);

  //
  // Start session with Locking SP using current admin Password
  //
  Ret = OpalStartSession (
          Session,
          OPAL_UID_LOCKING_SP,
          TRUE,
          PassLength,
          Password,
          OPAL_LOCKING_SP_ADMIN1_AUTHORITY,
          &MethodStatus
          );
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "start session with locking SP failed: Ret=%d MethodStatus=%u\n", Ret, MethodStatus));
    goto done;
  }

  //
  // Enable locking range
  //
  Ret = OpalSetLockingRange (
          Session,
          LockingRangeUid,
          RangeStart,
          RangeLength,
          ReadLockEnabled,
          WriteLockEnabled,
          ReadLocked,
          WriteLocked,
          &MethodStatus
          );

  OpalEndSession (Session);
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "set locking range failed: Ret=%d MethodStatus=0x%x\n", Ret, MethodStatus));
  }

done:
  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    Ret = TcgResultFailure;
  }

  return Ret;
}

/**
  Opens a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_SID_AUTHORITY,
  sets OPAL_UID_ADMIN_SP_C_PIN_SID with the new password,
  and sets OPAL_LOCKING_SP_C_PIN_ADMIN1 with the new password.

  @param[in]      Session,               The session info for one opal device.
  @param[in]      OldPassword        Current admin password
  @param[in]      OldPasswordLength  Length of current admin password in bytes
  @param[in]      NewPassword        New admin password to set
  @param[in]      NewPasswordLength  Length of new password in bytes

**/
TCG_RESULT
EFIAPI
OpalUtilSetAdminPassword (
  OPAL_SESSION  *Session,
  const VOID    *OldPassword,
  UINT32        OldPasswordLength,
  const VOID    *NewPassword,
  UINT32        NewPasswordLength
  )
{
  TCG_RESULT  Ret;
  UINT8       MethodStatus;

  NULL_CHECK (Session);
  NULL_CHECK (OldPassword);
  NULL_CHECK (NewPassword);

  //
  // Unknown ownership
  //
  Ret = OpalStartSession (
          Session,
          OPAL_UID_ADMIN_SP,
          TRUE,
          OldPasswordLength,
          OldPassword,
          OPAL_ADMIN_SP_SID_AUTHORITY,
          &MethodStatus
          );
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "start session with admin SP using old Password failed\n"));
    goto done;
  }

  //
  // Update SID = new pw
  //
  Ret = OpalSetPassword (Session, OPAL_UID_ADMIN_SP_C_PIN_SID, NewPassword, NewPasswordLength, &MethodStatus);
  OpalEndSession (Session);
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "set new admin SP Password failed\n"));
    goto done;
  }

  Ret = OpalStartSession (
          Session,
          OPAL_UID_LOCKING_SP,
          TRUE,
          OldPasswordLength,
          OldPassword,
          OPAL_LOCKING_SP_ADMIN1_AUTHORITY,
          &MethodStatus
          );
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "start session with locking SP using old Password failed\n"));
    goto done;
  }

  //
  // Update admin locking SP to new pw
  //
  Ret = OpalSetPassword (Session, OPAL_LOCKING_SP_C_PIN_ADMIN1, NewPassword, NewPasswordLength, &MethodStatus);
  OpalEndSession (Session);
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "set new locking SP Password failed\n"));
    goto done;
  }

done:
  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    Ret = TcgResultFailure;
  }

  return Ret;
}

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_USER1_AUTHORITY or OPAL_LOCKING_SP_ADMIN1_AUTHORITY
  and sets the User1 SP authority to enabled and sets the User1 password.

  @param[in]      Session,               The session info for one opal device.
  @param[in]      OldPassword        Current admin password
  @param[in]      OldPasswordLength  Length of current admin password in bytes
  @param[in]      NewPassword        New admin password to set
  @param[in]      NewPasswordLength  Length of new password in bytes

**/
TCG_RESULT
EFIAPI
OpalUtilSetUserPassword (
  OPAL_SESSION  *Session,
  const VOID    *OldPassword,
  UINT32        OldPasswordLength,
  const VOID    *NewPassword,
  UINT32        NewPasswordLength
  )
{
  UINT8       MethodStatus;
  TCG_RESULT  Ret;

  NULL_CHECK (Session);
  NULL_CHECK (OldPassword);
  NULL_CHECK (NewPassword);

  //
  // See if updating user1 authority
  //
  Ret = OpalStartSession (
          Session,
          OPAL_UID_LOCKING_SP,
          TRUE,
          OldPasswordLength,
          OldPassword,
          OPAL_LOCKING_SP_USER1_AUTHORITY,
          &MethodStatus
          );
  if ((Ret == TcgResultSuccess) && (MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS)) {
    Ret = OpalSetPassword (
            Session,
            OPAL_LOCKING_SP_C_PIN_USER1,
            NewPassword,
            NewPasswordLength,
            &MethodStatus
            );
    OpalEndSession (Session);
    if ((Ret == TcgResultSuccess) && (MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS)) {
      return Ret;
    }
  }

  //
  // Setting Password for first time or setting Password as admin
  //

  //
  // Start session with Locking SP using current admin Password
  //
  Ret = OpalStartSession (
          Session,
          OPAL_UID_LOCKING_SP,
          TRUE,
          OldPasswordLength,
          OldPassword,
          OPAL_LOCKING_SP_ADMIN1_AUTHORITY,
          &MethodStatus
          );
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "StartSession with locking SP as admin1 authority failed\n"));
    goto done;
  }

  //
  // Enable User1 and set its PIN
  //
  Ret = OpalSetLockingSpAuthorityEnabledAndPin (
          Session,
          OPAL_LOCKING_SP_C_PIN_USER1,
          OPAL_LOCKING_SP_USER1_AUTHORITY,
          NewPassword,
          NewPasswordLength,
          &MethodStatus
          );
  OpalEndSession (Session);
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "OpalSetLockingSpAuthorityEnabledAndPin failed\n"));
    goto done;
  }

done:
  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    Ret = TcgResultFailure;
  }

  return Ret;
}

/**
  Verify whether user input the correct password.

  @param[in]      Session,               The session info for one opal device.
  @param[in]      Password                    Admin password
  @param[in]      PasswordLength              Length of password in bytes
  @param[in/out]  HostSigningAuthority        Use the Host signing authority type.

**/
TCG_RESULT
EFIAPI
OpalUtilVerifyPassword (
  OPAL_SESSION  *Session,
  const VOID    *Password,
  UINT32        PasswordLength,
  TCG_UID       HostSigningAuthority
  )
{
  TCG_RESULT  Ret;
  UINT8       MethodStatus;

  NULL_CHECK (Session);
  NULL_CHECK (Password);

  Ret = OpalStartSession (
          Session,
          OPAL_UID_LOCKING_SP,
          TRUE,
          PasswordLength,
          Password,
          HostSigningAuthority,
          &MethodStatus
          );
  if ((Ret == TcgResultSuccess) && (MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS)) {
    OpalEndSession (Session);
    return TcgResultSuccess;
  }

  return TcgResultFailure;
}

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_USER1_AUTHORITY or OPAL_LOCKING_SP_ADMIN1_AUTHORITY
  and generates a new global locking range key to erase the Data.

  @param[in]      Session,               The session info for one opal device.
  @param[in]      Password                   Admin or user password
  @param[in]      PasswordLength         Length of password in bytes
  @param[in/out]  PasswordFailed       indicates if password failed (start session didn't work)

**/
TCG_RESULT
EFIAPI
OpalUtilSecureErase (
  OPAL_SESSION  *Session,
  const VOID    *Password,
  UINT32        PasswordLength,
  BOOLEAN       *PasswordFailed
  )
{
  UINT8       MethodStatus;
  TCG_RESULT  Ret;

  NULL_CHECK (Session);
  NULL_CHECK (Password);
  NULL_CHECK (PasswordFailed);

  //
  // Try to generate a new key with admin1
  //
  Ret = OpalStartSession (
          Session,
          OPAL_UID_LOCKING_SP,
          TRUE,
          PasswordLength,
          Password,
          OPAL_LOCKING_SP_ADMIN1_AUTHORITY,
          &MethodStatus
          );

  if ((Ret == TcgResultSuccess) && (MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS)) {
    Ret             = OpalGlobalLockingRangeGenKey (Session, &MethodStatus);
    *PasswordFailed = FALSE;
    OpalEndSession (Session);
  } else {
    //
    // Try to generate a new key with user1
    //
    Ret = OpalStartSession (
            Session,
            OPAL_UID_LOCKING_SP,
            TRUE,
            PasswordLength,
            Password,
            OPAL_LOCKING_SP_USER1_AUTHORITY,
            &MethodStatus
            );

    if ((Ret == TcgResultSuccess) && (MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS)) {
      Ret             = OpalGlobalLockingRangeGenKey (Session, &MethodStatus);
      *PasswordFailed = FALSE;
      OpalEndSession (Session);
    } else {
      *PasswordFailed = TRUE;
    }
  }

  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    Ret = TcgResultFailure;
  }

  return Ret;
}

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY and disables the User1 authority.

  @param[in]      Session,               The session info for one opal device.
  @param[in]      Password               Admin password
  @param[in]      PasswordLength         Length of password in bytes
  @param[in/out]  PasswordFailed         indicates if password failed (start session didn't work)

**/
TCG_RESULT
EFIAPI
OpalUtilDisableUser (
  OPAL_SESSION  *Session,
  const VOID    *Password,
  UINT32        PasswordLength,
  BOOLEAN       *PasswordFailed
  )
{
  UINT8       MethodStatus;
  TCG_RESULT  Ret;

  NULL_CHECK (Session);
  NULL_CHECK (Password);
  NULL_CHECK (PasswordFailed);

  //
  // Start session with Locking SP using current admin Password
  //
  Ret = OpalStartSession (
          Session,
          OPAL_UID_LOCKING_SP,
          TRUE,
          PasswordLength,
          Password,
          OPAL_LOCKING_SP_ADMIN1_AUTHORITY,
          &MethodStatus
          );
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "StartSession with Locking SP as Admin1 failed\n"));
    *PasswordFailed = TRUE;
    goto done;
  }

  *PasswordFailed = FALSE;
  Ret             = OpalDisableUser (Session, &MethodStatus);
  OpalEndSession (Session);

done:
  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    Ret = TcgResultFailure;
  }

  return Ret;
}

/**
  Opens a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY, then reverts the device using the RevertSP method.

  @param[in]      Session,           The session info for one opal device.
  @param[in]      KeepUserData       TRUE to keep existing Data on the disk, or FALSE to erase it
  @param[in]      Password           Admin password
  @param[in]      PasswordLength     Length of password in bytes
  @param[in/out]  PasswordFailed     indicates if password failed (start session didn't work)
  @param[in]      Msid               Msid info.
  @param[in]      MsidLength         Msid data length.

**/
TCG_RESULT
EFIAPI
OpalUtilRevert (
  OPAL_SESSION  *Session,
  BOOLEAN       KeepUserData,
  const VOID    *Password,
  UINT32        PasswordLength,
  BOOLEAN       *PasswordFailed,
  UINT8         *Msid,
  UINT32        MsidLength
  )
{
  UINT8       MethodStatus;
  TCG_RESULT  Ret;
  UINT32      RemovalTimeOut;

  NULL_CHECK (Session);
  NULL_CHECK (Msid);
  NULL_CHECK (Password);
  NULL_CHECK (PasswordFailed);

  RemovalTimeOut = GetRevertTimeOut (Session);
  DEBUG ((DEBUG_INFO, "OpalUtilRevert: Timeout value = %d\n", RemovalTimeOut));

  Ret = OpalStartSession (
          Session,
          OPAL_UID_LOCKING_SP,
          TRUE,
          PasswordLength,
          Password,
          OPAL_LOCKING_SP_ADMIN1_AUTHORITY,
          &MethodStatus
          );

  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "error starting session: Ret=%d, MethodStatus=%u\n", Ret, MethodStatus));
    *PasswordFailed = TRUE;
    goto done;
  }

  *PasswordFailed = FALSE;
  //
  // Try to revert with admin1
  //
  Ret = OpalPyrite2AdminRevert (Session, KeepUserData, &MethodStatus, RemovalTimeOut);
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    //
    // Device ends the session on successful revert, so only call OpalEndSession when fail.
    //
    DEBUG ((DEBUG_INFO, "OpalAdminRevert as admin failed\n"));
    OpalEndSession (Session);
  }

  Ret = OpalUtilSetSIDtoMSID (Session, Password, PasswordLength, Msid, MsidLength);

done:
  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    Ret = TcgResultFailure;
  }

  return Ret;
}

/**
  After revert success, set SID to MSID.

  @param          Session,           The session info for one opal device.
  @param          Password,          Input password info.
  @param          PasswordLength,    Input password length.
  @param          Msid               Msid info.
  @param          MsidLength         Msid data length.

**/
TCG_RESULT
EFIAPI
OpalUtilSetSIDtoMSID (
  OPAL_SESSION  *Session,
  const VOID    *Password,
  UINT32        PasswordLength,
  UINT8         *Msid,
  UINT32        MsidLength
  )
{
  TCG_RESULT  Ret;
  UINT8       MethodStatus;

  NULL_CHECK (Session);
  NULL_CHECK (Msid);
  NULL_CHECK (Password);

  //
  // Start session with admin sp to update SID to MSID
  //
  Ret = OpalStartSession (
          Session,
          OPAL_UID_ADMIN_SP,
          TRUE,
          PasswordLength,
          Password,
          OPAL_ADMIN_SP_SID_AUTHORITY,
          &MethodStatus
          );
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    goto done;
  }

  //
  // Update SID pin
  //
  Ret = OpalSetPassword (Session, OPAL_UID_ADMIN_SP_C_PIN_SID, Msid, MsidLength, &MethodStatus);
  OpalEndSession (Session);

done:
  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    Ret = TcgResultFailure;
  }

  return Ret;
}

/**
  Update global locking range.

  @param          Session,           The session info for one opal device.
  @param          Password,          Input password info.
  @param          PasswordLength,    Input password length.
  @param          ReadLocked,        Read lock info.
  @param          WriteLocked        write lock info.

**/
TCG_RESULT
EFIAPI
OpalUtilUpdateGlobalLockingRange (
  OPAL_SESSION  *Session,
  const VOID    *Password,
  UINT32        PasswordLength,
  BOOLEAN       ReadLocked,
  BOOLEAN       WriteLocked
  )
{
  UINT8       MethodStatus;
  TCG_RESULT  Ret;

  NULL_CHECK (Session);
  NULL_CHECK (Password);

  //
  // Try to start session with Locking SP as admin1 authority
  //
  Ret = OpalStartSession (
          Session,
          OPAL_UID_LOCKING_SP,
          TRUE,
          PasswordLength,
          Password,
          OPAL_LOCKING_SP_ADMIN1_AUTHORITY,
          &MethodStatus
          );
  if ((Ret == TcgResultSuccess) && (MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS)) {
    Ret = OpalUpdateGlobalLockingRange (
            Session,
            ReadLocked,
            WriteLocked,
            &MethodStatus
            );
    OpalEndSession (Session);
    if ((Ret == TcgResultSuccess) && (MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS)) {
      goto done;
    }
  }

  if (MethodStatus == TCG_METHOD_STATUS_CODE_AUTHORITY_LOCKED_OUT) {
    DEBUG ((DEBUG_INFO, "unlock as admin failed with AUTHORITY_LOCKED_OUT\n"));
  }

  //
  // Try user1 authority
  //
  Ret = OpalStartSession (
          Session,
          OPAL_UID_LOCKING_SP,
          TRUE,
          PasswordLength,
          Password,
          OPAL_LOCKING_SP_USER1_AUTHORITY,
          &MethodStatus
          );
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "StartSession with Locking SP as User1 failed\n"));
    goto done;
  }

  Ret = OpalUpdateGlobalLockingRange (Session, ReadLocked, WriteLocked, &MethodStatus);
  OpalEndSession (Session);

done:
  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    if (MethodStatus == TCG_METHOD_STATUS_CODE_AUTHORITY_LOCKED_OUT) {
      //
      // Caller need to know this special error, but return status not has type for it.
      // so here use TcgResultFailureInvalidType as an replacement.
      //
      Ret = TcgResultFailureInvalidType;
    } else {
      Ret = TcgResultFailure;
    }
  }

  return Ret;
}

/**
  Update global locking range.

  @param          Session,           The session info for one opal device.
  @param          Msid,              The data buffer to save Msid info.
  @param          MsidBufferLength,  The data buffer length for Msid.
  @param          MsidLength,        The actual data length for Msid.

**/
TCG_RESULT
EFIAPI
OpalUtilGetMsid (
  OPAL_SESSION  *Session,
  UINT8         *Msid,
  UINT32        MsidBufferLength,
  UINT32        *MsidLength
  )
{
  UINT8       MethodStatus;
  TCG_RESULT  Ret;

  NULL_CHECK (Session);
  NULL_CHECK (Msid);
  NULL_CHECK (MsidLength);

  Ret = OpalStartSession (
          Session,
          OPAL_UID_ADMIN_SP,
          TRUE,
          0,
          NULL,
          TCG_UID_NULL,
          &MethodStatus
          );
  if ((Ret == TcgResultSuccess) && (MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS)) {
    Ret = OpalGetMsid (Session, MsidBufferLength, Msid, MsidLength);
    OpalEndSession (Session);
  }

  if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
    Ret = TcgResultFailure;
  }

  return Ret;
}

/**

  The function determines who owns the device by attempting to start a session with different credentials.
  If the SID PIN matches the MSID PIN, the no one owns the device.
  If the SID PIN matches the ourSidPin, then "Us" owns the device.  Otherwise it is unknown.


  @param[in]      Session            The session info for one opal device.
  @param          Msid,              The Msid info.
  @param          MsidLength,        The data length for Msid.

**/
OPAL_OWNER_SHIP
EFIAPI
OpalUtilDetermineOwnership (
  OPAL_SESSION  *Session,
  UINT8         *Msid,
  UINT32        MsidLength
  )
{
  UINT8            MethodStatus;
  TCG_RESULT       Ret;
  OPAL_OWNER_SHIP  Owner;

  if ((Session == NULL) || (Msid == NULL)) {
    return OpalOwnershipUnknown;
  }

  Owner = OpalOwnershipUnknown;
  //
  // Start Session as SID_UID with ADMIN_SP using MSID PIN
  //
  Ret = OpalStartSession (
          Session,
          OPAL_UID_ADMIN_SP,
          TRUE,
          MsidLength,
          Msid,
          OPAL_ADMIN_SP_SID_AUTHORITY,
          &MethodStatus
          );
  if ((Ret == TcgResultSuccess) && (MethodStatus == TCG_METHOD_STATUS_CODE_SUCCESS)) {
    //
    // now we know that SID PIN == MSID PIN
    //
    Owner = OpalOwnershipNobody;

    OpalEndSession (Session);
  }

  return Owner;
}

/**

  The function returns if admin password exists.

  @param[in]      OwnerShip         The owner ship of the opal device.
  @param[in]      LockingFeature    The locking info of the opal device.

  @retval         TRUE              Admin password existed.
  @retval         FALSE             Admin password not existed.

**/
BOOLEAN
EFIAPI
OpalUtilAdminPasswordExists (
  IN  UINT16                          OwnerShip,
  IN  TCG_LOCKING_FEATURE_DESCRIPTOR  *LockingFeature
  )
{
  NULL_CHECK (LockingFeature);

  // if it is Unknown who owns the device
  // then someone has set password previously through our UI
  // because the SID would no longer match the generated SID (ownership us)
  // or someone has set password using 3rd party software

  //
  // Locking sp enabled is checked b/c it must be enabled to change the PIN of the Admin1.
  //
  return (OwnerShip == OpalOwnershipUnknown && LockingFeature->LockingEnabled);
}

/**
  Get Active Data Removal Mechanism Value.

  @param[in]      Session                        The session info for one opal device.
  @param[in]      GeneratedSid                   Generated SID of disk
  @param[in]      SidLength                      Length of generatedSid in bytes
  @param[out]     ActiveDataRemovalMechanism     Return the active data removal mechanism.

**/
TCG_RESULT
EFIAPI
OpalUtilGetActiveDataRemovalMechanism (
  OPAL_SESSION  *Session,
  const VOID    *GeneratedSid,
  UINT32        SidLength,
  UINT8         *ActiveDataRemovalMechanism
  )
{
  TCG_RESULT  Ret;
  UINT8       MethodStatus;

  NULL_CHECK (Session);
  NULL_CHECK (GeneratedSid);
  NULL_CHECK (ActiveDataRemovalMechanism);

  Ret = OpalStartSession (
          Session,
          OPAL_UID_ADMIN_SP,
          TRUE,
          SidLength,
          GeneratedSid,
          OPAL_ADMIN_SP_ANYBODY_AUTHORITY,
          &MethodStatus
          );
  if ((Ret != TcgResultSuccess) || (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS)) {
    DEBUG ((DEBUG_INFO, "Start session with admin SP as SID authority failed: Ret=%d MethodStatus=%u\n", Ret, MethodStatus));
    if (MethodStatus != TCG_METHOD_STATUS_CODE_SUCCESS) {
      Ret = TcgResultFailure;
    }

    return Ret;
  }

  Ret = OpalPyrite2GetActiveDataRemovalMechanism (
          Session,
          ActiveDataRemovalMechanism
          );

  if (Ret != TcgResultSuccess) {
    DEBUG ((DEBUG_INFO, "Pyrite2 Get Active Data Removal Mechanism failed: Ret=%d\n", Ret));
  }

  OpalEndSession (Session);

  return Ret;
}

/**
  Calculate the estimated time.

  @param[in]      IsMinute               Whether the input time value is minute type or second type.
  @param[in]      Time                   The input time value.

**/
UINT32
CalculateDataRemovalTime (
  IN BOOLEAN  IsMinute,
  IN UINT16   Time
  )
{
  if (IsMinute) {
    return Time * 2 * 60;
  } else {
    return Time * 2;
  }
}

/**
  Return the estimated time for specific type.

  @param[in]      Index               The input data removal type.
  @param[in]      Descriptor          DATA_REMOVAL_FEATURE_DESCRIPTOR

**/
UINT32
GetDataRemovalTime (
  IN  UINT8                            Index,
  IN  DATA_REMOVAL_FEATURE_DESCRIPTOR  *Descriptor
  )
{
  switch (Index) {
    case OverwriteDataErase:
      return CalculateDataRemovalTime (Descriptor->FormatBit0, SwapBytes16 (Descriptor->TimeBit0));

    case BlockErase:
      return CalculateDataRemovalTime (Descriptor->FormatBit1, SwapBytes16 (Descriptor->TimeBit1));

    case CryptoErase:
      return CalculateDataRemovalTime (Descriptor->FormatBit2, SwapBytes16 (Descriptor->TimeBit2));

    case Unmap:
      return CalculateDataRemovalTime (Descriptor->FormatBit3, SwapBytes16 (Descriptor->TimeBit3));

    case ResetWritePointers:
      return CalculateDataRemovalTime (Descriptor->FormatBit4, SwapBytes16 (Descriptor->TimeBit4));

    case VendorSpecificErase:
      return CalculateDataRemovalTime (Descriptor->FormatBit5, SwapBytes16 (Descriptor->TimeBit5));

    default:
      return 0;
  }
}

/**
  Get the supported Data Removal Mechanism list.

  @param[in]      Session                        The session info for one opal device.
  @param[out]     RemovalMechanismLists          Return the supported data removal mechanism lists.

**/
TCG_RESULT
EFIAPI
OpalUtilGetDataRemovalMechanismLists (
  IN  OPAL_SESSION  *Session,
  OUT UINT32        *RemovalMechanismLists
  )
{
  TCG_RESULT                       Ret;
  UINTN                            DataSize;
  DATA_REMOVAL_FEATURE_DESCRIPTOR  Descriptor;
  UINT8                            Index;
  UINT8                            BitValue;

  NULL_CHECK (Session);
  NULL_CHECK (RemovalMechanismLists);

  DataSize = sizeof (Descriptor);
  Ret      = OpalGetFeatureDescriptor (Session, TCG_FEATURE_DATA_REMOVAL, &DataSize, &Descriptor);
  if (Ret != TcgResultSuccess) {
    return TcgResultFailure;
  }

  ASSERT (Descriptor.RemovalMechanism != 0);

  for (Index = 0; Index < ResearvedMechanism; Index++) {
    BitValue = (BOOLEAN)BitFieldRead8 (Descriptor.RemovalMechanism, Index, Index);

    if (BitValue == 0) {
      RemovalMechanismLists[Index] = 0;
    } else {
      RemovalMechanismLists[Index] = GetDataRemovalTime (Index, &Descriptor);
    }
  }

  return TcgResultSuccess;
}

/**
  Get revert timeout value.

  @param[in]      Session                       The session info for one opal device.

**/
UINT32
GetRevertTimeOut (
  IN OPAL_SESSION  *Session
  )
{
  TCG_RESULT                   TcgResult;
  OPAL_DISK_SUPPORT_ATTRIBUTE  SupportedAttributes;
  UINT16                       BaseComId;
  UINT32                       MsidLength;
  UINT8                        Msid[OPAL_MSID_LENGTH];
  UINT32                       RemovalMechanishLists[ResearvedMechanism];
  UINT8                        ActiveDataRemovalMechanism;

  TcgResult = OpalGetSupportedAttributesInfo (Session, &SupportedAttributes, &BaseComId);
  if ((TcgResult != TcgResultSuccess) || (SupportedAttributes.DataRemoval == 0)) {
    return 0;
  }

  TcgResult = OpalUtilGetMsid (Session, Msid, OPAL_MSID_LENGTH, &MsidLength);
  if (TcgResult != TcgResultSuccess) {
    return 0;
  }

  TcgResult = OpalUtilGetDataRemovalMechanismLists (Session, RemovalMechanishLists);
  if (TcgResult != TcgResultSuccess) {
    return 0;
  }

  TcgResult = OpalUtilGetActiveDataRemovalMechanism (Session, Msid, MsidLength, &ActiveDataRemovalMechanism);
  if (TcgResult != TcgResultSuccess) {
    return 0;
  }

  return RemovalMechanishLists[ActiveDataRemovalMechanism];
}
