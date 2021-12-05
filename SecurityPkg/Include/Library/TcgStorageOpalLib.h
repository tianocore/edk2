/** @file
  Public API for Opal Core library.

  (TCG Storage Architecture Core Specification, Version 2.01, Revision 1.00,
  https://trustedcomputinggroup.org/tcg-storage-architecture-core-specification/

  Storage Work Group Storage Security Subsystem Class: Pyrite, Specification Version 2.00, Revision 1.00,
  https://trustedcomputinggroup.org/resource/tcg-storage-security-subsystem-class-pyrite/

  Storage Work Group Storage Security Subsystem Class: Opal, Version 2.01 Final, Revision 1.00,
  https://trustedcomputinggroup.org/storage-work-group-storage-security-subsystem-class-opal/

  TCG Storage Security Subsystem Class: Opalite Version 1.00 Revision 1.00,
  https://trustedcomputinggroup.org/tcg-storage-security-subsystem-class-opalite/

  TCG Storage Feature Set: Block SID Authentication, Version 1.00 Final, Revision 1.00,
  https://trustedcomputinggroup.org/tcg-storage-feature-set-block-sid-authentication-specification/

  TCG Storage Opal SSC Feature Set: PSID Version 1.00, Revision 1.00,
  https://trustedcomputinggroup.org/tcg-storage-opal-feature-set-psid/)

  Check http://trustedcomputinggroup.org for latest specification updates.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _OPAL_CORE_H_
#define _OPAL_CORE_H_

#include <IndustryStandard/TcgStorageOpal.h>

#include <Library/TcgStorageCoreLib.h>
#include <Protocol/StorageSecurityCommand.h>

#pragma pack(1)

typedef struct {
  //
  // Opal SSC 1 support  (0 - not supported, 1 - supported)
  //
  UINT32    OpalSsc1          : 1;

  //
  // Opal SSC 2support  (0 - not supported, 1 - supported)
  //
  UINT32    OpalSsc2          : 1;

  //
  // Opal SSC Lite support  (0 - not supported, 1 - supported)
  //
  UINT32    OpalSscLite       : 1;

  //
  // Pyrite SSC support  (0 - not supported, 1 - supported)
  //
  UINT32    PyriteSsc         : 1;

  //
  // Security protocol 1 support  (0 - not supported, 1 - supported)
  //
  UINT32    Sp1               : 1;

  //
  // Security protocol 2 support  (0 - not supported, 1 - supported)
  //
  UINT32    Sp2               : 1;

  //
  // Security protocol IEEE1667 support  (0 - not supported, 1 - supported)
  //
  UINT32    SpIeee1667        : 1;

  //
  // Media encryption supported (0 - not supported, 1 - supported)
  //
  UINT32    MediaEncryption   : 1;

  //
  // Initial C_PIN_SID PIN Indicator
  //  0 - The initial C_PIN_SID PIN value is NOT equal to the C_PIN_MSID PIN value
  //  1 - The initial C_PIN_SID PIN value is equal to the C_PIN_MSID PIN value
  //
  UINT32    InitCpinIndicator : 1;

  //
  // Behavior of C_PIN_SID PIN upon TPer Revert
  //  0 - The initial C_PIN_SID PIN value is NOT equal to the C_PIN_MSID PIN value
  //  1 - The initial C_PIN_SID PIN value is equal to the C_PIN_MSID PIN value
  //
  UINT32    CpinUponRevert    : 1;

  //
  // Media encryption supported (0 - not supported, 1 - supported)
  //
  UINT32    BlockSid          : 1;

  //
  // Pyrite SSC V2 support  (0 - not supported, 1 - supported)
  //
  UINT32    PyriteSscV2       : 1;

  //
  // Supported Data Removal Mechanism support  (0 - not supported, 1 - supported)
  //
  UINT32    DataRemoval       : 1;
} OPAL_DISK_SUPPORT_ATTRIBUTE;

//
// Opal device ownership type
// The type indicates who was the determined owner of the device.
//
typedef enum {
  //
  // Represents the device ownership is unknown because starting a session as the SID authority with the ADMIN SP
  // was unsuccessful with the provided PIN
  //
  OpalOwnershipUnknown,

  //
  // Represents that the ADMIN SP SID authority contains the same PIN as the MSID PIN
  //
  OpalOwnershipNobody,
} OPAL_OWNER_SHIP;

//
// Structure that is used to represent an Opal session.
// The structure must be initialized by calling OpalStartSession before being used as a parameter
// for any other Opal function.
// This structure should NOT be directly modified by the client of this library.
//
//
typedef struct  {
  UINT32                                   HostSessionId;
  UINT32                                   TperSessionId;
  UINT16                                   ComIdExtension;

  UINT16                                   OpalBaseComId;

  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *Sscp;
  UINT32                                   MediaId;
} OPAL_SESSION;
#pragma pack()

/**

  The function fills in the provided Buffer with the supported protocol list
  of the device specified.

  @param[in]        Session         OPAL_SESSION data.
  @param[in]        BufferSize      Size of Buffer provided (in bytes)
  @param[in]        BuffAddress     Buffer address to fill with security protocol list

**/
TCG_RESULT
EFIAPI
OpalRetrieveSupportedProtocolList (
  OPAL_SESSION  *Session,
  UINTN         BufferSize,
  VOID          *BuffAddress
  );

