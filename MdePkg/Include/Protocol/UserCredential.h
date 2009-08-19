/** @file
  UEFI 2.2 User Credential Protocol definition.

  Attached to a device handle, this protocol identifies a single means of identifying the user.

  Copyright (c) 2009, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __USER_CREDENTIAL_H__
#define __USER_CREDENTIAL_H__

#define EFI_USER_CREDENTIAL_PROTOCOL_GUID \
  { \
    0x71ee5e94, 0x65b9, 0x45d5, { 0x82, 0x1a, 0x3a, 0x4d, 0x86, 0xcf, 0xe6, 0xbe } \
  }

typedef VOID *EFI_USER_PROFILE_HANDLE;
typedef VOID *EFI_USER_INFO_HANDLE;

///
/// The attributes of the user profile information.
///
typedef UINT16 EFI_USER_INFO_ATTRIBS;
#define EFI_USER_INFO_STORAGE                   0x000F
#define EFI_USER_INFO_STORAGE_VOLATILE          0x0000
#define EFI_USER_INFO_STORAGE_CREDENTIAL_NV     0x0001
#define EFI_USER_INFO_STORAGE_PLATFORM_NV       0x0002

#define EFI_USER_INFO_ACCESS                    0x0070
#define EFI_USER_INFO_PUBLIC                    0x0010
#define EFI_USER_INFO_PRIVATE                   0x0020
#define EFI_USER_INFO_PROTECTED                 0x0030
#define EFI_USER_INFO_EXCLUSIVE                 0x0080

///
/// User information structure
///
typedef struct {
  ///
  /// The user credential identifier associated with this user information or else Nil if the 
  /// information is not associated with any specific credential.
  ///
  EFI_GUID               Credential;
  ///
  /// The type of user information.
  ///
  UINT8                  InfoType;
  ///
  /// Must be set to 0.
  ///
  UINT8                  Reserved1;
  ///
  /// The attributes of the user profile information.
  ///
  EFI_USER_INFO_ATTRIBS  InfoAttribs;
  ///
  /// The size of the user information, in bytes, including this header.
  ///
  UINT32                 InfoSize;
} EFI_USER_INFO;

///
/// User credential class GUIDs
///
#define EFI_USER_CREDENTIAL_CLASS_UNKNOWN \
  { 0x5cf32e68, 0x7660, 0x449b, { 0x80, 0xe6, 0x7e, 0xa3, 0x6e, 0x3, 0xf6, 0xa8 } }
#define EFI_USER_CREDENTIAL_CLASS_PASSWORD \
  { 0xf8e5058c, 0xccb6, 0x4714, { 0xb2, 0x20, 0x3f, 0x7e, 0x3a, 0x64, 0xb, 0xd1 } }
#define EFI_USER_CREDENTIAL_CLASS_SMART_CARD \
  { 0x5f03ba33, 0x8c6b, 0x4c24, { 0xaa, 0x2e, 0x14, 0xa2, 0x65, 0x7b, 0xd4, 0x54 } }
#define EFI_USER_CREDENTIAL_CLASS_FINGERPRINT \
  { 0x32cba21f, 0xf308, 0x4cbc, { 0x9a, 0xb5, 0xf5, 0xa3, 0x69, 0x9f, 0x4, 0x4a } }
#define EFI_USER_CREDENTIAL_CLASS_HANDPRINT \
  { 0x5917ef16, 0xf723, 0x4bb9, { 0xa6, 0x4b, 0xd8, 0xc5, 0x32, 0xf4, 0xd8, 0xb5 } }
#define EFI_USER_CREDENTIAL_CLASS_SECURE_CARD \
  { 0x8a6b4a83, 0x42fe, 0x45d2, { 0xa2, 0xef, 0x46, 0xf0, 0x6c, 0x7d, 0x98, 0x52 } }

typedef UINT64   EFI_CREDENTIAL_CAPABILITIES;
#define EFI_CREDENTIAL_CAPABILITIES_ENROLL  0x0000000000000001

///
/// Credential logon flags 
///
typedef UINT32 EFI_CREDENTIAL_LOGON_FLAGS;
#define EFI_CREDENTIAL_LOGON_FLAG_AUTO                0x00000001
#define EFI_CREDENTIAL_LOGON_FLAG_DEFAULT             0x00000002

///
/// User information record types
///

///
/// No information.
///
#define EFI_USER_INFO_EMPTY_RECORD                    0x00
///
/// Provide the user's name for the enrolled user.
///
#define EFI_USER_INFO_NAME_RECORD                     0x01
typedef CHAR16 *EFI_USER_INFO_NAME;
///
/// Provides the date and time when the user profile was created.
///
#define EFI_USER_INFO_CREATE_DATE_RECORD              0x02
typedef EFI_TIME EFI_USER_INFO_CREATE_DATE;
///
/// Provides the date and time when the user profile was selected.
///
#define EFI_USER_INFO_USAGE_DATE_RECORD               0x03
typedef EFI_TIME EFI_USER_INFO_USAGE_DATE;
///
/// Provides the number of times that the user profile has been selected.
///
#define EFI_USER_INFO_USAGE_COUNT_RECORD              0x04
typedef UINT64 EFI_USER_INFO_USAGE_COUNT;
///
/// Provides a unique non-volatile user identifier for each enrolled user.
///
#define EFI_USER_INFO_IDENTIFIER_RECORD               0x05
typedef UINT8 EFI_USER_INFO_IDENTIFIER[16];
///
/// Specifies the type of a particular credential associated with the user profile.
///
#define EFI_USER_INFO_CREDENTIAL_TYPE_RECORD          0x06
typedef EFI_GUID EFI_USER_INFO_CREDENTIAL_TYPE;
///
/// Specifies the user-readable name of a particular credential type.
///
#define EFI_USER_INFO_CREDENTIAL_TYPE_NAME_RECORD     0x07
typedef CHAR16 *EFI_USER_INFO_CREDENTIAL_TYPE_NAME;
///
/// Specifies the credential provider.
///
#define EFI_USER_INFO_CREDENTIAL_PROVIDER_RECORD      0x08
typedef EFI_GUID EFI_USER_INFO_CREDENTIAL_PROVIDER;
///
/// Specifies the user-readable name of a particular credential's provider.
///
#define EFI_USER_INFO_CREDENTIAL_PROVIDER_NAME_RECORD 0x09
typedef CHAR16 *EFI_USER_INFO_CREDENTIAL_PROVIDER_NAME;
///
/// Provides PKCS#11 credential information from a smart card.
///
#define EFI_USER_INFO_PKCS11_RECORD                   0x0A
///
/// Provides standard biometric information in the format specified by the ISO 19785 (Common 
/// Biometric Exchange Formats Framework) specification.
///
#define EFI_USER_INFO_CBEFF_RECORD                    0x0B
typedef VOID *EFI_USER_INFO_CBEFF;
///
/// Indicates how close of a match the fingerprint must be in order to be considered a match.
///
#define EFI_USER_INFO_FAR_RECORD                      0x0C
typedef UINT8 EFI_USER_INFO_FAR;
///
/// Indicates how many attempts the user has to with a particular credential before the system prevents 
/// further attempts.
///
#define EFI_USER_INFO_RETRY_RECORD                    0x0D
typedef UINT8 EFI_USER_INFO_RETRY;
///
/// Provides the user's pre-OS access rights.
///
#define EFI_USER_INFO_ACCESS_POLICY_RECORD            0x0E

typedef struct {
  UINT32  Type;  ///< Specifies the type of user access control.
  UINT32  Size;  ///< Specifies the size of the user access control record, in bytes, including this header.
} EFI_USER_INFO_ACCESS_CONTROL;

typedef EFI_USER_INFO_ACCESS_CONTROL EFI_USER_INFO_ACCESS_POLICY;

///
/// User Information access types
///

///
/// Forbids the user from booting or loading executables from the specified device path or any child 
/// device paths.
///
#define EFI_USER_INFO_ACCESS_FORBID_LOAD              0x00000001
///
///
/// Permits the user from booting or loading executables from the specified device path or any child 
/// device paths.
///
#define EFI_USER_INFO_ACCESS_PERMIT_LOAD              0x00000002
///
/// Presence of this record indicates that a user can update enrollment information.
///
#define EFI_USER_INFO_ACCESS_ENROLL_SELF              0x00000003
///
/// Presence of this record indicates that a user can enroll new users.
///
#define EFI_USER_INFO_ACCESS_ENROLL_OTHERS            0x00000004
///
/// Presence of this record indicates that a user can update the user information of any user.
///
#define EFI_USER_INFO_ACCESS_MANAGE                   0x00000005
///
/// Describes permissions usable when configuring the platform.
///
#define EFI_USER_INFO_ACCESS_SETUP                    0x00000006
///
/// Standard GUIDs for access to configure the platform.
///
#define EFI_USER_INFO_ACCESS_SETUP_ADMIN_GUID \
  { 0x85b75607, 0xf7ce, 0x471e, { 0xb7, 0xe4, 0x2a, 0xea, 0x5f, 0x72, 0x32, 0xee } }
#define EFI_USER_INFO_ACCESS_SETUP_NORMAL_GUID \
  { 0x1db29ae0, 0x9dcb, 0x43bc, { 0x8d, 0x87, 0x5d, 0xa1, 0x49, 0x64, 0xdd, 0xe2 } }
#define EFI_USER_INFO_ACCESS_SETUP_RESTRICTED_GUID \
  { 0xbdb38125, 0x4d63, 0x49f4, { 0x82, 0x12, 0x61, 0xcf, 0x5a, 0x19, 0xa, 0xf8 } }

///
/// Forbids UEFI drivers from being started from the specified device path(s) or any child device paths.
///
#define EFI_USER_INFO_ACCESS_FORBID_CONNECT           0x00000007
///
/// Permits UEFI drivers to be started on the specified device path(s) or any child device paths.
///
#define EFI_USER_INFO_ACCESS_PERMIT_CONNECT           0x00000008
///
/// Modifies the boot order.
///
#define EFI_USER_INFO_ACCESS_BOOT_ORDER               0x00000009
typedef UINT32 EFI_USER_INFO_ACCESS_BOOT_ORDER_HDR;

#define EFI_USER_INFO_ACCESS_BOOT_ORDER_MASK          0x0000000F
///
/// Insert new boot options at the beginning of the boot order.
///
#define EFI_USER_INFO_ACCESS_BOOT_ORDER_INSERT        0x00000000
///
/// Append new boot options to the end of the boot order.
///
#define EFI_USER_INFO_ACCESS_BOOT_ORDER_APPEND        0x00000001
///
/// Replace the entire boot order.
///
#define EFI_USER_INFO_ACCESS_BOOT_ORDER_REPLACE       0x00000002
///
/// The Boot Manager will not attempt find a default boot device 
/// when the default boot order is does not lead to a bootable device.
///
#define EFI_USER_INFO_ACCESS_BOOT_ORDER_NODEFAULT     0x00000010

///
/// Provides the expression which determines which credentials are required to assert user identity.
///
#define EFI_USER_INFO_IDENTITY_POLICY_RECORD          0x0F

typedef struct {
  UINT32  Type;    ///< Specifies either an operator or a data item. 
  UINT32  Length;  ///< The length of this block, in bytes, including this header.
} EFI_USER_INFO_IDENTITY_POLICY;

///
/// User identity policy expression operators.
///
#define EFI_USER_INFO_IDENTITY_FALSE                  0x00
#define EFI_USER_INFO_IDENTITY_TRUE                   0x01
#define EFI_USER_INFO_IDENTITY_CREDENTIAL_TYPE        0x02
#define EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER    0x03
#define EFI_USER_INFO_IDENTITY_NOT                    0x10
#define EFI_USER_INFO_IDENTITY_AND                    0x11
#define EFI_USER_INFO_IDENTITY_OR                     0x12

///
/// Provides placeholder for additional user profile information identified by a GUID.
///
#define EFI_USER_INFO_GUID_RECORD                     0xFF
typedef EFI_GUID EFI_USER_INFO_GUID;

///
/// User information table
/// A collection of EFI_USER_INFO records, prefixed with this header.
///
typedef struct {
  UINT64   Size;  ///< Total size of the user information table, in bytes.
} EFI_USER_INFO_TABLE;

typedef struct _EFI_USER_CREDENTIAL_PROTOCOL  EFI_USER_CREDENTIAL_PROTOCOL;

/**
  Enroll a user on a credential provider.

  This function enrolls a user profile using this credential provider. If a user profile is successfully 
  enrolled, it calls the User Manager Protocol function Notify() to notify the user manager driver 
  that credential information has changed.

  @param[in] This                Points to this instance of the EFI_USER_CREDENTIAL_PROTOCOL.
  @param[in] User                The user profile to enroll.
 
  @retval EFI_SUCCESS            User profile was successfully enrolled.
  @retval EFI_ACCESS_DENIED      Current user profile does not permit enrollment on the user profile 
                                 handle. Either the user profile cannot enroll on any user profile or 
                                 cannot enroll on a user profile other than the current user profile.
  @retval EFI_UNSUPPORTED        This credential provider does not support enrollment in the pre-OS.
  @retval EFI_DEVICE_ERROR       The new credential could not be created because of a device error.
  @retval EFI_INVALID_PARAMETER  User does not refer to a valid user profile handle.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CREDENTIAL_ENROLL)(
  IN CONST EFI_USER_CREDENTIAL_PROTOCOL  *This,
  IN       EFI_USER_PROFILE_HANDLE       User
  );

/**
  Returns the user interface information used during user identification.

  This function returns information about the form used when interacting with the user during user 
  identification. The form is the first enabled form in the form-set class 
  EFI_HII_USER_CREDENTIAL_FORMSET_GUID installed on the HII handle HiiHandle. If 
  the user credential provider does not require a form to identify the user, then this function should 
  return EFI_NOT_FOUND.

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL_PROTOCOL.
  @param[out] Hii        On return, holds the HII database handle.
  @param[out] FormSetId  On return, holds the identifier of the form set which contains
                         the form used during user identification.
  @param[out] FormId     On return, holds the identifier of the form used during user identification.
 
  @retval EFI_SUCCESS    Form returned successfully.
  @retval EFI_NOT_FOUND  Form not returned.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CREDENTIAL_FORM)(
  IN CONST EFI_USER_CREDENTIAL_PROTOCOL  *This,
  OUT      EFI_HII_HANDLE                *Hii,
  OUT      EFI_GUID                      *FormSetId,
  OUT      EFI_FORM_ID                   *FormId
  );

/**
  Returns bitmap used to describe the credential provider type.

  This optional function returns a bitmap which is less than or equal to the number of pixels specified 
  by Width and Height. If no such bitmap exists, then EFI_NOT_FOUND is returned. 

  @param[in]     This    Points to this instance of the EFI_USER_CREDENTIAL_PROTOCOL.
  @param[in,out] Width   On entry, points to the desired bitmap width. If NULL then no bitmap information will 
                         be returned. On exit, points to the width of the bitmap returned.
  @param[in,out] Height  On entry, points to the desired bitmap height. If NULL then no bitmap information will 
                         be returned. On exit, points to the height of the bitmap returned
  @param[out]    Hii     On return, holds the HII database handle. 
  @param[out]    Image   On return, holds the HII image identifier. 
 
  @retval EFI_SUCCESS    Image identifier returned successfully.
  @retval EFI_NOT_FOUND  Image identifier not returned.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CREDENTIAL_TILE)(
  IN CONST EFI_USER_CREDENTIAL_PROTOCOL  *This,
  IN OUT   UINTN                         *Width,
  IN OUT   UINTN                         *Height,
  OUT      EFI_HII_HANDLE                *Hii,
  OUT      EFI_IMAGE_ID                  *Image
  );

/**
  Returns string used to describe the credential provider type.

  This function returns a string which describes the credential provider. If no such string exists, then 
  EFI_NOT_FOUND is returned. 

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL_PROTOCOL.
  @param[out] Hii        On return, holds the HII database handle.
  @param[out] String     On return, holds the HII string identifier.
 
  @retval EFI_SUCCESS    String identifier returned successfully.
  @retval EFI_NOT_FOUND  String identifier not returned.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CREDENTIAL_TITLE)(
  IN CONST EFI_USER_CREDENTIAL_PROTOCOL  *This,
  OUT      EFI_HII_HANDLE                *Hii,
  OUT      EFI_STRING_ID                 *String
  );

/**
  Return the user identifier associated with the currently authenticated user.

  This function returns the user identifier of the user authenticated by this credential provider. This 
  function is called after the credential-related information has been submitted on a form OR after a 
  call to Default() has returned that this credential is ready to log on.

  @param[in]  This           Points to this instance of the EFI_USER_CREDENTIAL_PROTOCOL.
  @param[in]  User           The user profile handle of the user profile currently being considered by the user 
                             identity manager. If NULL, then no user profile is currently under consideration.
  @param[out] Identifier     On return, points to the user identifier. 
 
  @retval EFI_SUCCESS        User identifier returned successfully.
  @retval EFI_NOT_READY      No user identifier can be returned.
  @retval EFI_ACCESS_DENIED  The user has been locked out of this user credential.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CREDENTIAL_USER)(
  IN CONST EFI_USER_CREDENTIAL_PROTOCOL  *This,
  IN       EFI_USER_PROFILE_HANDLE       User,
  OUT      EFI_USER_INFO_IDENTIFIER      *Identifier
  );

/**
  Indicate that user interface interaction has begun for the specified credential.

  This function is called when a credential provider is selected by the user. If AutoLogon returns 
  FALSE, then the user interface will be constructed by the User Identity Manager. 

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL_PROTOCOL.
  @param[out] AutoLogon  On return, points to the credential provider's capabilities after the credential provider 
                         has been selected by the user. 
 
  @retval EFI_SUCCESS    Credential provider successfully selected.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CREDENTIAL_SELECT)(
  IN CONST EFI_USER_CREDENTIAL_PROTOCOL  *This,
  OUT      EFI_CREDENTIAL_LOGON_FLAGS    *AutoLogon
  ); 

/**
  Indicate that user interface interaction has ended for the specified credential.

  This function is called when a credential provider is deselected by the user.

  @param[in] This        Points to this instance of the EFI_USER_CREDENTIAL_PROTOCOL.
 
  @retval EFI_SUCCESS    Credential provider successfully deselected.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CREDENTIAL_DESELECT)(
  IN CONST EFI_USER_CREDENTIAL_PROTOCOL  *This
  );

/**
  Return the default logon behavior for this user credential.

  This function reports the default login behavior regarding this credential provider.  

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL_PROTOCOL.
  @param[out] AutoLogon  On return, holds whether the credential provider should be used by default to 
                         automatically log on the user.  
 
  @retval EFI_SUCCESS    Default information successfully returned.
**/
typedef 
EFI_STATUS
(EFIAPI *EFI_CREDENTIAL_DEFAULT)(
  IN  CONST EFI_USER_CREDENTIAL_PROTOCOL        *This,
  OUT EFI_CREDENTIAL_LOGON_FLAGS                *AutoLogon
  );

