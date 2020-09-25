/** @file
UEFI Shell based application for unit testing the Variable Policy Protocol.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
***/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/VariablePolicy.h>
#include <Library/VariablePolicyHelperLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include "VariablePolicyFuncTestInternal.h"

// TODO: Need to add to the UnitTestFrameworkPkg
// #include <Library/UnitTestBootLib.h>

#define UNIT_TEST_APP_NAME        "Variable Policy Unit Test Application"
#define UNIT_TEST_APP_VERSION     "0.1"

// TODO: Need to add to the UnitTestFrameworkPkg
UNIT_TEST_FRAMEWORK_HANDLE
GetActiveFrameworkHandle (
  VOID
  );

EDKII_VARIABLE_POLICY_PROTOCOL  *mVarPol = NULL;


EFI_GUID mTestNamespaceGuid1 = { 0x3b389299, 0xabaf, 0x433b, { 0xa4, 0xa9, 0x23, 0xc8, 0x44, 0x02, 0xfc, 0xad } };
EFI_GUID mTestNamespaceGuid2 = { 0x4c49a3aa, 0xbcb0, 0x544c, { 0xb5, 0xba, 0x34, 0xd9, 0x55, 0x13, 0x0d, 0xbe } };
EFI_GUID mTestNamespaceGuid3 = { 0x5d5ab4bb, 0xcdc1, 0x655d, { 0xc6, 0xcb, 0x45, 0xea, 0x66, 0x24, 0x1e, 0xcf } };

#define  TEST_AUTH_VAR_NAME     L"DummyAuthVar"
EFI_GUID mTestAuthNamespaceGuid = { 0xb6c5a2c6, 0x3ece, 0x4b9b, { 0x8c, 0xc8, 0x96, 0xd8, 0xd9, 0xca, 0xd3, 0x4e } };

/**
  Prerequisite for most test cases.
  Makes sure that the protocol can be located prior to the test case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
LocateVarPolicyPreReq (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;

  if (mVarPol == NULL) {
    Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid,
                                  NULL,
                                  (VOID **) &mVarPol);
    UT_ASSERT_NOT_EFI_ERROR (Status);
    UT_ASSERT_NOT_NULL (mVarPol);
  }

  return UNIT_TEST_PASSED;
}

/**
  Prerequisite to make sure that VarPolicy is also enabled.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
VarPolicyEnabledPreReq (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN     State;

  UT_ASSERT_EQUAL(LocateVarPolicyPreReq(Context), UNIT_TEST_PASSED);
  Status = mVarPol->IsVariablePolicyEnabled (&State);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_TRUE(State);

  return UNIT_TEST_PASSED;
}

/**
  Prerequisite to make sure that VarPolicy is also disabled.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
VarPolicyDisabledPreReq (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN     State;

  UT_ASSERT_EQUAL(LocateVarPolicyPreReq(Context), UNIT_TEST_PASSED);
  Status = mVarPol->IsVariablePolicyEnabled (&State);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_FALSE(State);

  return UNIT_TEST_PASSED;
}

//=============================================================================
//
// GETTING STARTED TEST SUITE
//
//=============================================================================
/**
  Unit test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
CheckVpEnabled (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  BOOLEAN State;

  Status = mVarPol->IsVariablePolicyEnabled (&State);

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (State, TRUE);

  return UNIT_TEST_PASSED;
}

/**
  CheckVpRevision test.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
CheckVpRevision (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UT_ASSERT_NOT_EQUAL (mVarPol->Revision, 0);
  UT_LOG_INFO ("VP Revision: 0x%x\n", mVarPol->Revision);

  return UNIT_TEST_PASSED;
}

/**
  NOTE: Copied from SecureBootConfigImpl.c, then modified.

  Create a time based data payload by concatenating the EFI_VARIABLE_AUTHENTICATION_2
  descriptor with the input data. NO authentication is required in this function.

  @param[in, out]   DataSize       On input, the size of Data buffer in bytes.
                                   On output, the size of data returned in Data
                                   buffer in bytes.
  @param[in, out]   Data           On input, Pointer to data buffer to be wrapped or
                                   pointer to NULL to wrap an empty payload.
                                   On output, Pointer to the new payload date buffer allocated from pool,
                                   it's caller's responsibility to free the memory when finish using it.
  @param[in]        Time           [Optional] If provided, will be used as the timestamp for the payload.
                                   If NULL, a new timestamp will be generated using GetTime().

  @retval EFI_SUCCESS              Create time based payload successfully.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resources to create time based payload.
  @retval EFI_INVALID_PARAMETER    The parameter is invalid.
  @retval Others                   Unexpected error happens.

  **/
