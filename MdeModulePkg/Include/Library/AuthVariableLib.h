/** @file
  Provides services to initialize and process authenticated variables.

Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _AUTH_VARIABLE_LIB_H_
#define _AUTH_VARIABLE_LIB_H_

#include <Protocol/VarCheck.h>

///
/// Size of AuthInfo prior to the data payload.
///
#define AUTHINFO_SIZE  ((OFFSET_OF (EFI_VARIABLE_AUTHENTICATION, AuthInfo)) +\
                       (OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)) + \
                       sizeof (EFI_CERT_BLOCK_RSA_2048_SHA256))

#define AUTHINFO2_SIZE(VarAuth2)  ((OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo)) +\
                                  (UINTN) ((EFI_VARIABLE_AUTHENTICATION_2 *) (VarAuth2))->AuthInfo.Hdr.dwLength)

#define OFFSET_OF_AUTHINFO2_CERT_DATA  ((OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo)) +\
                                       (OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)))

typedef struct {
  CHAR16      *VariableName;
  EFI_GUID    *VendorGuid;
  UINT32      Attributes;
  UINTN       DataSize;
  VOID        *Data;
  UINT32      PubKeyIndex;
  UINT64      MonotonicCount;
  EFI_TIME    *TimeStamp;
} AUTH_VARIABLE_INFO;

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.

  @param[in]  VariableName          Name of the variable to be found.
  @param[in]  VendorGuid            Variable vendor GUID to be found.
  @param[out] AuthVariableInfo      Pointer to AUTH_VARIABLE_INFO structure for
                                    output of the variable found.

  @retval EFI_INVALID_PARAMETER     If VariableName is not an empty string,
                                    while VendorGuid is NULL.
  @retval EFI_SUCCESS               Variable successfully found.
  @retval EFI_NOT_FOUND             Variable not found

**/
typedef
EFI_STATUS
(EFIAPI *AUTH_VAR_LIB_FIND_VARIABLE)(
  IN  CHAR16                *VariableName,
  IN  EFI_GUID              *VendorGuid,
  OUT AUTH_VARIABLE_INFO    *AuthVariableInfo
  );

/**
  Finds next variable in storage blocks of volatile and non-volatile storage areas.

  This code finds next variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.

  @param[in]  VariableName          Name of the variable to be found.
  @param[in]  VendorGuid            Variable vendor GUID to be found.
  @param[out] AuthVariableInfo      Pointer to AUTH_VARIABLE_INFO structure for
                                    output of the next variable.

  @retval EFI_INVALID_PARAMETER     If VariableName is not an empty string,
                                    while VendorGuid is NULL.
  @retval EFI_SUCCESS               Variable successfully found.
  @retval EFI_NOT_FOUND             Variable not found

**/
typedef
EFI_STATUS
(EFIAPI *AUTH_VAR_LIB_FIND_NEXT_VARIABLE)(
  IN  CHAR16                *VariableName,
  IN  EFI_GUID              *VendorGuid,
  OUT AUTH_VARIABLE_INFO    *AuthVariableInfo
  );

/**
  Update the variable region with Variable information.

  @param[in] AuthVariableInfo       Pointer AUTH_VARIABLE_INFO structure for
                                    input of the variable.

  @retval EFI_SUCCESS               The update operation is success.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_WRITE_PROTECTED       Variable is write-protected.
  @retval EFI_OUT_OF_RESOURCES      There is not enough resource.

**/
typedef
EFI_STATUS
(EFIAPI *AUTH_VAR_LIB_UPDATE_VARIABLE)(
  IN AUTH_VARIABLE_INFO     *AuthVariableInfo
  );

/**
  Get scratch buffer.

  @param[in, out] ScratchBufferSize Scratch buffer size. If input size is greater than
                                    the maximum supported buffer size, this value contains
                                    the maximum supported buffer size as output.
  @param[out]     ScratchBuffer     Pointer to scratch buffer address.

  @retval EFI_SUCCESS       Get scratch buffer successfully.
  @retval EFI_UNSUPPORTED   If input size is greater than the maximum supported buffer size.

**/
typedef
EFI_STATUS
(EFIAPI *AUTH_VAR_LIB_GET_SCRATCH_BUFFER)(
  IN OUT UINTN      *ScratchBufferSize,
  OUT    VOID       **ScratchBuffer
  );

