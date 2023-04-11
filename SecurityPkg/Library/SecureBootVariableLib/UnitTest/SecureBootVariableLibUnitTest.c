/** @file
  Unit tests of the implementation of SecureBootVariableLib.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <UefiSecureBoot.h>
#include <Guid/GlobalVariable.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/ImageAuthentication.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/UnitTestLib.h>
#include <Library/SecureBootVariableLib.h>

#define UNIT_TEST_APP_NAME     "SecureBootVariableLib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"
#define VAR_AUTH_DESC_SIZE     OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo) + OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)

extern EFI_TIME  mMaxTimestamp;
extern EFI_TIME  mDefaultPayloadTimestamp;

/**
  Sets the value of a variable.

  @param[in]  VariableName       A Null-terminated string that is the name of the vendor's variable.
                                 Each VariableName is unique for each VendorGuid. VariableName must
                                 contain 1 or more characters. If VariableName is an empty string,
                                 then EFI_INVALID_PARAMETER is returned.
  @param[in]  VendorGuid         A unique identifier for the vendor.
  @param[in]  Attributes         Attributes bitmask to set for the variable.
  @param[in]  DataSize           The size in bytes of the Data buffer. Unless the EFI_VARIABLE_APPEND_WRITE or
                                 EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a size of zero
                                 causes the variable to be deleted. When the EFI_VARIABLE_APPEND_WRITE attribute is
                                 set, then a SetVariable() call with a DataSize of zero will not cause any change to
                                 the variable value (the timestamp associated with the variable may be updated however
                                 even if no new data value is provided,see the description of the
                                 EFI_VARIABLE_AUTHENTICATION_2 descriptor below. In this case the DataSize will not
                                 be zero since the EFI_VARIABLE_AUTHENTICATION_2 descriptor will be populated).
  @param[in]  Data               The contents for the variable.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits, name, and GUID was supplied, or the
                                 DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS being set,
                                 but the AuthInfo does NOT pass the validation check carried out by the firmware.

  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.

**/
STATIC
EFI_STATUS
EFIAPI
MockSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  DEBUG ((
    DEBUG_INFO,
    "%a %s %g %x %x %p\n",
    __func__,
    VariableName,
    VendorGuid,
    Attributes,
    DataSize,
    Data
    ));
  check_expected_ptr (VariableName);
  check_expected_ptr (VendorGuid);
  check_expected_ptr (Attributes);
  check_expected (DataSize);
  check_expected (Data);

  return (EFI_STATUS)mock ();
}

/**
  Returns the value of a variable.

  @param[in]       VariableName  A Null-terminated string that is the name of the vendor's
                                 variable.
  @param[in]       VendorGuid    A unique identifier for the vendor.
  @param[out]      Attributes    If not NULL, a pointer to the memory location to return the
                                 attributes bitmask for the variable.
  @param[in, out]  DataSize      On input, the size in bytes of the return Data buffer.
                                 On output the size of data returned in Data.
  @param[out]      Data          The buffer to return the contents of the variable. May be NULL
                                 with a zero DataSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER  VariableName is NULL.
  @retval EFI_INVALID_PARAMETER  VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER  DataSize is NULL.
  @retval EFI_INVALID_PARAMETER  The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION The variable could not be retrieved due to an authentication failure.

**/
STATIC
EFI_STATUS
EFIAPI
MockGetVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes     OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data           OPTIONAL
  )
{
  UINTN    TargetSize;
  BOOLEAN  Exist;

  DEBUG ((
    DEBUG_INFO,
    "%a %s %g %p %x %p\n",
    __func__,
    VariableName,
    VendorGuid,
    Attributes,
    *DataSize,
    Data
    ));
  assert_non_null (DataSize);
  check_expected_ptr (VariableName);
  check_expected_ptr (VendorGuid);
  check_expected (*DataSize);

  Exist = (BOOLEAN)mock ();

  if (!Exist) {
    return EFI_NOT_FOUND;
  }

  TargetSize = (UINTN)mock ();
  if (TargetSize > *DataSize) {
    *DataSize = TargetSize;
    return EFI_BUFFER_TOO_SMALL;
  } else {
    assert_non_null (Data);
    CopyMem (Data, (VOID *)(UINTN)mock (), TargetSize);
  }

  return EFI_SUCCESS;
}

///
/// Mock version of the UEFI Runtime Services Table
///
EFI_RUNTIME_SERVICES  gMockRuntime = {
  {
    EFI_RUNTIME_SERVICES_SIGNATURE,     // Signature
    EFI_RUNTIME_SERVICES_REVISION,      // Revision
    sizeof (EFI_RUNTIME_SERVICES),      // HeaderSize
    0,                                  // CRC32
    0                                   // Reserved
  },
  NULL,               // GetTime
  NULL,               // SetTime
  NULL,               // GetWakeupTime
  NULL,               // SetWakeupTime
  NULL,               // SetVirtualAddressMap
  NULL,               // ConvertPointer
  MockGetVariable,    // GetVariable
  NULL,               // GetNextVariableName
  MockSetVariable,    // SetVariable
  NULL,               // GetNextHighMonotonicCount
  NULL,               // ResetSystem
  NULL,               // UpdateCapsule
  NULL,               // QueryCapsuleCapabilities
  NULL                // QueryVariableInfo
};