/**
  Return information attached to the credential provider.

  This function returns user information. 

  @param[in]     This           Points to this instance of the EFI_USER_CREDENTIAL_PROTOCOL.
  @param[in]     UserInfo       Handle of the user information data record. 
  @param[out]    Info           On entry, points to a buffer of at least *InfoSize bytes. On exit, holds the user 
                                information. If the buffer is too small to hold the information, then 
                                EFI_BUFFER_TOO_SMALL is returned and InfoSize is updated to contain the 
                                number of bytes actually required.
  @param[in,out] InfoSize       On entry, points to the size of Info. On return, points to the size of the user 
                                information. 
 
  @retval EFI_SUCCESS           Information returned successfully.
  @retval EFI_ACCESS_DENIED     The information about the specified user cannot be accessed by the 
                                current user.
  @retval EFI_BUFFER_TOO_SMALL  The size specified by InfoSize is too small to hold all of the user 
                                information. The size required is returned in *InfoSize.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CREDENTIAL_GET_INFO)(
  IN CONST EFI_USER_CREDENTIAL_PROTOCOL  *This,
  IN       EFI_USER_INFO_HANDLE          UserInfo,
  OUT      EFI_USER_INFO                 *Info,
  IN OUT   UINTN                         *InfoSize
  );

/**
  Enumerate all of the enrolled users on the platform.

  This function returns the next user information record. To retrieve the first user information record 
  handle, point UserInfo at a NULL. Each subsequent call will retrieve another user information 
  record handle until there are no more, at which point UserInfo will point to NULL. 

  @param[in]     This      Points to this instance of the EFI_USER_CREDENTIAL_PROTOCOL.
  @param[in,out] UserInfo  On entry, points to the previous user information handle or NULL to start 
                           enumeration. On exit, points to the next user information handle or NULL if there is 
                           no more user information.
 
  @retval EFI_SUCCESS      User information returned.
  @retval EFI_NOT_FOUND    No more user information found.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CREDENTIAL_GET_NEXT_INFO)(
  IN CONST EFI_USER_CREDENTIAL_PROTOCOL  *This,
  IN OUT   EFI_USER_INFO_HANDLE          *UserInfo
  );

///
/// This protocol provides support for a single class of credentials
///
struct _EFI_USER_CREDENTIAL_PROTOCOL {
  EFI_GUID                      Identifier;  ///< Uniquely identifies this credential provider.
  EFI_GUID                      Type;        ///< Identifies this class of User Credential Provider.
  EFI_CREDENTIAL_ENROLL         Enroll;
  EFI_CREDENTIAL_FORM           Form;
  EFI_CREDENTIAL_TILE           Tile;
  EFI_CREDENTIAL_TITLE          Title;
  EFI_CREDENTIAL_USER           User;
  EFI_CREDENTIAL_SELECT         Select;   
  EFI_CREDENTIAL_DESELECT       Deselect;
  EFI_CREDENTIAL_DEFAULT        Default;
  EFI_CREDENTIAL_GET_INFO       GetInfo;
  EFI_CREDENTIAL_GET_NEXT_INFO  GetNextInfo;
  EFI_CREDENTIAL_CAPABILITIES   Capabilities;
};

extern EFI_GUID gEfiUserCredentialProtocolGuid;

#endif