/**

  The function fills in the provided Buffer with the level 0 discovery Header
  of the device specified.

  @param[in]        Session         OPAL_SESSION data.
  @param[in]        BufferSize      Size of Buffer provided (in bytes)
  @param[in]        BuffAddress     Buffer address to fill with Level 0 Discovery response

**/
TCG_RESULT
EFIAPI
OpalRetrieveLevel0DiscoveryHeader (
  OPAL_SESSION  *Session,
  UINTN         BufferSize,
  VOID          *BuffAddress
  );

/**
  Starts a session with a security provider (SP).

  If a session is started successfully, the caller must end the session with OpalEndSession when finished
  performing Opal actions.

  @param[in/out]  Session                 OPAL_SESSION to initialize.
  @param[in]      SpId                    Security provider ID to start the session with.
  @param[in]      Write                   Whether the session should be read-only (FALSE) or read/write (TRUE).
  @param[in]      HostChallengeLength     Length of the host challenge.  Length should be 0 if hostChallenge is NULL
  @param[in]      HostChallenge           Host challenge for Host Signing Authority.  If NULL, then no Host Challenge will be sent.
  @param[in]      HostSigningAuthority    Host Signing Authority used for start session.  If NULL, then no Host Signing Authority will be sent.
  @param[in/out]  MethodStatus            Status of the StartSession method; only valid if TcgResultSuccess is returned.

  @return TcgResultSuccess indicates that the function completed without any internal errors.
  The caller must inspect the MethodStatus field to determine whether the method completed successfully.

**/
TCG_RESULT
EFIAPI
OpalStartSession (
  OPAL_SESSION  *Session,
  TCG_UID       SpId,
  BOOLEAN       Write,
  UINT32        HostChallengeLength,
  const VOID    *HostChallenge,
  TCG_UID       HostSigningAuthority,
  UINT8         *MethodStatus
  );

/**
  Close a session opened with OpalStartSession.

  @param[in/out]  Session                 OPAL_SESSION to end.

**/
TCG_RESULT
EFIAPI
OpalEndSession (
  OPAL_SESSION  *Session
  );

/**

  Reverts device using Admin SP Revert method.

  @param[in]  AdminSpSession      OPAL_SESSION with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY to perform PSID revert.

**/
TCG_RESULT
EFIAPI
OpalPsidRevert (
  OPAL_SESSION  *AdminSpSession
  );

/**

  The function retrieves the MSID from the device specified

  @param[in]  AdminSpSession      OPAL_SESSION with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY to perform PSID revert.
  @param[in]  MsidBufferSize      Allocated buffer size (in bytes) for MSID allocated by caller
  @param[in]  Msid                Variable length byte sequence representing MSID of device
  @param[in]  MsidLength          Actual length of MSID retrieved from device

**/
TCG_RESULT
EFIAPI
OpalGetMsid (
  OPAL_SESSION  *AdminSpSession,
  UINT32        MsidBufferSize,
  UINT8         *Msid,
  UINT32        *MsidLength
  );

