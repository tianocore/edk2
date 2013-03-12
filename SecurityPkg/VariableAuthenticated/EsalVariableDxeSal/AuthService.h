/** @file
  The internal header file includes the common header files, defines
  internal structure and functions used by AuthService module.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
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
/// Size of AuthInfo prior to the data payload
///
#define AUTHINFO_SIZE (((UINTN)(((EFI_VARIABLE_AUTHENTICATION *) 0)->AuthInfo.CertData)) + sizeof (EFI_CERT_BLOCK_RSA_2048_SHA256))

///
/// Item number of support signature types.
///
#define SIGSUPPORT_NUM 2

/**
  Process variable with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set, and return the index of associated public key.

  @param[in]  Data                        The data pointer.
  @param[in]  DataSize                    The size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  VirtualMode                 The current calling mode for this function.
  @param[in]  Global                      The context of this Extended SAL Variable Services Class call.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  The attribute value of the variable.
  @param[out] KeyIndex                    The output index of corresponding public key in database.
  @param[out] MonotonicCount              The output value of corresponding Monotonic Count.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_WRITE_PROTECTED             The variable is write-protected and needs authentication with
                                          EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set.
  @retval EFI_SECURITY_VIOLATION          The variable is with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS
                                          set, but the AuthInfo does NOT pass the validation 
                                          check carried out by the firmware. 
  @retval EFI_SUCCESS                     The variable is not write-protected, or passed validation successfully.

**/
EFI_STATUS
VerifyVariable (
  IN  VOID                      *Data,
  IN  UINTN                     DataSize,
  IN  BOOLEAN                   VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL      *Global,
  IN  VARIABLE_POINTER_TRACK    *Variable,
  IN  UINT32                    Attributes OPTIONAL,
  OUT UINT32                    *KeyIndex OPTIONAL,
  OUT UINT64                    *MonotonicCount OPTIONAL
  );

/**
  Initializes for authenticated varibale service.

  @retval EFI_SUCCESS           The function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate enough memory resources.

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
  Process variable with platform key for verification.

  @param[in]  VariableName                The name of Variable to be found.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        The data pointer.
  @param[in]  DataSize                    The size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  VirtualMode                 The current calling mode for this function.
  @param[in]  Global                      The context of this Extended SAL Variable Services Class call.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  The attribute value of the variable.
  @param[in]  IsPk                        Indicates whether to process pk.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_SECURITY_VIOLATION          The variable does NOT pass the validation 
                                          check carried out by the firmware. 
  @retval EFI_SUCCESS                     The variable passed validation successfully.

**/
EFI_STATUS
ProcessVarWithPk (
  IN  CHAR16                    *VariableName,
  IN  EFI_GUID                  *VendorGuid,
  IN  VOID                      *Data,
  IN  UINTN                     DataSize,
  IN  BOOLEAN                   VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL      *Global,
  IN  VARIABLE_POINTER_TRACK    *Variable,
  IN  UINT32                    Attributes OPTIONAL,
  IN  BOOLEAN                   IsPk
  );

/**
  Process variable with key exchange key for verification.

  @param[in]  VariableName                The name of Variable to be found.
  @param[in]  VendorGuid                  The variable vendor GUID.
  @param[in]  Data                        The data pointer.
  @param[in]  DataSize                    Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  VirtualMode                 The current calling mode for this function.
  @param[in]  Global                      The context of this Extended SAL Variable Services Class call.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  The attribute value of the variable.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_SECURITY_VIOLATION          The variable does NOT pass the validation 
                                          check carried out by the firmware. 
  @retval EFI_SUCCESS                     The variable passed validation successfully.

**/
EFI_STATUS
ProcessVarWithKek (
  IN  CHAR16                               *VariableName,
  IN  EFI_GUID                             *VendorGuid,
  IN  VOID                                 *Data,
  IN  UINTN                                DataSize,
  IN  BOOLEAN                              VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL                 *Global,
  IN  VARIABLE_POINTER_TRACK               *Variable,
  IN  UINT32                               Attributes OPTIONAL
  );

#endif