/**
  This function is to check if the remaining variable space is enough to set
  all Variables from argument list successfully. The purpose of the check
  is to keep the consistency of the Variables to be in variable storage.

  Note: Variables are assumed to be in same storage.
  The set sequence of Variables will be same with the sequence of VariableEntry from argument list,
  so follow the argument sequence to check the Variables.

  @param[in] Attributes         Variable attributes for Variable entries.
  @param ...                    The variable argument list with type VARIABLE_ENTRY_CONSISTENCY *.
                                A NULL terminates the list. The VariableSize of
                                VARIABLE_ENTRY_CONSISTENCY is the variable data size as input.
                                It will be changed to variable total size as output.

  @retval TRUE                  Have enough variable space to set the Variables successfully.
  @retval FALSE                 No enough variable space to set the Variables successfully.

**/
typedef
BOOLEAN
(EFIAPI *AUTH_VAR_LIB_CHECK_REMAINING_SPACE)(
  IN UINT32                     Attributes,
  ...
  );

/**
  Return TRUE if at OS runtime.

  @retval TRUE If at OS runtime.
  @retval FALSE If at boot time.

**/
typedef
BOOLEAN
(EFIAPI *AUTH_VAR_LIB_AT_RUNTIME)(
  VOID
  );

#define AUTH_VAR_LIB_CONTEXT_IN_STRUCT_VERSION  0x01

typedef struct {
  UINTN                                 StructVersion;
  UINTN                                 StructSize;
  //
  // Reflect the overhead associated with the saving
  // of a single EFI authenticated variable with the exception
  // of the overhead associated with the length
  // of the string name of the EFI variable.
  //
  UINTN                                 MaxAuthVariableSize;
  AUTH_VAR_LIB_FIND_VARIABLE            FindVariable;
  AUTH_VAR_LIB_FIND_NEXT_VARIABLE       FindNextVariable;
  AUTH_VAR_LIB_UPDATE_VARIABLE          UpdateVariable;
  AUTH_VAR_LIB_GET_SCRATCH_BUFFER       GetScratchBuffer;
  AUTH_VAR_LIB_CHECK_REMAINING_SPACE    CheckRemainingSpaceForConsistency;
  AUTH_VAR_LIB_AT_RUNTIME               AtRuntime;
} AUTH_VAR_LIB_CONTEXT_IN;

#define AUTH_VAR_LIB_CONTEXT_OUT_STRUCT_VERSION  0x01

typedef struct {
  UINTN                      StructVersion;
  UINTN                      StructSize;
  //
  // Caller needs to set variable property for the variables.
  //
  VARIABLE_ENTRY_PROPERTY    *AuthVarEntry;
  UINTN                      AuthVarEntryCount;
  //
  // Caller needs to ConvertPointer() for the pointers.
  //
  VOID                       ***AddressPointer;
  UINTN                      AddressPointerCount;
} AUTH_VAR_LIB_CONTEXT_OUT;

/**
  Initialization for authenticated varibale services.
  If this initialization returns error status, other APIs will not work
  and expect to be not called then.

  @param[in]  AuthVarLibContextIn   Pointer to input auth variable lib context.
  @param[out] AuthVarLibContextOut  Pointer to output auth variable lib context.

  @retval EFI_SUCCESS               Function successfully executed.
  @retval EFI_INVALID_PARAMETER     If AuthVarLibContextIn == NULL or AuthVarLibContextOut == NULL.
  @retval EFI_OUT_OF_RESOURCES      Fail to allocate enough resource.
  @retval EFI_UNSUPPORTED           Unsupported to process authenticated variable.

**/
EFI_STATUS
EFIAPI
AuthVariableLibInitialize (
  IN  AUTH_VAR_LIB_CONTEXT_IN   *AuthVarLibContextIn,
  OUT AUTH_VAR_LIB_CONTEXT_OUT  *AuthVarLibContextOut
  );

/**
  Process variable with EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set.

  @param[in] VariableName           Name of the variable.
  @param[in] VendorGuid             Variable vendor GUID.
  @param[in] Data                   Data pointer.
  @param[in] DataSize               Size of Data.
  @param[in] Attributes             Attribute value of the variable.

  @retval EFI_SUCCESS               The firmware has successfully stored the variable and its data as
                                    defined by the Attributes.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_WRITE_PROTECTED       Variable is write-protected.
  @retval EFI_OUT_OF_RESOURCES      There is not enough resource.
  @retval EFI_SECURITY_VIOLATION    The variable is with EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS
                                    set, but the AuthInfo does NOT pass the validation
                                    check carried out by the firmware.
  @retval EFI_UNSUPPORTED           Unsupported to process authenticated variable.

**/
EFI_STATUS
EFIAPI
AuthVariableLibProcessVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN VOID      *Data,
  IN UINTN     DataSize,
  IN UINT32    Attributes
  );

#endif