/**

  The function activates the Locking SP.
  Once activated, per Opal spec, the ADMIN SP SID PIN is copied over to the ADMIN1 LOCKING SP PIN.
  If the Locking SP is already enabled, then TcgResultSuccess is returned and no action occurs.

  @param[in]      AdminSpSession      OPAL_SESSION with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_SID_AUTHORITY to activate Locking SP
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalActivateLockingSp (
  OPAL_SESSION  *AdminSpSession,
  UINT8         *MethodStatus
  );

/**

  The function sets the PIN column of the specified cpinRowUid (authority) with the newPin value.

  @param[in/out]  Session                 OPAL_SESSION to set password
  @param[in]      CpinRowUid              UID of row (authority) to update PIN column
  @param[in]      NewPin                  New Pin to set for cpinRowUid specified
  @param[in]      NewPinLength            Length in bytes of newPin
  @param[in/out]  MethodStatus            Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalSetPassword (
  OPAL_SESSION  *Session,
  TCG_UID       CpinRowUid,
  const VOID    *NewPin,
  UINT32        NewPinLength,
  UINT8         *MethodStatus
  );

/**

  The function retrieves the active key of the global locking range
  and calls the GenKey method on the active key retrieved.

  @param[in]        LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP to generate key
  @param[in/out]    MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalGlobalLockingRangeGenKey (
  OPAL_SESSION  *LockingSpSession,
  UINT8         *MethodStatus
  );

/**

  The function updates the ReadLocked and WriteLocked columns of the Global Locking Range.
  This function is required for a user1 authority, since a user1 authority shall only have access to ReadLocked and WriteLocked columns
  (not ReadLockEnabled and WriteLockEnabled columns).

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP to generate key
  @param[in]      ReadLocked          Value to set ReadLocked column for Global Locking Range
  @param[in]      WriteLocked         Value to set WriteLocked column for Global Locking Range
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalUpdateGlobalLockingRange (
  OPAL_SESSION  *LockingSpSession,
  BOOLEAN       ReadLocked,
  BOOLEAN       WriteLocked,
  UINT8         *MethodStatus
  );

/**

  The function updates the RangeStart, RangeLength, ReadLockedEnabled, WriteLockedEnabled, ReadLocked and WriteLocked columns
  of the specified Locking Range.  This function requires admin authority of a locking SP session.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP to generate key
  @param[in]      LockingRangeUid     Locking range UID to set values
  @param[in]      RangeStart          Value to set RangeStart column for Locking Range
  @param[in]      RangeLength         Value to set RangeLength column for Locking Range
  @param[in]      ReadLockEnabled     Value to set readLockEnabled column for Locking Range
  @param[in]      WriteLockEnabled    Value to set writeLockEnabled column for Locking Range
  @param[in]      ReadLocked          Value to set ReadLocked column for Locking Range
  @param[in]      WriteLocked         Value to set WriteLocked column for Locking Range
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalSetLockingRange (
  OPAL_SESSION  *LockingSpSession,
  TCG_UID       LockingRangeUid,
  UINT64        RangeStart,
  UINT64        RangeLength,
  BOOLEAN       ReadLockEnabled,
  BOOLEAN       WriteLockEnabled,
  BOOLEAN       ReadLocked,
  BOOLEAN       WriteLocked,
  UINT8         *MethodStatus
  );

/**

  The function sets the Enabled column to TRUE for the authorityUid provided and updates the PIN column for the cpinRowUid provided
  using the newPin provided.  AuthorityUid and cpinRowUid should describe the same authority.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY to update
  @param[in]      CpinRowUid          Row UID of C_PIN table of Locking SP to update PIN
  @param[in]      AuthorityUid        UID of Locking SP authority to update Pin column with
  @param[in]      NewPin              New Password used to set Pin column
  @param[in]      NewPinLength        Length in bytes of new password
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalSetLockingSpAuthorityEnabledAndPin (
  OPAL_SESSION  *LockingSpSession,
  TCG_UID       CpinRowUid,
  TCG_UID       AuthorityUid,
  const VOID    *NewPin,
  UINT32        NewPinLength,
  UINT8         *MethodStatus
  );

/**

  The function sets the Enabled column to FALSE for the USER1 authority.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY to disable User1
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalDisableUser (
  OPAL_SESSION  *LockingSpSession,
  UINT8         *MethodStatus
  );

/**

  The function calls the Admin SP RevertSP method on the Locking SP.  If KeepUserData is True, then the optional parameter
  to keep the user data is set to True, otherwise the optional parameter is not provided.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY to revertSP
  @param[in]      KeepUserData        Specifies whether or not to keep user data when performing RevertSP action. True = keeps user data.
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.

**/
TCG_RESULT
EFIAPI
OpalAdminRevert (
  OPAL_SESSION  *LockingSpSession,
  BOOLEAN       KeepUserData,
  UINT8         *MethodStatus
  );

