/** @file
  The internal header file includes the common header files, defines
  internal structure and functions used by AuthService module.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _AUTHSERVICE_H_
#define _AUTHSERVICE_H_

#define EFI_CERT_TYPE_RSA2048_SHA256_SIZE 256
#define EFI_CERT_TYPE_RSA2048_SIZE        256

///
/// Size of AuthInfo prior to the data payload.
///
#define AUTHINFO_SIZE ((OFFSET_OF (EFI_VARIABLE_AUTHENTICATION, AuthInfo)) + \
                       (OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)) + \
                       sizeof (EFI_CERT_BLOCK_RSA_2048_SHA256))

#define AUTHINFO2_SIZE(VarAuth2) ((OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo)) + \
                                  (UINTN) ((EFI_VARIABLE_AUTHENTICATION_2 *) (VarAuth2))->AuthInfo.Hdr.dwLength)

#define OFFSET_OF_AUTHINFO2_CERT_DATA ((OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo)) + \
                                       (OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)))

///
/// "AuthVarKeyDatabase" variable for the Public Key store.
///
#define AUTHVAR_KEYDB_NAME      L"AuthVarKeyDatabase"
#define AUTHVAR_KEYDB_NAME_SIZE 38

///
/// Max size of public key database, restricted by max individal EFI varible size, exclude variable header and name size.
///
#define MAX_KEYDB_SIZE  (FixedPcdGet32 (PcdMaxVariableSize) - sizeof (VARIABLE_HEADER) - AUTHVAR_KEYDB_NAME_SIZE)
#define MAX_KEY_NUM     (MAX_KEYDB_SIZE / EFI_CERT_TYPE_RSA2048_SIZE)

///
/// Struct to record signature requirement defined by UEFI spec.
/// For SigHeaderSize and SigDataSize, ((UINT32) ~0) means NO exact length requirement for this field.
///
typedef struct {
  EFI_GUID    SigType;
  // Expected SignatureHeader size in Bytes.
  UINT32      SigHeaderSize;
  // Expected SignatureData size in Bytes.
  UINT32      SigDataSize;
} EFI_SIGNATURE_ITEM;

typedef enum {
  AuthVarTypePk,
  AuthVarTypeKek,
  AuthVarTypePriv
} AUTHVAR_TYPE;

#pragma pack(1)
typedef struct {
  EFI_GUID    VendorGuid;
  UINT32      CertNodeSize;
  UINT32      NameSize;
  UINT32      CertDataSize;
  /// CHAR16  VariableName[NameSize];
  /// UINT8   CertData[CertDataSize];
} AUTH_CERT_DB_DATA;
#pragma pack()

/**
  Process variable with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS/EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param[in]  VariableName                Name of Variable to be found.
  @param[in]  VendorGuid                  Variable vendor GUID.

  @param[in]  Data                        Data pointer.
  @param[in]  DataSize                    Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  Attribute value of the variable.

  @return EFI_INVALID_PARAMETER           Invalid parameter
  @return EFI_WRITE_PROTECTED             Variable is write-protected and needs authentication with
                                          EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set.
  @return EFI_SECURITY_VIOLATION          The variable is with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS
                                          set, but the AuthInfo does NOT pass the validation
                                          check carried out by the firmware.
  @return EFI_SUCCESS                     Variable is not write-protected, or passed validation successfully.

**/
EFI_STATUS
ProcessVariable (
  IN     CHAR16                             *VariableName,
  IN     EFI_GUID                           *VendorGuid,
  IN     VOID                               *Data,
  IN     UINTN                              DataSize,
  IN     VARIABLE_POINTER_TRACK             *Variable,
  IN     UINT32                             Attributes
  );

/**
  Update platform mode.

  @param[in]      Mode                    SETUP_MODE or USER_MODE.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Update platform mode successfully.

**/
EFI_STATUS
UpdatePlatformMode (
  IN  UINT32                    Mode
  );

/**
  Initializes for authenticated varibale service.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
AutenticatedVariableServiceInitialize (
  VOID
  );

/**
  Initializes for cryptlib service before use, include register algrithm and allocate scratch.

**/
VOID
CryptLibraryInitialize (
  VOID
  );

/**
  Check input data form to make sure it is a valid EFI_SIGNATURE_LIST for PK/KEK variable.

  @param[in]  VariableName                Name of Variable to be check.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Point to the variable data to be checked.
  @param[in]  DataSize                    Size of Data.

  @return EFI_INVALID_PARAMETER           Invalid signature list format.
  @return EFI_SUCCESS                     Passed signature list format check successfully.
  
**/
EFI_STATUS
CheckSignatureListFormat(
  IN  CHAR16                    *VariableName,
  IN  EFI_GUID                  *VendorGuid,
  IN  VOID                      *Data,
  IN  UINTN                     DataSize
  );