/**
  Unit test for SetSecureBootMode () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SetSecureBootModeShouldSetVar (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8       SecureBootMode;
  EFI_STATUS  Status;

  SecureBootMode = 0xAB; // Any random magic number...
  expect_memory (MockSetVariable, VariableName, EFI_CUSTOM_MODE_NAME, sizeof (EFI_CUSTOM_MODE_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiCustomModeEnableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS);
  expect_value (MockSetVariable, DataSize, sizeof (SecureBootMode));
  expect_memory (MockSetVariable, Data, &SecureBootMode, sizeof (SecureBootMode));

  will_return (MockSetVariable, EFI_SUCCESS);

  Status = SetSecureBootMode (SecureBootMode);

  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for GetSetupMode () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
GetSetupModeShouldGetVar (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       TargetMode;
  UINT8       SetupMode;

  TargetMode = 0xAB; // Any random magic number...
  expect_memory (MockGetVariable, VariableName, EFI_SETUP_MODE_NAME, sizeof (EFI_SETUP_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (SetupMode));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (SetupMode));
  will_return (MockGetVariable, &TargetMode);

  Status = GetSetupMode (&SetupMode);

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (SetupMode, TargetMode);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for GetSetupMode () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
IsSecureBootEnableShouldGetVar (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Enabled;
  UINT8    TargetMode;

  TargetMode = SECURE_BOOT_MODE_ENABLE;
  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (TargetMode));

  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (TargetMode));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (TargetMode));
  will_return (MockGetVariable, &TargetMode);

  Enabled = IsSecureBootEnabled ();

  UT_ASSERT_EQUAL (Enabled, SECURE_BOOT_MODE_ENABLE);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SecureBootCreateDataFromInput () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SecureBootCreateDataFromInputSimple (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_SIGNATURE_LIST            *SigList    = NULL;
  EFI_SIGNATURE_DATA            *SigData    = NULL;
  UINTN                         SigListSize = 0;
  EFI_STATUS                    Status;
  UINT8                         TestData[] = { 0 };
  SECURE_BOOT_CERTIFICATE_INFO  KeyInfo;

  KeyInfo.Data     = TestData;
  KeyInfo.DataSize = sizeof (TestData);

  Status = SecureBootCreateDataFromInput (&SigListSize, &SigList, 1, &KeyInfo);

  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_NOT_NULL (SigList);
  UT_ASSERT_TRUE (CompareGuid (&SigList->SignatureType, &gEfiCertX509Guid));
  UT_ASSERT_EQUAL (SigList->SignatureSize, sizeof (EFI_SIGNATURE_DATA) - 1 + sizeof (TestData));
  UT_ASSERT_EQUAL (SigList->SignatureHeaderSize, 0);
  UT_ASSERT_EQUAL (SigList->SignatureListSize, sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + sizeof (TestData));
  UT_ASSERT_EQUAL (SigList->SignatureListSize, SigListSize);

  SigData = (EFI_SIGNATURE_DATA *)((UINTN)SigList + sizeof (EFI_SIGNATURE_LIST));
  UT_ASSERT_TRUE (CompareGuid (&SigData->SignatureOwner, &gEfiGlobalVariableGuid));
  UT_ASSERT_MEM_EQUAL (SigData->SignatureData, TestData, sizeof (TestData));

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SecureBootCreateDataFromInput () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SecureBootCreateDataFromInputNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_SIGNATURE_LIST            *SigList    = NULL;
  UINTN                         SigListSize = 0;
  EFI_STATUS                    Status;
  SECURE_BOOT_CERTIFICATE_INFO  KeyInfo = {
    .Data     = NULL,
    .DataSize = 0
  };

  Status = SecureBootCreateDataFromInput (&SigListSize, &SigList, 0, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  Status = SecureBootCreateDataFromInput (&SigListSize, &SigList, 1, &KeyInfo);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SecureBootCreateDataFromInput () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SecureBootCreateDataFromInputMultiple (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_SIGNATURE_LIST            *SigList    = NULL;
  EFI_SIGNATURE_DATA            *SigData    = NULL;
  UINTN                         SigListSize = 0;
  UINTN                         TotalSize   = 0;
  UINTN                         Index       = 0;
  UINT8                         TestData1[] = { 0 };
  UINT8                         TestData2[] = { 1, 2 };
  EFI_STATUS                    Status;
  SECURE_BOOT_CERTIFICATE_INFO  KeyInfo[2];

  KeyInfo[0].Data     = TestData1;
  KeyInfo[0].DataSize = sizeof (TestData1);
  KeyInfo[1].Data     = TestData2;
  KeyInfo[1].DataSize = sizeof (TestData2);

  Status = SecureBootCreateDataFromInput (&SigListSize, &SigList, 2, KeyInfo);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_NOT_NULL (SigList);

  for (Index = 0; Index < 2; Index++) {
    UT_ASSERT_TRUE (SigListSize > TotalSize);

    UT_ASSERT_TRUE (CompareGuid (&SigList->SignatureType, &gEfiCertX509Guid));
    UT_ASSERT_EQUAL (SigList->SignatureSize, sizeof (EFI_SIGNATURE_DATA) - 1 + KeyInfo[Index].DataSize);
    UT_ASSERT_EQUAL (SigList->SignatureHeaderSize, 0);
    UT_ASSERT_EQUAL (SigList->SignatureListSize, sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + KeyInfo[Index].DataSize);

    SigData = (EFI_SIGNATURE_DATA *)((UINTN)SigList + sizeof (EFI_SIGNATURE_LIST));
    UT_ASSERT_TRUE (CompareGuid (&SigData->SignatureOwner, &gEfiGlobalVariableGuid));
    UT_ASSERT_MEM_EQUAL (SigData->SignatureData, KeyInfo[Index].Data, KeyInfo[Index].DataSize);
    TotalSize = TotalSize + SigList->SignatureListSize;
    SigList   = (EFI_SIGNATURE_LIST *)((UINTN)SigList + SigList->SignatureListSize);
  }

  UT_ASSERT_EQUAL (SigListSize, TotalSize);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for CreateTimeBasedPayload () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
CreateTimeBasedPayloadShouldPopulateDescriptor (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8                          Data[]   = { 2 };
  UINTN                          DataSize = sizeof (Data);
  UINT8                          *CheckData;
  EFI_VARIABLE_AUTHENTICATION_2  *VarAuth;
  EFI_STATUS                     Status;
  EFI_TIME                       Time = {
    .Year       = 2012,
    .Month      = 3,
    .Day        = 4,
    .Hour       = 5,
    .Minute     = 6,
    .Second     = 7,
    .Pad1       = 0,
    .Nanosecond = 8910,
    .TimeZone   = 1112,
    .Pad2       = 0
  };

  CheckData = AllocateCopyPool (DataSize, Data);
  Status    = CreateTimeBasedPayload (&DataSize, &CheckData, &Time);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // This is result that we did not pack this structure...
  // we cannot even use the sizeof (EFI_VARIABLE_AUTHENTICATION_2) - 1,
  // because the structure is not at the end of this structure, but partially
  // inside it...
  UT_ASSERT_EQUAL (DataSize, VAR_AUTH_DESC_SIZE + sizeof (Data));
  UT_ASSERT_NOT_NULL (CheckData);

  VarAuth = (EFI_VARIABLE_AUTHENTICATION_2 *)CheckData;
  UT_ASSERT_MEM_EQUAL (&(VarAuth->TimeStamp), &Time, sizeof (EFI_TIME));

  UT_ASSERT_EQUAL (VarAuth->AuthInfo.Hdr.dwLength, OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData));
  UT_ASSERT_EQUAL (VarAuth->AuthInfo.Hdr.wRevision, 0x0200);
  UT_ASSERT_EQUAL (VarAuth->AuthInfo.Hdr.wCertificateType, WIN_CERT_TYPE_EFI_GUID);
  UT_ASSERT_TRUE (CompareGuid (&VarAuth->AuthInfo.CertType, &gEfiCertPkcs7Guid));

  UT_ASSERT_MEM_EQUAL (VarAuth->AuthInfo.CertData, Data, sizeof (Data));

  return UNIT_TEST_PASSED;
}

/**
  Unit test for CreateTimeBasedPayload () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
CreateTimeBasedPayloadShouldCheckInput (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       DataSize = 0;
  UINT8       *Data    = NULL;
  EFI_TIME    Time;
  EFI_STATUS  Status;

  Status = CreateTimeBasedPayload (NULL, &Data, &Time);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  Status = CreateTimeBasedPayload (&DataSize, NULL, &Time);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  Status = CreateTimeBasedPayload (&DataSize, &Data, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for DeleteDb () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
DeleteDbShouldDelete (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy       = 3;
  UINT8       *Payload    = NULL;
  UINTN       PayloadSize = 0;

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (Dummy));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));
  will_return (MockGetVariable, &Dummy);

  Status = CreateTimeBasedPayload (&PayloadSize, &Payload, &mMaxTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE);

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE);
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE);

  will_return (MockSetVariable, EFI_SUCCESS);

  Status = DeleteDb ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for DeleteDbx () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
DeleteDbxShouldDelete (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy       = 3;
  UINT8       *Payload    = NULL;
  UINTN       PayloadSize = 0;

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (Dummy));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));
  will_return (MockGetVariable, &Dummy);

  Status = CreateTimeBasedPayload (&PayloadSize, &Payload, &mMaxTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE);

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE);
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE);

  will_return (MockSetVariable, EFI_SUCCESS);

  Status = DeleteDbx ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for DeleteDbt () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
DeleteDbtShouldDelete (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy       = 3;
  UINT8       *Payload    = NULL;
  UINTN       PayloadSize = 0;

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (Dummy));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));
  will_return (MockGetVariable, &Dummy);

  Status = CreateTimeBasedPayload (&PayloadSize, &Payload, &mMaxTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE);

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE);
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE);

  will_return (MockSetVariable, EFI_SUCCESS);

  Status = DeleteDbt ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for DeleteKEK () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
DeleteKEKShouldDelete (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy       = 3;
  UINT8       *Payload    = NULL;
  UINTN       PayloadSize = 0;

  expect_memory (MockGetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));

  expect_memory (MockGetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (Dummy));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));
  will_return (MockGetVariable, &Dummy);

  Status = CreateTimeBasedPayload (&PayloadSize, &Payload, &mMaxTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE);

  expect_memory (MockSetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE);
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE);

  will_return (MockSetVariable, EFI_SUCCESS);

  Status = DeleteKEK ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for DeletePlatformKey () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
DeletePKShouldDelete (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy       = 3;
  UINT8       *Payload    = NULL;
  UINTN       PayloadSize = 0;
  UINT8       BootMode    = CUSTOM_SECURE_BOOT_MODE;

  expect_memory (MockSetVariable, VariableName, EFI_CUSTOM_MODE_NAME, sizeof (EFI_CUSTOM_MODE_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiCustomModeEnableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS);
  expect_value (MockSetVariable, DataSize, sizeof (BootMode));
  expect_memory (MockSetVariable, Data, &BootMode, sizeof (BootMode));

  will_return (MockSetVariable, EFI_SUCCESS);

  expect_memory (MockGetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));

  expect_memory (MockGetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (Dummy));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));
  will_return (MockGetVariable, &Dummy);

  Status = CreateTimeBasedPayload (&PayloadSize, &Payload, &mMaxTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE);

  expect_memory (MockSetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE);
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE);

  will_return (MockSetVariable, EFI_SUCCESS);

  Status = DeletePlatformKey ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for DeleteSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
DeleteSecureBootVariablesShouldDelete (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy       = 3;
  UINT8       *Payload    = NULL;
  UINTN       PayloadSize = 0;
  UINT8       BootMode    = CUSTOM_SECURE_BOOT_MODE;

  Status = CreateTimeBasedPayload (&PayloadSize, &Payload, &mMaxTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE);

  will_return (DisablePKProtection, EFI_SUCCESS);

  expect_memory (MockSetVariable, VariableName, EFI_CUSTOM_MODE_NAME, sizeof (EFI_CUSTOM_MODE_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiCustomModeEnableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS);
  expect_value (MockSetVariable, DataSize, sizeof (BootMode));
  expect_memory (MockSetVariable, Data, &BootMode, sizeof (BootMode));

  will_return (MockSetVariable, EFI_SUCCESS);

  expect_memory (MockGetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));

  expect_memory (MockGetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (Dummy));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));
  will_return (MockGetVariable, &Dummy);

  expect_memory (MockSetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE);
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE);

  will_return (MockSetVariable, EFI_SUCCESS);

  expect_memory (MockGetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));

  expect_memory (MockGetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (Dummy));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));
  will_return (MockGetVariable, &Dummy);

  expect_memory (MockSetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE);
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE);

  will_return (MockSetVariable, EFI_SUCCESS);

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (Dummy));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));
  will_return (MockGetVariable, &Dummy);

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE);
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE);

  will_return (MockSetVariable, EFI_SUCCESS);

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (Dummy));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));
  will_return (MockGetVariable, &Dummy);

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE);
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE);

  will_return (MockSetVariable, EFI_SUCCESS);

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (Dummy));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (Dummy));
  will_return (MockGetVariable, &Dummy);

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE);
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE);

  will_return (MockSetVariable, EFI_SUCCESS);

  Status = DeleteSecureBootVariables ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for DeleteSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
DeleteSecureBootVariablesShouldCheckProtection (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  will_return (DisablePKProtection, EFI_SECURITY_VIOLATION);

  Status = DeleteSecureBootVariables ();
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ABORTED);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for DeleteSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
DeleteSecureBootVariablesShouldProceedWithNotFound (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       BootMode = CUSTOM_SECURE_BOOT_MODE;

  will_return (DisablePKProtection, EFI_SUCCESS);

  expect_memory (MockSetVariable, VariableName, EFI_CUSTOM_MODE_NAME, sizeof (EFI_CUSTOM_MODE_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiCustomModeEnableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS);
  expect_value (MockSetVariable, DataSize, sizeof (BootMode));
  expect_memory (MockSetVariable, Data, &BootMode, sizeof (BootMode));

  will_return (MockSetVariable, EFI_SUCCESS);

  expect_memory (MockGetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  expect_memory (MockGetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  expect_memory (MockGetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockGetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  Status = DeleteSecureBootVariables ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for DeleteSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
EnrollFromInputShouldComplete (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy       = 3;
  UINT8       *Payload    = NULL;
  UINTN       PayloadSize = sizeof (Dummy);

  Payload = AllocateCopyPool (sizeof (Dummy), &Dummy);
  Status  = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (Dummy));

  expect_memory (MockSetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (Dummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (Dummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  Status = EnrollFromInput (EFI_PLATFORM_KEY_NAME, &gEfiGlobalVariableGuid, sizeof (Dummy), &Dummy);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SetDefaultSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SetSecureBootVariablesShouldComplete (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  UINT8                     DbDummy     = 0xDE;
  UINT8                     DbtDummy    = 0xAD;
  UINT8                     DbxDummy    = 0xBE;
  UINT8                     KekDummy    = 0xEF;
  UINT8                     PkDummy     = 0xFE;
  UINT8                     *Payload    = NULL;
  UINTN                     PayloadSize = sizeof (DbDummy);
  SECURE_BOOT_PAYLOAD_INFO  PayloadInfo;

  PayloadInfo.DbPtr             = &DbDummy;
  PayloadInfo.DbSize            = sizeof (DbDummy);
  PayloadInfo.DbxPtr            = &DbxDummy;
  PayloadInfo.DbxSize           = sizeof (DbxDummy);
  PayloadInfo.DbtPtr            = &DbtDummy;
  PayloadInfo.DbtSize           = sizeof (DbtDummy);
  PayloadInfo.KekPtr            = &KekDummy;
  PayloadInfo.KekSize           = sizeof (KekDummy);
  PayloadInfo.PkPtr             = &PkDummy;
  PayloadInfo.PkSize            = sizeof (PkDummy);
  PayloadInfo.SecureBootKeyName = L"Food";

  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  Payload = AllocateCopyPool (sizeof (DbxDummy), &DbxDummy);
  Status  = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &DbDummy, sizeof (DbDummy));
  PayloadSize = sizeof (DbDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &DbtDummy, sizeof (DbtDummy));
  PayloadSize = sizeof (DbtDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &KekDummy, sizeof (KekDummy));
  PayloadSize = sizeof (KekDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));

  expect_memory (MockSetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &PkDummy, sizeof (PkDummy));
  PayloadSize = sizeof (PkDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (PkDummy));

  expect_memory (MockSetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (PkDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (PkDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  Status = SetSecureBootVariablesToDefault (&PayloadInfo);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SetDefaultSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SetSecureBootVariablesShouldStopWhenSecure (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  UINT8                     TargetMode = SECURE_BOOT_MODE_ENABLE;
  SECURE_BOOT_PAYLOAD_INFO  PayloadInfo;

  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (TargetMode));

  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, sizeof (TargetMode));

  will_return (MockGetVariable, TRUE);
  will_return (MockGetVariable, sizeof (TargetMode));
  will_return (MockGetVariable, &TargetMode);

  Status = SetSecureBootVariablesToDefault (&PayloadInfo);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ABORTED);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SetDefaultSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SetSecureBootVariablesShouldStopFailDBX (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  UINT8                     DbxDummy    = 0xBE;
  UINT8                     *Payload    = NULL;
  UINTN                     PayloadSize = sizeof (DbxDummy);
  SECURE_BOOT_PAYLOAD_INFO  PayloadInfo;

  PayloadInfo.DbxPtr            = &DbxDummy;
  PayloadInfo.DbxSize           = sizeof (DbxDummy);
  PayloadInfo.SecureBootKeyName = L"Fail DBX";

  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  Payload = AllocateCopyPool (sizeof (DbxDummy), &DbxDummy);
  Status  = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  will_return (MockSetVariable, EFI_WRITE_PROTECTED);

  Status = SetSecureBootVariablesToDefault (&PayloadInfo);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SetDefaultSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SetSecureBootVariablesShouldStopFailDB (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  UINT8                     DbDummy     = 0xDE;
  UINT8                     DbxDummy    = 0xBE;
  UINT8                     *Payload    = NULL;
  UINTN                     PayloadSize = sizeof (DbDummy);
  SECURE_BOOT_PAYLOAD_INFO  PayloadInfo;

  PayloadInfo.DbPtr             = &DbDummy;
  PayloadInfo.DbSize            = sizeof (DbDummy);
  PayloadInfo.DbxPtr            = &DbxDummy;
  PayloadInfo.DbxSize           = sizeof (DbxDummy);
  PayloadInfo.SecureBootKeyName = L"Fail DB";

  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  Payload = AllocateCopyPool (sizeof (DbxDummy), &DbxDummy);
  Status  = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &DbDummy, sizeof (DbDummy));
  PayloadSize = sizeof (DbDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  will_return (MockSetVariable, EFI_WRITE_PROTECTED);

  Status = SetSecureBootVariablesToDefault (&PayloadInfo);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SetDefaultSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SetSecureBootVariablesShouldStopFailDBT (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  UINT8                     DbDummy     = 0xDE;
  UINT8                     DbtDummy    = 0xAD;
  UINT8                     DbxDummy    = 0xBE;
  UINT8                     *Payload    = NULL;
  UINTN                     PayloadSize = sizeof (DbDummy);
  SECURE_BOOT_PAYLOAD_INFO  PayloadInfo;

  PayloadInfo.DbPtr             = &DbDummy;
  PayloadInfo.DbSize            = sizeof (DbDummy);
  PayloadInfo.DbxPtr            = &DbxDummy;
  PayloadInfo.DbxSize           = sizeof (DbxDummy);
  PayloadInfo.DbtPtr            = &DbtDummy;
  PayloadInfo.DbtSize           = sizeof (DbtDummy);
  PayloadInfo.SecureBootKeyName = L"Fail DBT";

  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  Payload = AllocateCopyPool (sizeof (DbxDummy), &DbxDummy);
  Status  = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &DbDummy, sizeof (DbDummy));
  PayloadSize = sizeof (DbDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &DbtDummy, sizeof (DbtDummy));
  PayloadSize = sizeof (DbtDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));

  will_return (MockSetVariable, EFI_ACCESS_DENIED);

  Status = SetSecureBootVariablesToDefault (&PayloadInfo);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SetDefaultSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SetSecureBootVariablesShouldStopFailKEK (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  UINT8                     DbDummy     = 0xDE;
  UINT8                     DbtDummy    = 0xAD;
  UINT8                     DbxDummy    = 0xBE;
  UINT8                     KekDummy    = 0xEF;
  UINT8                     PkDummy     = 0xFE;
  UINT8                     *Payload    = NULL;
  UINTN                     PayloadSize = sizeof (DbDummy);
  SECURE_BOOT_PAYLOAD_INFO  PayloadInfo;

  PayloadInfo.DbPtr             = &DbDummy;
  PayloadInfo.DbSize            = sizeof (DbDummy);
  PayloadInfo.DbxPtr            = &DbxDummy;
  PayloadInfo.DbxSize           = sizeof (DbxDummy);
  PayloadInfo.DbtPtr            = &DbtDummy;
  PayloadInfo.DbtSize           = sizeof (DbtDummy);
  PayloadInfo.KekPtr            = &KekDummy;
  PayloadInfo.KekSize           = sizeof (KekDummy);
  PayloadInfo.PkPtr             = &PkDummy;
  PayloadInfo.PkSize            = sizeof (PkDummy);
  PayloadInfo.SecureBootKeyName = L"Food";

  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  Payload = AllocateCopyPool (sizeof (DbxDummy), &DbxDummy);
  Status  = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &DbDummy, sizeof (DbDummy));
  PayloadSize = sizeof (DbDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &DbtDummy, sizeof (DbtDummy));
  PayloadSize = sizeof (DbtDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &KekDummy, sizeof (KekDummy));
  PayloadSize = sizeof (KekDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));

  expect_memory (MockSetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));

  will_return (MockSetVariable, EFI_DEVICE_ERROR);

  Status = SetSecureBootVariablesToDefault (&PayloadInfo);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_DEVICE_ERROR);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SetDefaultSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SetSecureBootVariablesShouldStopFailPK (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  UINT8                     DbDummy     = 0xDE;
  UINT8                     DbtDummy    = 0xAD;
  UINT8                     DbxDummy    = 0xBE;
  UINT8                     KekDummy    = 0xEF;
  UINT8                     PkDummy     = 0xFE;
  UINT8                     *Payload    = NULL;
  UINTN                     PayloadSize = sizeof (DbDummy);
  SECURE_BOOT_PAYLOAD_INFO  PayloadInfo;

  PayloadInfo.DbPtr             = &DbDummy;
  PayloadInfo.DbSize            = sizeof (DbDummy);
  PayloadInfo.DbxPtr            = &DbxDummy;
  PayloadInfo.DbxSize           = sizeof (DbxDummy);
  PayloadInfo.DbtPtr            = &DbtDummy;
  PayloadInfo.DbtSize           = sizeof (DbtDummy);
  PayloadInfo.KekPtr            = &KekDummy;
  PayloadInfo.KekSize           = sizeof (KekDummy);
  PayloadInfo.PkPtr             = &PkDummy;
  PayloadInfo.PkSize            = sizeof (PkDummy);
  PayloadInfo.SecureBootKeyName = L"Food";

  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  Payload = AllocateCopyPool (sizeof (DbxDummy), &DbxDummy);
  Status  = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &DbDummy, sizeof (DbDummy));
  PayloadSize = sizeof (DbDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &DbtDummy, sizeof (DbtDummy));
  PayloadSize = sizeof (DbtDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE2, sizeof (EFI_IMAGE_SECURITY_DATABASE2));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbtDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &KekDummy, sizeof (KekDummy));
  PayloadSize = sizeof (KekDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));

  expect_memory (MockSetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &PkDummy, sizeof (PkDummy));
  PayloadSize = sizeof (PkDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (PkDummy));

  expect_memory (MockSetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (PkDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (PkDummy));

  will_return (MockSetVariable, EFI_INVALID_PARAMETER);

  Status = SetSecureBootVariablesToDefault (&PayloadInfo);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SECURITY_VIOLATION);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SetDefaultSecureBootVariables () API of the SecureBootVariableLib.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SetSecureBootVariablesDBTOptional (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  UINT8                     DbDummy     = 0xDE;
  UINT8                     DbxDummy    = 0xBE;
  UINT8                     KekDummy    = 0xEF;
  UINT8                     PkDummy     = 0xFE;
  UINT8                     *Payload    = NULL;
  UINTN                     PayloadSize = sizeof (DbDummy);
  SECURE_BOOT_PAYLOAD_INFO  PayloadInfo;

  PayloadInfo.DbPtr             = &DbDummy;
  PayloadInfo.DbSize            = sizeof (DbDummy);
  PayloadInfo.DbxPtr            = &DbxDummy;
  PayloadInfo.DbxSize           = sizeof (DbxDummy);
  PayloadInfo.DbtPtr            = NULL;
  PayloadInfo.DbtSize           = 0;
  PayloadInfo.KekPtr            = &KekDummy;
  PayloadInfo.KekSize           = sizeof (KekDummy);
  PayloadInfo.PkPtr             = &PkDummy;
  PayloadInfo.PkSize            = sizeof (PkDummy);
  PayloadInfo.SecureBootKeyName = L"Food";

  expect_memory (MockGetVariable, VariableName, EFI_SECURE_BOOT_MODE_NAME, sizeof (EFI_SECURE_BOOT_MODE_NAME));
  expect_value (MockGetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockGetVariable, *DataSize, 0);

  will_return (MockGetVariable, FALSE);

  Payload = AllocateCopyPool (sizeof (DbxDummy), &DbxDummy);
  Status  = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE1, sizeof (EFI_IMAGE_SECURITY_DATABASE1));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbxDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &DbDummy, sizeof (DbDummy));
  PayloadSize = sizeof (DbDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  expect_memory (MockSetVariable, VariableName, EFI_IMAGE_SECURITY_DATABASE, sizeof (EFI_IMAGE_SECURITY_DATABASE));
  expect_value (MockSetVariable, VendorGuid, &gEfiImageSecurityDatabaseGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (DbDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &KekDummy, sizeof (KekDummy));
  PayloadSize = sizeof (KekDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));

  expect_memory (MockSetVariable, VariableName, EFI_KEY_EXCHANGE_KEY_NAME, sizeof (EFI_KEY_EXCHANGE_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (KekDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  CopyMem (Payload, &PkDummy, sizeof (PkDummy));
  PayloadSize = sizeof (PkDummy);
  Status      = CreateTimeBasedPayload (&PayloadSize, &Payload, &mDefaultPayloadTimestamp);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (PayloadSize, VAR_AUTH_DESC_SIZE + sizeof (PkDummy));

  expect_memory (MockSetVariable, VariableName, EFI_PLATFORM_KEY_NAME, sizeof (EFI_PLATFORM_KEY_NAME));
  expect_value (MockSetVariable, VendorGuid, &gEfiGlobalVariableGuid);
  expect_value (MockSetVariable, Attributes, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);
  expect_value (MockSetVariable, DataSize, VAR_AUTH_DESC_SIZE + sizeof (PkDummy));
  expect_memory (MockSetVariable, Data, Payload, VAR_AUTH_DESC_SIZE + sizeof (PkDummy));

  will_return (MockSetVariable, EFI_SUCCESS);

  Status = SetSecureBootVariablesToDefault (&PayloadInfo);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Initialze the unit test framework, suite, and unit tests for the
  SecureBootVariableLib and run the SecureBootVariableLib unit test.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      SecureBootVarMiscTests;
  UNIT_TEST_SUITE_HANDLE      SecureBootVarDeleteTests;
  UNIT_TEST_SUITE_HANDLE      SecureBootVarEnrollTests;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the SecureBootVariableLib Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&SecureBootVarMiscTests, Framework, "SecureBootVariableLib Miscellaneous Tests", "SecureBootVariableLib.Miscellaneous", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SecureBootVariableLib\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  Status = CreateUnitTestSuite (&SecureBootVarDeleteTests, Framework, "SecureBootVariableLib Deletion Tests", "SecureBootVariableLib.Deletion", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SecureBootVariableLib\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  Status = CreateUnitTestSuite (&SecureBootVarEnrollTests, Framework, "SecureBootVariableLib Enrollment Tests", "SecureBootVariableLib.Enrollment", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SecureBootVariableLib\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // --------------Suite-----------Description--------------Name----------Function--------Pre---Post-------------------Context-----------
  //
  AddTestCase (SecureBootVarMiscTests, "SetSecureBootMode should propagate to set variable", "SetSecureBootMode", SetSecureBootModeShouldSetVar, NULL, NULL, NULL);
  AddTestCase (SecureBootVarMiscTests, "GetSetupMode should propagate to get variable", "GetSetupMode", GetSetupModeShouldGetVar, NULL, NULL, NULL);
  AddTestCase (SecureBootVarMiscTests, "IsSecureBootEnabled should propagate to get variable", "IsSecureBootEnabled", IsSecureBootEnableShouldGetVar, NULL, NULL, NULL);
  AddTestCase (SecureBootVarMiscTests, "SecureBootCreateDataFromInput with one input cert", "SecureBootCreateDataFromInput One Cert", SecureBootCreateDataFromInputSimple, NULL, NULL, NULL);
  AddTestCase (SecureBootVarMiscTests, "SecureBootCreateDataFromInput with no input cert", "SecureBootCreateDataFromInput No Cert", SecureBootCreateDataFromInputNull, NULL, NULL, NULL);
  AddTestCase (SecureBootVarMiscTests, "SecureBootCreateDataFromInput with multiple input cert", "SecureBootCreateDataFromInput No Cert", SecureBootCreateDataFromInputMultiple, NULL, NULL, NULL);
  AddTestCase (SecureBootVarMiscTests, "CreateTimeBasedPayload should populate descriptor data", "CreateTimeBasedPayload Normal", CreateTimeBasedPayloadShouldPopulateDescriptor, NULL, NULL, NULL);
  AddTestCase (SecureBootVarMiscTests, "CreateTimeBasedPayload should fail on NULL inputs", "CreateTimeBasedPayload NULL", CreateTimeBasedPayloadShouldCheckInput, NULL, NULL, NULL);

  AddTestCase (SecureBootVarDeleteTests, "DeleteDb should delete DB with auth info", "DeleteDb", DeleteDbShouldDelete, NULL, NULL, NULL);
  AddTestCase (SecureBootVarDeleteTests, "DeleteDbx should delete DBX with auth info", "DeleteDbx", DeleteDbxShouldDelete, NULL, NULL, NULL);
  AddTestCase (SecureBootVarDeleteTests, "DeleteDbt should delete DBT with auth info", "DeleteDbt", DeleteDbtShouldDelete, NULL, NULL, NULL);
  AddTestCase (SecureBootVarDeleteTests, "DeleteKEK should delete KEK with auth info", "DeleteKEK", DeleteKEKShouldDelete, NULL, NULL, NULL);
  AddTestCase (SecureBootVarDeleteTests, "DeletePlatformKey should delete PK with auth info", "DeletePlatformKey", DeletePKShouldDelete, NULL, NULL, NULL);
  AddTestCase (SecureBootVarDeleteTests, "DeleteSecureBootVariables should delete properly", "DeleteSecureBootVariables Normal", DeleteSecureBootVariablesShouldDelete, NULL, NULL, NULL);
  AddTestCase (SecureBootVarDeleteTests, "DeleteSecureBootVariables should fail if protection disable fails", "DeleteSecureBootVariables Fail", DeleteSecureBootVariablesShouldCheckProtection, NULL, NULL, NULL);
  AddTestCase (SecureBootVarDeleteTests, "DeleteSecureBootVariables should continue if any variable is not found", "DeleteSecureBootVariables Proceed", DeleteSecureBootVariablesShouldProceedWithNotFound, NULL, NULL, NULL);

  AddTestCase (SecureBootVarEnrollTests, "EnrollFromInput should supply with authenticated payload", "EnrollFromInput Normal", EnrollFromInputShouldComplete, NULL, NULL, NULL);
  AddTestCase (SecureBootVarEnrollTests, "SetSecureBootVariablesToDefault should complete", "SetSecureBootVariablesToDefault Normal", SetSecureBootVariablesShouldComplete, NULL, NULL, NULL);
  AddTestCase (SecureBootVarEnrollTests, "SetSecureBootVariablesToDefault should stop when already enabled", "SetSecureBootVariablesToDefault Already Started", SetSecureBootVariablesShouldStopWhenSecure, NULL, NULL, NULL);
  AddTestCase (SecureBootVarEnrollTests, "SetSecureBootVariablesToDefault should stop when DB failed", "SetSecureBootVariablesToDefault Fails DB", SetSecureBootVariablesShouldStopFailDB, NULL, NULL, NULL);
  AddTestCase (SecureBootVarEnrollTests, "SetSecureBootVariablesToDefault should stop when DBT failed", "SetSecureBootVariablesToDefault Fails DBT", SetSecureBootVariablesShouldStopFailDBT, NULL, NULL, NULL);
  AddTestCase (SecureBootVarEnrollTests, "SetSecureBootVariablesToDefault should stop when DBX failed", "SetSecureBootVariablesToDefault Fails DBX", SetSecureBootVariablesShouldStopFailDBX, NULL, NULL, NULL);
  AddTestCase (SecureBootVarEnrollTests, "SetSecureBootVariablesToDefault should stop when KEK failed", "SetSecureBootVariablesToDefault Fails KEK", SetSecureBootVariablesShouldStopFailKEK, NULL, NULL, NULL);
  AddTestCase (SecureBootVarEnrollTests, "SetSecureBootVariablesToDefault should stop when PK failed", "SetSecureBootVariablesToDefault Fails PK", SetSecureBootVariablesShouldStopFailPK, NULL, NULL, NULL);
  AddTestCase (SecureBootVarEnrollTests, "SetSecureBootVariablesToDefault should only be optional", "SetSecureBootVariablesToDefault DBT Optional", SetSecureBootVariablesDBTOptional, NULL, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return UnitTestingEntry ();
}