/**

  The function retrieves the TryLimit column for the specified rowUid (authority).

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP to retrieve try limit
  @param[in]      RowUid              Row UID of the Locking SP C_PIN table to retrieve TryLimit column
  @param[in/out]  TryLimit            Value from TryLimit column

**/
TCG_RESULT
EFIAPI
OpalGetTryLimit (
  OPAL_SESSION  *LockingSpSession,
  TCG_UID       RowUid,
  UINT32        *TryLimit
  );

/**

  The function populates the CreateStruct with a payload that will retrieve the global locking range active key.
  It is intended to be called with a session that is already started with a valid credential.
  The function does not send the payload.

  @param[in]      Session        OPAL_SESSION to populate command for, needs comId
  @param[in/out]  CreateStruct   Structure to populate with encoded TCG command
  @param[in/out]  Size           Size in bytes of the command created.

**/
TCG_RESULT
EFIAPI
OpalCreateRetrieveGlobalLockingRangeActiveKey (
  const OPAL_SESSION  *Session,
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT32              *Size
  );

/**

  The function acquires the activeKey specified for the Global Locking Range from the parseStruct.

  @param[in]      ParseStruct    Structure that contains the device's response with the activekey
  @param[in/out]  ActiveKey      The UID of the active key retrieved

**/
TCG_RESULT
EFIAPI
OpalParseRetrieveGlobalLockingRangeActiveKey (
  TCG_PARSE_STRUCT  *ParseStruct,
  TCG_UID           *ActiveKey
  );

/**

  Get the support attribute info.

  @param[in]      Session             OPAL_SESSION with OPAL_UID_LOCKING_SP to retrieve info.
  @param[in/out]  LockingFeature      Return the Locking info.

**/
TCG_RESULT
EFIAPI
OpalGetLockingInfo (
  OPAL_SESSION                    *Session,
  TCG_LOCKING_FEATURE_DESCRIPTOR  *LockingFeature
  );

/**

  The function determines whether or not all of the requirements for the Opal Feature (not full specification)
  are met by the specified device.

  @param[in]      SupportedAttributes     Opal device attribute.

**/
BOOLEAN
EFIAPI
OpalFeatureSupported (
  OPAL_DISK_SUPPORT_ATTRIBUTE  *SupportedAttributes
  );

/**

  The function returns whether or not the device is Opal Enabled.
  TRUE means that the device is partially or fully locked.
  This will perform a Level 0 Discovery and parse the locking feature descriptor

  @param[in]      SupportedAttributes     Opal device attribute.
  @param[in]      LockingFeature          Opal device locking status.


**/
BOOLEAN
EFIAPI
OpalFeatureEnabled (
  OPAL_DISK_SUPPORT_ATTRIBUTE     *SupportedAttributes,
  TCG_LOCKING_FEATURE_DESCRIPTOR  *LockingFeature
  );

/**

  The function returns whether or not the device is Opal Locked.
  TRUE means that the device is partially or fully locked.
  This will perform a Level 0 Discovery and parse the locking feature descriptor

  @param[in]      SupportedAttributes     Opal device attribute.
  @param[in]      LockingFeature          Opal device locking status.

**/
BOOLEAN
OpalDeviceLocked (
  OPAL_DISK_SUPPORT_ATTRIBUTE     *SupportedAttributes,
  TCG_LOCKING_FEATURE_DESCRIPTOR  *LockingFeature
  );

/**
  Trig the block sid action.

  @param[in]      Session            OPAL_SESSION to populate command for, needs comId
  @param[in]      HardwareReset      Whether need to do hardware reset.

**/
TCG_RESULT
EFIAPI
OpalBlockSid (
  OPAL_SESSION  *Session,
  BOOLEAN       HardwareReset
  );