STATIC
EFI_STATUS
CreateEmptyTimeBasedPayload (
  IN OUT UINTN            *DataSize,
  IN OUT UINT8            **Data,
  IN     EFI_TIME         *Time OPTIONAL
  )
{
  UINT8                            *NewData;
  UINT8                            *Payload;
  UINTN                            PayloadSize;
  EFI_VARIABLE_AUTHENTICATION_2    *DescriptorData;
  UINTN                            DescriptorSize;
  EFI_TIME                         NewTime;

  if (Data == NULL || DataSize == NULL) {
    DEBUG((DEBUG_ERROR, "CreateEmptyTimeBasedPayload(), invalid arg\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // In Setup mode or Custom mode, the variable does not need to be signed but the
  // parameters to the SetVariable() call still need to be prepared as authenticated
  // variable. So we create EFI_VARIABLE_AUTHENTICATED_2 descriptor without certificate
  // data in it.
  //
  Payload     = *Data;
  PayloadSize = *DataSize;

  DescriptorSize    = OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo) + OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  NewData = (UINT8*) AllocateZeroPool (DescriptorSize + PayloadSize);
  if (NewData == NULL) {
    DEBUG((DEBUG_ERROR, "CreateEmptyTimeBasedPayload() Out of resources.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Payload != NULL) && (PayloadSize != 0)) {
    CopyMem (NewData + DescriptorSize, Payload, PayloadSize);
  }

  DescriptorData = (EFI_VARIABLE_AUTHENTICATION_2 *) (NewData);

  //
  // Use or create the timestamp.
  //
  // If Time is NULL, create a new timestamp.
  if (Time == NULL)
  {
    NewTime.Year       = 9999;
    NewTime.Month      = 12;
    NewTime.Day        = 31;
    NewTime.Hour       = 23;
    NewTime.Minute     = 59;
    NewTime.Second     = 59;
    NewTime.Pad1       = 0;
    NewTime.Nanosecond = 0;
    NewTime.TimeZone   = 0;
    NewTime.Daylight   = 0;
    NewTime.Pad2       = 0;
    Time = &NewTime;        // Use the new timestamp.
  }
  CopyMem (&DescriptorData->TimeStamp, Time, sizeof (EFI_TIME));

  DescriptorData->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  DescriptorData->AuthInfo.Hdr.wRevision        = 0x0200;
  DescriptorData->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&DescriptorData->AuthInfo.CertType, &gEfiCertPkcs7Guid);

  if (Payload != NULL) {
    FreePool(Payload);
  }

  *DataSize = DescriptorSize + PayloadSize;
  *Data     = NewData;
  return EFI_SUCCESS;
}

//=============================================================================
//
// NO_LOCK POLICY TEST SUITE
//
//=============================================================================
/**
  TestMinSizeNoLock test.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
TestMinSizeNoLock (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value1;
  UINT32     Value2;
  UINT8     *Buffer;

  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"MinSizeNoLockVar",
                                        4,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var that is smaller than minsize
  //
  Value1 = 0x12;
  Status = gRT->SetVariable (L"MinSizeNoLockVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value1),
                             &Value1);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  //
  // Try to write a var of size that matches minsize
  //
  Value2 = 0xa1b2c3d4;
  Status = gRT->SetVariable (L"MinSizeNoLockVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value2),
                             &Value2);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to delete the var
  //
  Status = gRT->SetVariable (L"MinSizeNoLockVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var of size larger than minsize
  //
  Buffer = AllocateZeroPool (40);
  UT_ASSERT_NOT_NULL (Buffer);
  Status = gRT->SetVariable (L"MinSizeNoLockVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             40,
                             Buffer);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Delete the variable
  //
  Status = gRT->SetVariable (L"MinSizeNoLockVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  FreePool (Buffer);

  return UNIT_TEST_PASSED;
}

/**
  TestMaxSizeNoLock test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestMaxSizeNoLock (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value1;
  UINT32     Value2;
  UINT8     *Buffer;

  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"MaxSizeNoLockVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        4,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var that is smaller than maxsize
  //
  Value1 = 0x34;
  Status = gRT->SetVariable (L"MaxSizeNoLockVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value1),
                             &Value1);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to delete the var
  //
  Status = gRT->SetVariable (L"MaxSizeNoLockVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var of size that matches maxsize
  //
  Value2 = 0xa1b2c3d4;
  Status = gRT->SetVariable (L"MaxSizeNoLockVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value2),
                             &Value2);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to delete the var
  //
  Status = gRT->SetVariable (L"MaxSizeNoLockVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var of size larger than maxsize
  //
  Buffer = AllocateZeroPool (40);
  UT_ASSERT_NOT_NULL (Buffer);
  Status = gRT->SetVariable (L"MaxSizeNoLockVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             40,
                             Buffer);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  FreePool (Buffer);

  return UNIT_TEST_PASSED;
}

/**
  TestMustHaveAttrNoLock test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestMustHaveAttrNoLock (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value;

  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"MustHaveAttrNoLockVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var that doesn't have the must-have attributes
  //
  Value = 0x56;
  Status = gRT->SetVariable (L"MustHaveAttrNoLockVar",
                             &mTestNamespaceGuid1,
                             EFI_VARIABLE_BOOTSERVICE_ACCESS,
                             sizeof (Value),
                             &Value);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  //
  // Try to write a var that has exactly the required attributes
  //
  Status = gRT->SetVariable (L"MustHaveAttrNoLockVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to delete the var
  // NOTE: some implementations of VP will require the musthave attributes to be passed even when deleting
  //
  Status = gRT->SetVariable (L"MustHaveAttrNoLockVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var that has the required attributes and one extra attribute
  //
  Status = gRT->SetVariable (L"MustHaveAttrNoLockVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to delete the var
  // NOTE: some implementations of VP will require the musthave attributes to be passed even when deleting
  //
  Status = gRT->SetVariable (L"MustHaveAttrNoLockVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  return UNIT_TEST_PASSED;
}

/**
  TestCantHaveAttrNoLock test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestCantHaveAttrNoLock (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value;

  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"CantHaveAttrNoLockVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        EFI_VARIABLE_NON_VOLATILE,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var that has a can't have attr
  //
  Value = 0x78;
  Status = gRT->SetVariable (L"CantHaveAttrNoLockVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  //
  // Try to write a var that satisfies the can't have requirement
  //
  Status = gRT->SetVariable (L"CantHaveAttrNoLockVar",
                             &mTestNamespaceGuid1,
                             EFI_VARIABLE_BOOTSERVICE_ACCESS,
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to delete the var
  //
  Status = gRT->SetVariable (L"CantHaveAttrNoLockVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  TestMaxSizeNamespaceNoLock test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestMaxSizeNamespaceNoLock (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value1;
  UINT32     Value2;
  UINT8     *Buffer;

  //
  // Register a namespace-wide policy limiting max size to 4 bytes
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid2,
                                        NULL,
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        4,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var that is smaller than maxsize
  //
  Value1 = 0x34;
  Status = gRT->SetVariable (L"MaxSizeNoLockVar",
                             &mTestNamespaceGuid2,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value1),
                             &Value1);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to delete the var
  //
  Status = gRT->SetVariable (L"MaxSizeNoLockVar",
                             &mTestNamespaceGuid2,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var of size that matches maxsize
  //
  Value2 = 0xa1b2c3d4;
  Status = gRT->SetVariable (L"MaxSizeNoLockVar",
                             &mTestNamespaceGuid2,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value2),
                             &Value2);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to delete the var
  //
  Status = gRT->SetVariable (L"MaxSizeNoLockVar",
                             &mTestNamespaceGuid2,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var of size larger than maxsize
  //
  Buffer = AllocateZeroPool (40);
  UT_ASSERT_NOT_NULL (Buffer);
  Status = gRT->SetVariable (L"MaxSizeNoLockVar",
                             &mTestNamespaceGuid2,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             40,
                             Buffer);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  FreePool (Buffer);

  return UNIT_TEST_PASSED;
}

/**
  TestMustHaveAttrWildcardNoLock test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestMustHaveAttrWildcardNoLock (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value;

  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"MustHaveAttrWildcardNoLockVar####",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var that doesn't have the must-have attributes
  //
  Value = 0x56;
  Status = gRT->SetVariable (L"MustHaveAttrWildcardNoLockVar1573",
                             &mTestNamespaceGuid1,
                             EFI_VARIABLE_BOOTSERVICE_ACCESS,
                             sizeof (Value),
                             &Value);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  //
  // Try to write a var that has exactly the required attributes
  //
  Status = gRT->SetVariable (L"MustHaveAttrWildcardNoLockVar1234",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to delete the var
  // NOTE: some implementations of VP will require the musthave attributes to be passed even when deleting
  //
  Status = gRT->SetVariable (L"MustHaveAttrWildcardNoLockVar1234",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try to write a var that has the required attributes and one extra attribute
  //
  Status = gRT->SetVariable (L"MustHaveAttrWildcardNoLockVar5612",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to delete the var
  // NOTE: some implementations of VP will require the musthave attributes to be passed even when deleting
  //
  Status = gRT->SetVariable (L"MustHaveAttrWildcardNoLockVar5612",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  TestPolicyprioritizationNoLock test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestPolicyprioritizationNoLock (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value8;
  UINT16     Value16;
  UINT32     Value32;
  UINT64     Value64;

  //
  // Register a policy targeting the specific var
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid3,
                                        L"PolicyPriorityTestVar123",
                                        8, // min size of UINT64
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Register a policy with wildcards in the name
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid3,
                                        L"PolicyPriorityTestVar###",
                                        4, // min size of UINT32
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Register a policy with wildcards in the name
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid3,
                                        NULL,
                                        2, // min size of UINT16
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // The idea is that the most specific policy is applied:
  //   For varname "TestVar", the namespace-wide one should apply: UINT16 minimum
  //   For varname "PolicyPriorityTestVar567" the wildcard policy should apply: UINT32 minimum
  //   For varname "PolicyPriorityTestVar123" the var-specific policy should apply: UINT64 minimum
  //

  //
  // Let's confirm the namespace-wide policy enforcement
  //
  Value8 = 0x78;
  Status = gRT->SetVariable (L"TestVar",
                             &mTestNamespaceGuid3,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value8),
                             &Value8);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  Value16 = 0x6543;
  Status = gRT->SetVariable (L"TestVar",
                             &mTestNamespaceGuid3,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value16),
                             &Value16);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Let's confirm the wildcard policy enforcement
  //
  Value16 = 0xabba;
  Status = gRT->SetVariable (L"PolicyPriorityTestVar567",
                             &mTestNamespaceGuid3,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value16),
                             &Value16);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  Value32 = 0xfedcba98;
  Status = gRT->SetVariable (L"PolicyPriorityTestVar567",
                             &mTestNamespaceGuid3,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value32),
                             &Value32);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Let's confirm the var-specific policy enforcement
  //
  Value32 = 0x8d3f627c;
  Status = gRT->SetVariable (L"PolicyPriorityTestVar123",
                             &mTestNamespaceGuid3,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value32),
                             &Value32);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  Value64 = 0xbebecdcdafaf6767;
  Status = gRT->SetVariable (L"PolicyPriorityTestVar123",
                             &mTestNamespaceGuid3,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value64),
                             &Value64);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

//=============================================================================
//
// LOCK_NOW POLICY TEST SUITE
//
//=============================================================================
/**
  TestExistingVarLockNow test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestExistingVarLockNow (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value;

  //
  // Write a var that we'll protect next
  //
  Value = 0x78;
  Status = gRT->SetVariable (L"ExistingLockNowVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Register a LockNow policy targeting the var
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"ExistingLockNowVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_LOCK_NOW);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Attempt to modify the locked var
  //
  Value = 0xA5;
  Status = gRT->SetVariable (L"ExistingLockNowVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  //
  // Attempt to delete the locked var
  //
  Status = gRT->SetVariable (L"ExistingLockNowVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  //
  // This variable is deleted in final cleanup.
  //

  return UNIT_TEST_PASSED;
}

/**
  TestNonexistentVarLockNow test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestNonexistentVarLockNow (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value;
  UINTN      Size;

  //
  // Make sure the variable we're about to create the policy for doesn't exist
  //
  Size = 0;
  Status = gRT->GetVariable (L"NonexistentLockNowVar",
                             &mTestNamespaceGuid1,
                             NULL,
                             &Size,
                             NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  //
  // Register a LockNow policy targeting the var
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"NonexistentLockNowVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_LOCK_NOW);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Attempt to create the locked var
  //
  Value = 0xA5;
  Status = gRT->SetVariable (L"NonexistentLockNowVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  return UNIT_TEST_PASSED;
}

//=============================================================================
//
// LOCK_ON_CREATE POLICY TEST SUITE
//
//=============================================================================
/**
  TestExistingVarLockOnCreate test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestExistingVarLockOnCreate (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value;

  //
  // Write a var that we'll protect later
  //
  Value = 0x78;
  Status = gRT->SetVariable (L"ExistingLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Register a LockNow policy targeting the var
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"ExistingLockOnCreateVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_LOCK_ON_CREATE);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Attempt to modify the locked var
  //
  Value = 0xA5;
  Status = gRT->SetVariable (L"ExistingLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  //
  // Attempt to delete the locked var
  //
  Status = gRT->SetVariable (L"ExistingLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  //
  // This variable is deleted in final cleanup.
  //

  return UNIT_TEST_PASSED;
}

/**
  TestNonexistentVarLockOnCreate test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestNonexistentVarLockOnCreate (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value1;
  UINT32     Value2;
  UINTN      Size;

  //
  // Make sure the variable we're about to create the policy for doesn't exist
  //
  Size = 0;
  Status = gRT->GetVariable (L"NonexistentLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             NULL,
                             &Size,
                             NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  //
  // Register a LockOnCreate policy targeting the var
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"NonexistentLockOnCreateVar",
                                        2, // min size of 2 bytes, UINT16+
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        EFI_VARIABLE_RUNTIME_ACCESS, // must have RT attr
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_LOCK_ON_CREATE);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Attempt to create the var, but smaller than min size
  //
  Value1 = 0xA5;
  Status = gRT->SetVariable (L"NonexistentLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value1),
                             &Value1);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  //
  // Now let's make sure attribute req is enforced
  //
  Value2 = 0x43218765;
  Status = gRT->SetVariable (L"NonexistentLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value2),
                             &Value2);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  //
  // Now let's create a valid variable
  //
  Value2 = 0x43218765;
  Status = gRT->SetVariable (L"NonexistentLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value2),
                             &Value2);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Let's make sure we can't modify it
  //
  Value2 = 0xa5a5b6b6;
  Status = gRT->SetVariable (L"NonexistentLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value2),
                             &Value2);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  //
  // Finally, let's make sure we can't delete it
  //
  Status = gRT->SetVariable (L"NonexistentLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  //
  // This variable is deleted in final cleanup.
  //

  return UNIT_TEST_PASSED;
}

//=============================================================================
//
// LOCK_ON_VAR_STATE POLICY TEST SUITE
//
//=============================================================================
/**
  TestLockOnVarStateBeforeCreate test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestLockOnVarStateBeforeCreate (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN      Size;
  UINT8      Value;

  //
  // First of all, let's make sure the var we're trying to protect doesn't exist
  //
  Size = 0;
  Status = gRT->GetVariable (L"NonexistentLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             NULL,
                             &Size,
                             NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  //
  // Good, now let's create a policy
  //
  Status = RegisterVarStateVariablePolicy (mVarPol,
                                           &mTestNamespaceGuid1,
                                           L"NonexistentLockOnVarStateVar",
                                           VARIABLE_POLICY_NO_MIN_SIZE,
                                           VARIABLE_POLICY_NO_MAX_SIZE,
                                           VARIABLE_POLICY_NO_MUST_ATTR,
                                           VARIABLE_POLICY_NO_CANT_ATTR,
                                           &mTestNamespaceGuid1,
                                           L"Trigger1",
                                           0x7E);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Now we write the trigger var
  //
  Value = 0x7E;
  Status = gRT->SetVariable (L"Trigger1",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Ok, now we attempt to write a var protected by the trigger
  //
  Value = 0xFA;
  Status = gRT->SetVariable (L"NonexistentLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  //
  // Let's modify the trigger var and "untrigger" the policy
  //
  Value = 0x38;
  Status = gRT->SetVariable (L"Trigger1",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Now we should be able to create the var targeted by the policy
  //
  Value = 0x23;
  Status = gRT->SetVariable (L"NonexistentLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Cleanup: delete the trigger and the protected var
  //
  Status = gRT->SetVariable (L"Trigger1",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = gRT->SetVariable (L"NonexistentLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  TestLockOnVarStateAfterCreate test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestLockOnVarStateAfterCreate (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value;

  //
  // Let's create a policy
  //
  Status = RegisterVarStateVariablePolicy (mVarPol,
                                           &mTestNamespaceGuid1,
                                           L"ExistingLockOnVarStateVar",
                                           VARIABLE_POLICY_NO_MIN_SIZE,
                                           VARIABLE_POLICY_NO_MAX_SIZE,
                                           VARIABLE_POLICY_NO_MUST_ATTR,
                                           VARIABLE_POLICY_NO_CANT_ATTR,
                                           &mTestNamespaceGuid1,
                                           L"Trigger2",
                                           0x5C);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should be able to write targeted var since the policy isn't active yet.
  //
  Value = 0x17;
  Status = gRT->SetVariable (L"ExistingLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Let's modify the var to make sure the policy isn't acting like a lock-on-create one
  //
  Value = 0x30;
  Status = gRT->SetVariable (L"ExistingLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Now we trigger the policy
  //
  Value = 0x5C;
  Status = gRT->SetVariable (L"Trigger2",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Let's now verify the variable is protected
  //
  Value = 0xB9;
  Status = gRT->SetVariable (L"ExistingLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  //
  // Ok, to clean up, we need to remove the trigger var, so delete it, and then delete the target var
  //
  Status = gRT->SetVariable (L"Trigger2",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = gRT->SetVariable (L"ExistingLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  TestLockOnVarStateInvalidLargeTrigger test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestLockOnVarStateInvalidLargeTrigger (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT16     Value;

  //
  // First let's create a variable policy
  //
  Status = RegisterVarStateVariablePolicy (mVarPol,
                                           &mTestNamespaceGuid1,
                                           L"InvalidLargeTriggerLockOnVarStateVar",
                                           VARIABLE_POLICY_NO_MIN_SIZE,
                                           VARIABLE_POLICY_NO_MAX_SIZE,
                                           VARIABLE_POLICY_NO_MUST_ATTR,
                                           VARIABLE_POLICY_NO_CANT_ATTR,
                                           &mTestNamespaceGuid1,
                                           L"Trigger3",
                                           0x5C);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Now attempt to trigger the lock but with a variable larger than one byte
  //
  Value = 0x8085;
  Status = gRT->SetVariable (L"Trigger3",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should still be able to create the targeted var
  //
  Value = 0x1234;
  Status = gRT->SetVariable (L"InvalidLargeTriggerLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Let's clean up by deleting the invalid trigger and the targeted var
  //
  Status = gRT->SetVariable (L"Trigger3",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = gRT->SetVariable (L"InvalidLargeTriggerLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  TestLockOnVarStateWrongValueTrigger test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestLockOnVarStateWrongValueTrigger (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8      Value;

  //
  // First let's create a variable policy
  //
  Status = RegisterVarStateVariablePolicy (mVarPol,
                                           &mTestNamespaceGuid1,
                                           L"WrongValueTriggerLockOnVarStateVar",
                                           VARIABLE_POLICY_NO_MIN_SIZE,
                                           VARIABLE_POLICY_NO_MAX_SIZE,
                                           VARIABLE_POLICY_NO_MUST_ATTR,
                                           VARIABLE_POLICY_NO_CANT_ATTR,
                                           &mTestNamespaceGuid1,
                                           L"Trigger4",
                                           0xCA);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Now attempt to trigger the lock but with a wrong value
  //
  Value = 0x80;
  Status = gRT->SetVariable (L"Trigger4",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Should still be able to create the targeted var
  //
  Value = 0x14;
  Status = gRT->SetVariable (L"WrongValueTriggerLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Let's clean up by deleting the invalid trigger and the targeted var
  //
  Status = gRT->SetVariable (L"Trigger4",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = gRT->SetVariable (L"WrongValueTriggerLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

//=============================================================================
//
// INVALID POLICY TEST SUITE
//
//=============================================================================
/**
  TestInvalidAttributesPolicy test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestInvalidAttributesPolicy (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;

  //
  // The only must/can't have attributes supported by VPE are NV, BS, and RT. They are 1, 2, and 4, respectively.
  // Let's try some bits higher than that?
  //

  //
  // Trying must have attribute 0x8 which is EFI_VARIABLE_HARDWARE_ERROR_RECORD
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"InvalidMustHaveAttributesPolicyVar1",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        EFI_VARIABLE_HARDWARE_ERROR_RECORD,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_LOG_INFO ("Setting must have attr to EFI_VARIABLE_HARDWARE_ERROR_RECORD returned %r\n", Status);

  //
  // Let's try 0x10 - EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS, a deprecated attribute
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"InvalidMustHaveAttributesPolicyVar2",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_LOG_INFO ("Setting must have attr to EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS returned %r\n", Status);

  //
  // Let's try 0x20 - EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"InvalidMustHaveAttributesPolicyVar3",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_LOG_INFO ("Setting must have attr to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS returned %r\n", Status);

  //
  // Let's try something wild, like 0x4000
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"InvalidMustHaveAttributesPolicyVar4",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        0x4000,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_LOG_INFO ("Setting must have attr to 0x4000 returned %r\n", Status);

  //
  // Now repeat the same tests, but for the can't-have param
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"InvalidCantHaveAttributesPolicyVar1",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        EFI_VARIABLE_HARDWARE_ERROR_RECORD,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_LOG_INFO ("Setting cant have attr to EFI_VARIABLE_HARDWARE_ERROR_RECORD returned %r\n", Status);

  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"InvalidCantHaveAttributesPolicyVar2",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_LOG_INFO ("Setting cant have attr to EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS returned %r\n", Status);

  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"InvalidCantHaveAttributesPolicyVar3",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_LOG_INFO ("Setting cant have attr to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS returned %r\n", Status);

  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"InvalidCantHaveAttributesPolicyVar4",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        0x4000,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  UT_LOG_INFO ("Setting cant have attr to 0x4000 returned %r\n", Status);

  return UNIT_TEST_PASSED;
}

/**
  TestLargeMinSizePolicy test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestLargeMinSizePolicy (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;

  //
  // Let's set the min size to 2GB and see what happens
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"LargeMinSizeInvalidPolicyVar",
                                        0x80000000,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);

  UT_LOG_INFO ("Setting min size to 0x80000000 returned %r\n", Status);

  return UNIT_TEST_PASSED;
}

/**
  TestZeroMaxSizePolicy test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestZeroMaxSizePolicy (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;

  //
  // Let's set the max size to 0 and see what happens
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"ZeroMinSizeInvalidPolicyVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        0,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_NO_LOCK);
  //UT_ASSERT_NOT_EQUAL (Status, EFI_SUCCESS); // this fails on QC. Real bug? Do we care?
  UT_LOG_INFO ("Setting max size to 0 returned %r\n", Status);

  return UNIT_TEST_PASSED;
}

/**
  TestInvalidPolicyTypePolicy test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestInvalidPolicyTypePolicy (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;

  //
  // Let's set policy type to an invalid value and see what happens
  // Valid ones are:
  //        VARIABLE_POLICY_TYPE_NO_LOCK            0
  //        VARIABLE_POLICY_TYPE_LOCK_NOW           1
  //        VARIABLE_POLICY_TYPE_LOCK_ON_CREATE     2
  //        VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE  3
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"InvalidPolicyTypePolicyVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        4);
  UT_ASSERT_NOT_EQUAL (Status, EFI_SUCCESS);

  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"InvalidPolicyTypePolicyVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        147);
  UT_ASSERT_NOT_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}

//=============================================================================
//
// DUMP POLICY TEST SUITE
//
//=============================================================================
/**
  TestDumpPolicy test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestDumpPolicy (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8*     Buffer;
  UINT32     Size;

  //
  // First let's call DumpVariablePolicy with null buffer to get size
  //
  Size = 0;
  Status = mVarPol->DumpVariablePolicy (NULL, &Size);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_BUFFER_TOO_SMALL);

  //
  // Now we allocate the buffer for the dump
  //
  Buffer = NULL;
  Buffer = AllocatePool (Size);
  UT_ASSERT_NOT_NULL (Buffer);

  //
  // Now we get the dump. In this test we will not analyze the dump.
  //
  Status = mVarPol->DumpVariablePolicy (Buffer, &Size);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

//=============================================================================
//
// POLICY VERSION TEST SUITE
//
//=============================================================================
/**
  TestPolicyVersion test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestPolicyVersion (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS             Status;
  VARIABLE_POLICY_ENTRY  *NewEntry;

  //
  // Create the new entry using a helper lib
  //
  NewEntry = NULL;
  Status = CreateBasicVariablePolicy (&mTestNamespaceGuid1,
                                      L"PolicyVersionTestNoLockVar",
                                      VARIABLE_POLICY_NO_MIN_SIZE,
                                      4, // max size of 4 bytes
                                      VARIABLE_POLICY_NO_MUST_ATTR,
                                      VARIABLE_POLICY_NO_CANT_ATTR,
                                      VARIABLE_POLICY_TYPE_NO_LOCK,
                                      &NewEntry);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  NewEntry->Version = 0x1234;
  Status = mVarPol->RegisterVariablePolicy (NewEntry);
  UT_LOG_INFO ("Registering policy entry with an unknown version status: %r\n", Status);

  FreePool (NewEntry);

  return UNIT_TEST_PASSED;
}

//=============================================================================
//
// LOCK VAR_POLICY TEST SUITE
//
//=============================================================================
/**
  LockPolicyEngineTests test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
LockPolicyEngineTests (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  UINT16      Value;
  UINT64      Value64;
  BOOLEAN     State;

  //
  // First let's register a policy that we'll test after VPE lock
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"BeforeVpeLockNoLockPolicyVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        4, // max size of 4 bytes
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_LOCK_ON_CREATE);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Now, lock VPE!
  //
  Status = mVarPol->LockVariablePolicy ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // See if we can lock it again?
  //
  Status = mVarPol->LockVariablePolicy ();
  UT_LOG_INFO ("Locking VPE for second time returned %r\n", Status);

  //
  // Let's confirm one of the policies from prior test suites is still enforced
  // Attempt to delete a locked var
  //
  Status = gRT->SetVariable (L"ExistingLockNowVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  //
  // We'll make sure the policy from earlier in this test case is actively filtering out by size
  //
  Value64 = 0x3829fed212345678;
  Status = gRT->SetVariable (L"BeforeVpeLockNoLockPolicyVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value64),
                             &Value64);
  UT_ASSERT_TRUE ((Status == EFI_WRITE_PROTECTED) || (Status == EFI_INVALID_PARAMETER));

  //
  // Let's create the variable from the policy now
  //
  Value = 0x323f;
  Status = gRT->SetVariable (L"BeforeVpeLockNoLockPolicyVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Now confirm that the var is locked after creation
  //
  Value = 0x1212;
  Status = gRT->SetVariable (L"BeforeVpeLockNoLockPolicyVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_WRITE_PROTECTED);

  //
  // Let's attempt to register a new policy, it should fail
  //
  Status = RegisterBasicVariablePolicy (mVarPol,
                                        &mTestNamespaceGuid1,
                                        L"AfterVpeLockNowPolicyVar",
                                        VARIABLE_POLICY_NO_MIN_SIZE,
                                        VARIABLE_POLICY_NO_MAX_SIZE,
                                        VARIABLE_POLICY_NO_MUST_ATTR,
                                        VARIABLE_POLICY_NO_CANT_ATTR,
                                        VARIABLE_POLICY_TYPE_LOCK_NOW);
  UT_ASSERT_NOT_EQUAL (Status, EFI_SUCCESS);

  //
  // Make sure VPE is enabled
  //
  Status = mVarPol->IsVariablePolicyEnabled (&State);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (State, TRUE);

  //
  // Finally, make sure we can't disable VPE
  //
  Status = mVarPol->DisableVariablePolicy ();
  UT_ASSERT_NOT_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}

/**
  Save context and reboot after the lock policy test suite.
  **/
STATIC
VOID
EFIAPI
SaveStateAndReboot (
  VOID
  )
{
  EFI_STATUS Status;

  //
  // Now, save all the data associated with this framework.
  // TODO: Need to add to the UnitTestFrameworkPkg
  Status = SaveFrameworkState( NULL, 0 );

  //
  // If we're all good, let's book...
  if (!EFI_ERROR( Status ))
  {
    //
    // Next, we want to update the BootNext variable to USB
    // so that we have a fighting chance of coming back here.
    //
    // TODO: Need to add to the UnitTestFrameworkPkg
    // SetBootNextDevice();

    //
    // Reset
    DEBUG(( DEBUG_INFO, "%a - Rebooting! Launch this test again once booted.\n", __FUNCTION__ ));
    gRT->ResetSystem( EfiResetCold, EFI_SUCCESS, 0, NULL );
    DEBUG(( DEBUG_ERROR, "%a - Unit test failed to quit! Framework can no longer be used!\n", __FUNCTION__ ));

    //
    // We REALLY shouldn't be here.
    Status = EFI_ABORTED;
  }

  return;
}

/**
  Reboot System and Trash the Context.

  @param            Context
  **/
STATIC
VOID
EFIAPI
IgnoreContextAndReboot (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  // Just a wrapper for prototype reuse.
  SaveStateAndReboot();
}

/**
  Disable policy tests.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
DisablePolicyEngineTests (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN     State;
  UINT8       Value;

  //
  // First, we disable the variable policy
  //
  Status = mVarPol->DisableVariablePolicy ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Confirm it's disabled
  //
  Status = mVarPol->IsVariablePolicyEnabled (&State);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (State, FALSE);

  //
  // Try locking it?
  //
  Status = mVarPol->LockVariablePolicy ();
  UT_LOG_INFO ("Locking VP after disabling it status: %r\n", Status);

  //
  // Try modifying the var from TestExistingVarLockNow
  //
  Value = 0xB5;
  Status = gRT->SetVariable (L"ExistingLockNowVar",
                             &mTestNamespaceGuid1,
                             (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                             sizeof (Value),
                             &Value);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Pre-Disable Setup and Test for Authenticated Variables test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthVarPart1 (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINT32        Data;
  UINTN         DataSize;
  UINT8         *DeleteData;

  // First, we need to create our dummy Authenticated Variable.
  Status = gRT->SetVariable(TEST_AUTH_VAR_NAME,
                            &mTestAuthNamespaceGuid,
                            (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                              EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
                            mTestAuthVarPayloadSize,
                            &mTestAuthVarPayload[0]);
  UT_ASSERT_NOT_EFI_ERROR(Status);

  // Prove that we created it.
  DataSize = sizeof(Data);
  Status = gRT->GetVariable(TEST_AUTH_VAR_NAME,
                            &mTestAuthNamespaceGuid,
                            NULL,
                            &DataSize,
                            &Data);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(Data, 0xDEADBEEF);

  // Prove that we cannot delete it.
  DeleteData = NULL;
  DataSize = 0;
  Status = CreateEmptyTimeBasedPayload(&DataSize, &DeleteData, NULL);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  Status = gRT->SetVariable(TEST_AUTH_VAR_NAME,
                            &mTestAuthNamespaceGuid,
                            (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                              EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
                            DataSize,
                            (VOID*)DeleteData);
  UT_ASSERT_STATUS_EQUAL(Status, EFI_SECURITY_VIOLATION);
  FreePool(DeleteData);

  return UNIT_TEST_PASSED;
}

/**
  Post-Disable Test for Authenticated Variables test.

  @param        Context
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthVarPart2 (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINT32        Data;
  UINTN         DataSize;
  UINT8         *DeleteData;

  // Prove that it exists.
  DataSize = sizeof(Data);
  Status = gRT->GetVariable(TEST_AUTH_VAR_NAME,
                            &mTestAuthNamespaceGuid,
                            NULL,
                            &DataSize,
                            &Data);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(Data, 0xDEADBEEF);

  // Prove that we can delete it.
  DeleteData = NULL;
  DataSize = 0;
  Status = CreateEmptyTimeBasedPayload(&DataSize, &DeleteData, NULL);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  Status = gRT->SetVariable(TEST_AUTH_VAR_NAME,
                            &mTestAuthNamespaceGuid,
                            (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                              EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
                            DataSize,
                            (VOID*)DeleteData);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  FreePool(DeleteData);

  // Prove that we deleted it.
  DataSize = sizeof(Data);
  Status = gRT->GetVariable(TEST_AUTH_VAR_NAME,
                            &mTestAuthNamespaceGuid,
                            NULL,
                            &DataSize,
                            &Data);
  UT_ASSERT_STATUS_EQUAL(Status, EFI_NOT_FOUND);

  return UNIT_TEST_PASSED;
}

/**
  Final Cleanup: delete some variables earlier test cases created.

  @param            Context
  **/
STATIC
VOID
EFIAPI
FinalCleanup (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;

  Status = gRT->SetVariable (L"ExistingLockNowVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_LOG_INFO ("Delete ExistingLockNowVar status: %r\n", Status);

  Status = gRT->SetVariable (L"ExistingLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_LOG_INFO ("Delete ExistingLockOnCreateVar status: %r\n", Status);

  Status = gRT->SetVariable (L"NonexistentLockOnCreateVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_LOG_INFO ("Delete NonexistentLockOnCreateVar status: %r\n", Status);

  Status = gRT->SetVariable (L"NonexistentLockNowVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_LOG_INFO ("Delete NonexistentLockNowVar status: %r\n", Status);

  Status = gRT->SetVariable (L"CantHaveAttrNoLockVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_LOG_INFO ("Delete CantHaveAttrNoLockVar status: %r\n", Status);

  Status = gRT->SetVariable (L"NonexistentLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_LOG_INFO ("Delete NonexistentLockOnVarStateVar status: %r\n", Status);

  Status = gRT->SetVariable (L"ExistingLockOnVarStateVar",
                             &mTestNamespaceGuid1,
                             0,
                             0,
                             NULL);
  UT_LOG_INFO ("Delete ExistingLockOnVarStateVar status: %r\n", Status);
}


/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable)
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      GettingStartedTestSuite;
  UNIT_TEST_SUITE_HANDLE      NoLockPoliciesTestSuite;
  UNIT_TEST_SUITE_HANDLE      LockNowPoliciesTestSuite;
  UNIT_TEST_SUITE_HANDLE      LockOnCreatePoliciesTestSuite;
  UNIT_TEST_SUITE_HANDLE      LockOnVarStatePoliciesTestSuite;
  UNIT_TEST_SUITE_HANDLE      InvalidPoliciesTestSuite;
  UNIT_TEST_SUITE_HANDLE      DumpPolicyTestSuite;
  UNIT_TEST_SUITE_HANDLE      PolicyVersionTestSuite;
  UNIT_TEST_SUITE_HANDLE      LockPolicyTestSuite;
  UNIT_TEST_SUITE_HANDLE      DisablePolicyTestSuite;

  Framework = NULL;
  GettingStartedTestSuite = NULL;
  NoLockPoliciesTestSuite = NULL;
  LockNowPoliciesTestSuite = NULL;
  LockOnCreatePoliciesTestSuite = NULL;
  LockOnVarStatePoliciesTestSuite = NULL;
  InvalidPoliciesTestSuite = NULL;
  DumpPolicyTestSuite = NULL;
  PolicyVersionTestSuite = NULL;
  LockPolicyTestSuite = NULL;
  DisablePolicyTestSuite = NULL;

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
  // Test suite 1: Getting Started. Get VP protocol, check state, log revision
  //
  Status = CreateUnitTestSuite (&GettingStartedTestSuite, Framework, "Getting Started", "Common.VP.GettingStarted", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the Getting Started Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (GettingStartedTestSuite, "Confirm VP is enabled", "Common.VP.GettingStarted.CheckVpEnabled", CheckVpEnabled, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (GettingStartedTestSuite, "Check VP revision", "Common.VP.GettingStarted.CheckVpRevision", CheckVpRevision, LocateVarPolicyPreReq, NULL, NULL);

  //
  // Test suite 2: Test NoLock Policies
  //
  Status = CreateUnitTestSuite (&NoLockPoliciesTestSuite, Framework, "Exercise NoLock Policies", "Common.VP.NoLockPolicies", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the NoLock Policies Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (NoLockPoliciesTestSuite, "Test Min Size enforcement in NoLock policy", "Common.VP.NoLockPolicies.TestMinSizeNoLock", TestMinSizeNoLock, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (NoLockPoliciesTestSuite, "Test Max Size enforcement in NoLock policy", "Common.VP.NoLockPolicies.TestMaxSizeNoLock", TestMaxSizeNoLock, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (NoLockPoliciesTestSuite, "Test Must Have Attribute enforcement in NoLock policy", "Common.VP.NoLockPolicies.TestMustHaveAttrNoLock", TestMustHaveAttrNoLock, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (NoLockPoliciesTestSuite, "Test Can't Have Attribute enforcement in NoLock policy", "Common.VP.NoLockPolicies.TestCantHaveAttrNoLock", TestCantHaveAttrNoLock, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (NoLockPoliciesTestSuite, "Test Max Size enforcement in NoLock policy for entire namespace", "Common.VP.NoLockPolicies.TestMaxSizeNamespaceNoLock", TestMaxSizeNamespaceNoLock, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (NoLockPoliciesTestSuite, "Test Must Have Attribute enforcement in NoLock policy with wildcards", "Common.VP.NoLockPolicies.TestMustHaveAttrWildcardNoLock", TestMustHaveAttrWildcardNoLock, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (NoLockPoliciesTestSuite, "Test policy prioritization between namespace-wide, wildcard, and var-specific policies", "Common.VP.NoLockPolicies.TestPolicyprioritizationNoLock", TestPolicyprioritizationNoLock, LocateVarPolicyPreReq, NULL, NULL);

  //
  // Test suite 3: Test LockNow policies
  //
  Status = CreateUnitTestSuite (&LockNowPoliciesTestSuite, Framework, "Exercise LockNow Policies", "Common.VP.LockNowPolicies", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the LockNow Policies Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (LockNowPoliciesTestSuite, "Test LockNow policy for a pre-existing variable", "Common.VP.LockNowPolicies.TestExistingVarLockNow", TestExistingVarLockNow, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (LockNowPoliciesTestSuite, "Test LockNow policy for a nonexistent variable", "Common.VP.LockNowPolicies.TestNonexistentVarLockNow", TestNonexistentVarLockNow, LocateVarPolicyPreReq, NULL, NULL);

  //
  // Test suite 4: Test LockOnCreate policies
  //
  Status = CreateUnitTestSuite (&LockOnCreatePoliciesTestSuite, Framework, "Exercise LockOnCreate Policies", "Common.VP.LockOnCreate", NULL, NULL);
  if (EFI_ERROR (Status))
  {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the LockOnCreate Policies Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (LockOnCreatePoliciesTestSuite, "Test LockOnCreate policy for a pre-existing variable", "Common.VP.LockOnCreate.TestExistingVarLockOnCreate", TestExistingVarLockOnCreate, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (LockOnCreatePoliciesTestSuite, "Test LockOnCreate policy for a nonexistent variable", "Common.VP.LockOnCreate.TestNonexistentVarLockOnCreate", TestNonexistentVarLockOnCreate, LocateVarPolicyPreReq, NULL, NULL);

  //
  // Test suite 5: Test LockOnVarState policies
  //
  Status = CreateUnitTestSuite (&LockOnVarStatePoliciesTestSuite, Framework, "Exercise LockOnVarState Policies", "Common.VP.LockOnVarState", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the LockOnVarState Policies Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (LockOnVarStatePoliciesTestSuite, "Test LockOnVarState policy for a nonexistent variable", "Common.VP.LockOnVarState.TestLockOnVarStateBeforeCreate", TestLockOnVarStateBeforeCreate, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (LockOnVarStatePoliciesTestSuite, "Test LockOnVarState policy for a pre-existing variable", "Common.VP.LockOnVarState.TestLockOnVarStateAfterCreate", TestLockOnVarStateAfterCreate, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (LockOnVarStatePoliciesTestSuite, "Test LockOnVarState policy triggered by invalid-size variable", "Common.VP.LockOnVarState.TestLockOnVarStateInvalidLargeTrigger", TestLockOnVarStateInvalidLargeTrigger, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (LockOnVarStatePoliciesTestSuite, "Test LockOnVarState policy triggered by invalid-value variable", "Common.VP.LockOnVarState.TestLockOnVarStateWrongValueTrigger", TestLockOnVarStateWrongValueTrigger, LocateVarPolicyPreReq, NULL, NULL);

  //
  // Test suite 6: Test registering invalid policies
  //
  Status = CreateUnitTestSuite (&InvalidPoliciesTestSuite, Framework, "Attempt registering invalid policies", "Common.VP.InvalidPolicies", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the Invalid Policies Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (InvalidPoliciesTestSuite, "Test policy with invalid must-have attributes", "Common.VP.InvalidPolicies.TestInvalidAttributesPolicy", TestInvalidAttributesPolicy, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (InvalidPoliciesTestSuite, "Test policy with invalid attributes", "Common.VP.InvalidPolicies.TestLargeMinSizePolicy", TestLargeMinSizePolicy, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (InvalidPoliciesTestSuite, "Test policy with invalid attributes", "Common.VP.InvalidPolicies.TestZeroMaxSizePolicy", TestZeroMaxSizePolicy, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (InvalidPoliciesTestSuite, "Test policy with invalid type", "Common.VP.InvalidPolicies.TestInvalidPolicyTypePolicy", TestInvalidPolicyTypePolicy, LocateVarPolicyPreReq, NULL, NULL);

  //
  // Test suite 7: Test dumping the policy
  //
  Status = CreateUnitTestSuite (&DumpPolicyTestSuite, Framework, "Attempt dumping policy", "Common.VP.DumpPolicy", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the Dump Policy Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (DumpPolicyTestSuite, "Test dumping policy", "Common.VP.DumpPolicy.TestDumpPolicy", TestDumpPolicy, LocateVarPolicyPreReq, NULL, NULL);

  //
  // Test suite 8: Test policy version
  //
  Status = CreateUnitTestSuite (&PolicyVersionTestSuite, Framework, "Use non-zero policy version", "Common.VP.PolicyVersion", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the Policy Version Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (PolicyVersionTestSuite, "Test policy version", "Common.VP.DumpPolicy.TestPolicyVersion", TestPolicyVersion, LocateVarPolicyPreReq, NULL, NULL);

  //
  // Test suite 9: Lock VPE and test implications
  //
  Status = CreateUnitTestSuite (&LockPolicyTestSuite, Framework, "Lock policy, test it", "Common.VP.LockPolicyTests", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the Lock Policy Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (LockPolicyTestSuite, "Test locking policy", "Common.VP.LockPolicyTests.LockPolicyEngineTests", LockPolicyEngineTests, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (LockPolicyTestSuite, "Test locking policy", "Common.VP.LockPolicyTests.LockPolicyEngineTests", LockPolicyEngineTests, LocateVarPolicyPreReq, IgnoreContextAndReboot, NULL);

  //
  // Test suite 10: Disable var policy and confirm expected behavior
  //
  Status = CreateUnitTestSuite (&DisablePolicyTestSuite, Framework, "Disable policy, test it", "Common.VP.DisablePolicyTests", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the Disable Policy Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (DisablePolicyTestSuite, "Confirm VP is enabled", "Common.VP.DisablePolicyTests.CheckVpEnabled", CheckVpEnabled, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (DisablePolicyTestSuite, "Test LockNow policy for a pre-existing variable", "Common.VP.DisablePolicyTests.TestExistingVarLockNow", TestExistingVarLockNow, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (DisablePolicyTestSuite, "Test AuthVar protection while VariablePolicy is enabled", "Common.VP.DisablePolicyTests.TestAuthVar1", TestAuthVarPart1, VarPolicyEnabledPreReq, NULL, NULL);
  AddTestCase (DisablePolicyTestSuite, "Test disabling policy", "Common.VP.DisablePolicyTests.DisablePolicyEngineTests", DisablePolicyEngineTests, LocateVarPolicyPreReq, NULL, NULL);
  AddTestCase (DisablePolicyTestSuite, "Test AuthVar protection while VariablePolicy is disabled", "Common.VP.DisablePolicyTests.TestAuthVar2", TestAuthVarPart2, VarPolicyDisabledPreReq, FinalCleanup, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}