/**
  Process variable with platform key for verification.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param[in]  VariableName                Name of Variable to be found.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Data pointer.
  @param[in]  DataSize                    Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  Attribute value of the variable.
  @param[in]  IsPk                        Indicate whether it is to process pk.

  @return EFI_INVALID_PARAMETER           Invalid parameter
  @return EFI_SECURITY_VIOLATION          The variable does NOT pass the validation
                                          check carried out by the firmware.
  @return EFI_SUCCESS                     Variable passed validation successfully.

**/
EFI_STATUS
ProcessVarWithPk (
  IN  CHAR16                    *VariableName,
  IN  EFI_GUID                  *VendorGuid,
  IN  VOID                      *Data,
  IN  UINTN                     DataSize,
  IN  VARIABLE_POINTER_TRACK    *Variable,
  IN  UINT32                    Attributes OPTIONAL,
  IN  BOOLEAN                   IsPk
  );

/**
  Process variable with key exchange key for verification.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param[in]  VariableName                Name of Variable to be found.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Data pointer.
  @param[in]  DataSize                    Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  Variable                    The variable information that is used to keep track of variable usage.
  @param[in]  Attributes                  Attribute value of the variable.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SECURITY_VIOLATION          The variable does NOT pass the validation
                                          check carried out by the firmware.
  @return EFI_SUCCESS                     Variable passed validation successfully.

**/
EFI_STATUS
ProcessVarWithKek (
  IN  CHAR16                    *VariableName,
  IN  EFI_GUID                  *VendorGuid,
  IN  VOID                      *Data,
  IN  UINTN                     DataSize,
  IN  VARIABLE_POINTER_TRACK    *Variable,
  IN  UINT32                    Attributes OPTIONAL
  );

/**
  Merge two buffers which formatted as EFI_SIGNATURE_LIST. Only the new EFI_SIGNATURE_DATA
  will be appended to the original EFI_SIGNATURE_LIST, duplicate EFI_SIGNATURE_DATA
  will be ignored.

  @param[in, out]  Data            Pointer to original EFI_SIGNATURE_LIST.
  @param[in]       DataSize        Size of Data buffer.
  @param[in]       NewData         Pointer to new EFI_SIGNATURE_LIST to be appended.
  @param[in]       NewDataSize     Size of NewData buffer.

  @return Size of the merged buffer.

**/
UINTN
AppendSignatureList (
  IN  OUT VOID            *Data,
  IN  UINTN               DataSize,
  IN  VOID                *NewData,
  IN  UINTN               NewDataSize
  );

/**
  Compare two EFI_TIME data.


  @param FirstTime           A pointer to the first EFI_TIME data.
  @param SecondTime          A pointer to the second EFI_TIME data.

  @retval  TRUE              The FirstTime is not later than the SecondTime.
  @retval  FALSE             The FirstTime is later than the SecondTime.

**/
BOOLEAN
CompareTimeStamp (
  IN EFI_TIME               *FirstTime,
  IN EFI_TIME               *SecondTime
  );


/**
  Process variable with EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.

  @param[in]  VariableName                Name of Variable to be found.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Data pointer.
  @param[in]  DataSize                    Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  Attribute value of the variable.
  @param[in]  AuthVarType                 Verify against PK or KEK database or private database.
  @param[out] VarDel                      Delete the variable or not.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_SECURITY_VIOLATION          The variable does NOT pass the validation
                                          check carried out by the firmware.
  @retval EFI_OUT_OF_RESOURCES            Failed to process variable due to lack
                                          of resources.
  @retval EFI_SUCCESS                     Variable pass validation successfully.

**/
EFI_STATUS
VerifyTimeBasedPayload (
  IN     CHAR16                             *VariableName,
  IN     EFI_GUID                           *VendorGuid,
  IN     VOID                               *Data,
  IN     UINTN                              DataSize,
  IN     VARIABLE_POINTER_TRACK             *Variable,
  IN     UINT32                             Attributes,
  IN     AUTHVAR_TYPE                       AuthVarType,
  OUT    BOOLEAN                            *VarDel
  );

extern UINT8  mPubKeyStore[MAX_KEYDB_SIZE];
extern UINT32 mPubKeyNumber;
extern VOID   *mHashCtx;
extern VOID   *mStorageArea;
extern UINT8  *mSerializationRuntimeBuffer;

#endif