/**

  Get the support attribute info.

  @param[in]      Session             OPAL_SESSION with OPAL_UID_LOCKING_SP to retrieve info.
  @param[in/out]  SupportedAttributes Return the support attribute info.
  @param[out]     OpalBaseComId       Return the base com id info.

**/
TCG_RESULT
EFIAPI
OpalGetSupportedAttributesInfo (
  OPAL_SESSION                 *Session,
  OPAL_DISK_SUPPORT_ATTRIBUTE  *SupportedAttributes,
  UINT16                       *OpalBaseComId
  );

/**
  Creates a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY, then reverts device using Admin SP Revert method.

  @param[in]      AdminSpSession     OPAL_SESSION to populate command for, needs comId
  @param[in]      Psid               PSID of device to revert.
  @param[in]      PsidLength         Length of PSID in bytes.

**/
TCG_RESULT
EFIAPI
OpalUtilPsidRevert (
  OPAL_SESSION  *AdminSpSession,
  const VOID    *Psid,
  UINT32        PsidLength
  );

/**
  Opens a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_SID_AUTHORITY,
  sets the OPAL_UID_ADMIN_SP_C_PIN_SID column with the new password,
  and activates the locking SP to copy SID PIN to Admin1 Locking SP PIN.

  @param[in]      AdminSpSession     OPAL_SESSION to populate command for, needs comId
  @param[in]      GeneratedSid       Generated SID of disk
  @param[in]      SidLength          Length of generatedSid in bytes
  @param[in]      Password           New admin password to set
  @param[in]      PassLength         Length of password in bytes

**/
TCG_RESULT
EFIAPI
OpalUtilSetAdminPasswordAsSid (
  OPAL_SESSION  *AdminSpSession,
  const VOID    *GeneratedSid,
  UINT32        SidLength,
  const VOID    *Password,
  UINT32        PassLength
  );

/**

  Opens a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY,
  and updates the specified locking range with the provided column values.

  @param[in]      LockingSpSession   OPAL_SESSION to populate command for, needs comId
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
  OPAL_SESSION  *LockingSpSession,
  const VOID    *Password,
  UINT32        PassLength,
  TCG_UID       LockingRangeUid,
  UINT64        RangeStart,
  UINT64        RangeLength,
  BOOLEAN       ReadLockEnabled,
  BOOLEAN       WriteLockEnabled,
  BOOLEAN       ReadLocked,
  BOOLEAN       WriteLocked
  );

/**
  Opens a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_SID_AUTHORITY,
  sets OPAL_UID_ADMIN_SP_C_PIN_SID with the new password,
  and sets OPAL_LOCKING_SP_C_PIN_ADMIN1 with the new password.

  @param[in]      AdminSpSession     OPAL_SESSION to populate command for, needs comId
  @param[in]      OldPassword        Current admin password
  @param[in]      OldPasswordLength  Length of current admin password in bytes
  @param[in]      NewPassword        New admin password to set
  @param[in]      NewPasswordLength  Length of new password in bytes

**/
TCG_RESULT
EFIAPI
OpalUtilSetAdminPassword (
  OPAL_SESSION  *AdminSpSession,
  const VOID    *OldPassword,
  UINT32        OldPasswordLength,
  const VOID    *NewPassword,
  UINT32        NewPasswordLength
  );

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_USER1_AUTHORITY or OPAL_LOCKING_SP_ADMIN1_AUTHORITY
  and sets the User1 SP authority to enabled and sets the User1 password.

  @param[in]      LockingSpSession   OPAL_SESSION to populate command for, needs comId
  @param[in]      OldPassword        Current admin password
  @param[in]      OldPasswordLength  Length of current admin password in bytes
  @param[in]      NewPassword        New admin password to set
  @param[in]      NewPasswordLength  Length of new password in bytes

**/
TCG_RESULT
EFIAPI
OpalUtilSetUserPassword (
  OPAL_SESSION  *LockingSpSession,
  const VOID    *OldPassword,
  UINT32        OldPasswordLength,
  const VOID    *NewPassword,
  UINT32        NewPasswordLength
  );

/**
  Verify whether user input the correct password.

  @param[in]      LockingSpSession            OPAL_SESSION to populate command for, needs comId
  @param[in]      Password                    Admin password
  @param[in]      PasswordLength              Length of password in bytes
  @param[in/out]  HostSigningAuthority        Use the Host signing authority type.

**/
TCG_RESULT
EFIAPI
OpalUtilVerifyPassword (
  OPAL_SESSION  *LockingSpSession,
  const VOID    *Password,
  UINT32        PasswordLength,
  TCG_UID       HostSigningAuthority
  );

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_USER1_AUTHORITY or OPAL_LOCKING_SP_ADMIN1_AUTHORITY
  and generates a new global locking range key to erase the Data.

  @param[in]      LockingSpSession     OPAL_SESSION to populate command for, needs comId
  @param[in]      Password             Admin or user password
  @param[in]      PasswordLength       Length of password in bytes
  @param[in/out]  PasswordFailed       indicates if password failed (start session didn't work)

**/
TCG_RESULT
EFIAPI
OpalUtilSecureErase (
  OPAL_SESSION  *LockingSpSession,
  const VOID    *Password,
  UINT32        PasswordLength,
  BOOLEAN       *PasswordFailed
  );

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY and disables the User1 authority.

  @param[in]      LockingSpSession      OPAL_SESSION to populate command for, needs comId
  @param[in]      Password              Admin password
  @param[in]      PasswordLength        Length of password in bytes
  @param[in/out]  PasswordFailed        indicates if password failed (start session didn't work)

**/
TCG_RESULT
EFIAPI
OpalUtilDisableUser (
  OPAL_SESSION  *LockingSpSession,
  const VOID    *Password,
  UINT32        PasswordLength,
  BOOLEAN       *PasswordFailed
  );

/**
  Opens a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY, then reverts the device using the RevertSP method.

  @param[in]      LockingSpSession      OPAL_SESSION to populate command for, needs comId
  @param[in]      KeepUserData       TRUE to keep existing Data on the disk, or FALSE to erase it
  @param[in]      Password           Admin password
  @param[in]      PasswordLength     Length of password in bytes
  @param[in/out]  PasswordFailed     indicates if password failed (start session didn't work)
  @param[in]      Msid               Input Msid info.
  @param[in]      MsidLength         Input Msid info length.

**/
TCG_RESULT
EFIAPI
OpalUtilRevert (
  OPAL_SESSION  *LockingSpSession,
  BOOLEAN       KeepUserData,
  const VOID    *Password,
  UINT32        PasswordLength,
  BOOLEAN       *PasswordFailed,
  UINT8         *Msid,
  UINT32        MsidLength
  );

/**
  After revert success, set SID to MSID.

  @param[in]      AdminSpSession     OPAL_SESSION to populate command for, needs comId
  @param          Password,          Input password info.
  @param          PasswordLength,    Input password length.
  @param[in]      Msid               Input Msid info.
  @param[in]      MsidLength         Input Msid info length.

**/
TCG_RESULT
EFIAPI
OpalUtilSetSIDtoMSID (
  OPAL_SESSION  *AdminSpSession,
  const VOID    *Password,
  UINT32        PasswordLength,
  UINT8         *Msid,
  UINT32        MsidLength
  );

/**
  Update global locking range.

  @param[in]      LockingSpSession   OPAL_SESSION to populate command for, needs comId
  @param          Password,          Input password info.
  @param          PasswordLength,    Input password length.
  @param          ReadLocked,        Read lock info.
  @param          WriteLocked        write lock info.

**/
TCG_RESULT
EFIAPI
OpalUtilUpdateGlobalLockingRange (
  OPAL_SESSION  *LockingSpSession,
  const VOID    *Password,
  UINT32        PasswordLength,
  BOOLEAN       ReadLocked,
  BOOLEAN       WriteLocked
  );

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
  );

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
  );

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
  );

/**
  Get Active Data Removal Mechanism Value.

  @param[in]      Session,                       The session info for one opal device.
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
  );

/**
  Get the supported Data Removal Mechanism list.

  @param[in]      Session,                       The session info for one opal device.
  @param[out]     RemovalMechanismLists          Return the supported data removal mechanism lists.

**/
TCG_RESULT
EFIAPI
OpalUtilGetDataRemovalMechanismLists (
  IN  OPAL_SESSION  *Session,
  OUT UINT32        *RemovalMechanismLists
  );

#endif // _OPAL_CORE_H_
