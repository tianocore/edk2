/** @file -- VariablePolicyUnitTest.c
UnitTest for...
Business logic for Variable Policy enforcement.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>

#include <Guid/VariableFormat.h>

#include <Protocol/VariablePolicy.h>
#include <Library/VariablePolicyLib.h>

#ifndef INTERNAL_UNIT_TEST
  #error Make sure to build thie with INTERNAL_UNIT_TEST enabled! Otherwise, some important tests may be skipped!
#endif

#define UNIT_TEST_NAME     "UEFI Variable Policy UnitTest"
#define UNIT_TEST_VERSION  "0.5"

/// === TEST DATA ==================================================================================

#pragma pack(push, 1)
#define SIMPLE_VARIABLE_POLICY_ENTRY_VAR_NAME_LENGTH  1001    // 1000 characters + terminator.
#define SIMPLE_VARIABLE_POLICY_ENTRY_VAR_NAME_SIZE    (SIMPLE_VARIABLE_POLICY_ENTRY_VAR_NAME_LENGTH * sizeof(CHAR16))
typedef struct _SIMPLE_VARIABLE_POLICY_ENTRY {
  VARIABLE_POLICY_ENTRY    Header;
  CHAR16                   Name[SIMPLE_VARIABLE_POLICY_ENTRY_VAR_NAME_LENGTH];
} SIMPLE_VARIABLE_POLICY_ENTRY;
#define EXPANDED_VARIABLE_POLICY_ENTRY_VAR_NAME_LENGTH  1001    // 1000 characters + terminator.
#define EXPANDED_VARIABLE_POLICY_ENTRY_VAR_NAME_SIZE    (EXPANDED_VARIABLE_POLICY_ENTRY_VAR_NAME_LENGTH * sizeof(CHAR16))
typedef struct _EXPANDED_VARIABLE_POLICY_ENTRY {
  VARIABLE_POLICY_ENTRY                Header;
  VARIABLE_LOCK_ON_VAR_STATE_POLICY    StatePolicy;
  CHAR16                               StateName[EXPANDED_VARIABLE_POLICY_ENTRY_VAR_NAME_LENGTH];
  CHAR16                               Name[EXPANDED_VARIABLE_POLICY_ENTRY_VAR_NAME_LENGTH];
} EXPANDED_VARIABLE_POLICY_ENTRY;
#pragma pack(pop)

// {F955BA2D-4A2C-480C-BFD1-3CC522610592}
#define TEST_GUID_1  { 0xf955ba2d, 0x4a2c, 0x480c, { 0xbf, 0xd1, 0x3c, 0xc5, 0x22, 0x61, 0x5, 0x92 } }
EFI_GUID  mTestGuid1 = TEST_GUID_1;
// {2DEA799E-5E73-43B9-870E-C945CE82AF3A}
#define TEST_GUID_2  { 0x2dea799e, 0x5e73, 0x43b9, { 0x87, 0xe, 0xc9, 0x45, 0xce, 0x82, 0xaf, 0x3a } }
EFI_GUID  mTestGuid2 = TEST_GUID_2;
// {698A2BFD-A616-482D-B88C-7100BD6682A9}
#define TEST_GUID_3  { 0x698a2bfd, 0xa616, 0x482d, { 0xb8, 0x8c, 0x71, 0x0, 0xbd, 0x66, 0x82, 0xa9 } }
EFI_GUID  mTestGuid3 = TEST_GUID_3;

#define   TEST_VAR_1_NAME  L"TestVar1"
#define   TEST_VAR_2_NAME  L"TestVar2"
#define   TEST_VAR_3_NAME  L"TestVar3"

#define   TEST_POLICY_ATTRIBUTES_NULL  0
#define   TEST_POLICY_MIN_SIZE_NULL    0
#define   TEST_POLICY_MAX_SIZE_NULL    MAX_UINT32

#define   TEST_POLICY_MIN_SIZE_10   10
#define   TEST_POLICY_MAX_SIZE_200  200

#define TEST_300_HASHES_STRING  L"##################################################"    \
                                      "##################################################"\
                                      "##################################################"\
                                      "##################################################"\
                                      "##################################################"\
                                      "##################################################"

/// === HELPER FUNCTIONS ===========================================================================

/**
  Helper function to initialize a VARIABLE_POLICY_ENTRY structure with a Name and StateName.

  Takes care of all the messy packing.

  @param[in,out]  Entry
  @param[in]      Name        [Optional]
  @param[in]      StateName   [Optional]

  @retval     TRUE
  @retval     FALSE
  **/
STATIC
BOOLEAN
InitExpVarPolicyStrings (
  EXPANDED_VARIABLE_POLICY_ENTRY *Entry,
  CHAR16 *Name, OPTIONAL
  CHAR16                              *StateName  OPTIONAL
  )
{
  UINTN  NameSize;
  UINTN  StateNameSize;

  NameSize      = Name == NULL ? 0 : StrSize (Name);
  StateNameSize = StateName == NULL ? 0 : StrSize (StateName);

  if ((NameSize > EXPANDED_VARIABLE_POLICY_ENTRY_VAR_NAME_SIZE) || (NameSize > MAX_UINT16) ||
      (StateNameSize > EXPANDED_VARIABLE_POLICY_ENTRY_VAR_NAME_SIZE) || (StateNameSize > MAX_UINT16))
  {
    return FALSE;
  }

  Entry->Header.OffsetToName = sizeof (VARIABLE_POLICY_ENTRY);
  if (StateName != NULL) {
    Entry->Header.OffsetToName += (UINT16)sizeof (VARIABLE_LOCK_ON_VAR_STATE_POLICY) + (UINT16)StateNameSize;
  }

  Entry->Header.Size = Entry->Header.OffsetToName + (UINT16)NameSize;

  CopyMem ((UINT8 *)Entry + Entry->Header.OffsetToName, Name, NameSize);
  if (StateName != NULL) {
    CopyMem ((UINT8 *)Entry + sizeof (VARIABLE_POLICY_ENTRY) + sizeof (VARIABLE_LOCK_ON_VAR_STATE_POLICY), StateName, StateNameSize);
  }

  return TRUE;
}

/**
  Mocked version of GetVariable, for testing.

  @param        VariableName
  @param        VendorGuid
  @param        Attributes
  @param        DataSize
  @param        Data
  **/
EFI_STATUS
EFIAPI
StubGetVariableNull (
  IN     CHAR16 *VariableName,
  IN     EFI_GUID *VendorGuid,
  OUT    UINT32 *Attributes, OPTIONAL
  IN OUT UINTN                       *DataSize,
  OUT    VOID                        *Data           OPTIONAL
  )
{
  UINT32      MockedAttr;
  UINTN       MockedDataSize;
  VOID        *MockedData;
  EFI_STATUS  MockedReturn;

  check_expected_ptr (VariableName);
  check_expected_ptr (VendorGuid);
  check_expected_ptr (DataSize);

  MockedAttr     = (UINT32)mock ();
  MockedDataSize = (UINTN)mock ();
  MockedData     = (VOID *)mock ();
  MockedReturn   = (EFI_STATUS)mock ();

  if (Attributes != NULL) {
    *Attributes = MockedAttr;
  }

  if ((Data != NULL) && !EFI_ERROR (MockedReturn)) {
    CopyMem (Data, MockedData, MockedDataSize);
  }

  *DataSize = MockedDataSize;

  return MockedReturn;
}

//
// Anything you think might be helpful that isn't a test itself.
//

/**
  This is a common setup function that will ensure the library is always initialized
  with the stubbed GetVariable.

  Not used by all test cases, but by most.

  @param    Context
  **/
STATIC
UNIT_TEST_STATUS
EFIAPI
LibInitMocked (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return EFI_ERROR (InitVariablePolicyLib (StubGetVariableNull)) ? UNIT_TEST_ERROR_PREREQUISITE_NOT_MET : UNIT_TEST_PASSED;
}

/**
  Common cleanup function to make sure that the library is always de-initialized prior
  to the next test case.

  @param    Context
  **/
STATIC
VOID
EFIAPI
LibCleanup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  DeinitVariablePolicyLib ();
}

/// === TEST CASES =================================================================================

/// ===== ARCHITECTURAL SUITE ==================================================

/**
  Test Case.

  @param    Context
  **/
UNIT_TEST_STATUS
EFIAPI
ShouldBeAbleToInitAndDeinitTheLibrary (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = InitVariablePolicyLib (StubGetVariableNull);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_TRUE (IsVariablePolicyLibInitialized ());

  Status = DeinitVariablePolicyLib ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_FALSE (IsVariablePolicyLibInitialized ());

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ShouldNotBeAbleToInitializeTheLibraryTwice (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = InitVariablePolicyLib (StubGetVariableNull);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  Status = InitVariablePolicyLib (StubGetVariableNull);
  UT_ASSERT_TRUE (EFI_ERROR (Status));
  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ShouldFailDeinitWithoutInit (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = DeinitVariablePolicyLib ();
  UT_ASSERT_TRUE (EFI_ERROR (Status));
  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ApiCommandsShouldNotRespondIfLibIsUninitialized (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8                         DummyData[8];
  UINT32                        DummyDataSize = sizeof (DummyData);
  SIMPLE_VARIABLE_POLICY_ENTRY  TestPolicy    = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };

  // This test should not start with an initialized library.

  // Verify that all API commands fail.
  UT_ASSERT_TRUE (EFI_ERROR (LockVariablePolicy ()));
  UT_ASSERT_TRUE (EFI_ERROR (DisableVariablePolicy ()));
  UT_ASSERT_TRUE (EFI_ERROR (RegisterVariablePolicy (&TestPolicy.Header)));
  UT_ASSERT_TRUE (EFI_ERROR (DumpVariablePolicy (DummyData, &DummyDataSize)));
  UT_ASSERT_FALSE (IsVariablePolicyInterfaceLocked ());
  UT_ASSERT_FALSE (IsVariablePolicyEnabled ());
  UT_ASSERT_TRUE (
    EFI_ERROR (
      ValidateSetVariable (
        TEST_VAR_1_NAME,
        &mTestGuid1,
        VARIABLE_ATTRIBUTE_NV_BS,
        sizeof (DummyData),
        DummyData
        )
      )
    );

  return UNIT_TEST_PASSED;
}

/// ===== INTERNAL FUNCTION SUITE ==============================================

#ifdef INTERNAL_UNIT_TEST

BOOLEAN
EvaluatePolicyMatch (
  IN CONST  VARIABLE_POLICY_ENTRY  *EvalEntry,
  IN CONST  CHAR16                 *VariableName,
  IN CONST  EFI_GUID               *VendorGuid,
  OUT       UINT8                  *MatchPriority    OPTIONAL
  );

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
PoliciesShouldMatchByNameAndGuid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  MatchCheckPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };
  CHAR16                        *CheckVar1Name = TEST_VAR_1_NAME;
  CHAR16                        *CheckVar2Name = TEST_VAR_2_NAME;

  // Make sure that a different name does not match.
  UT_ASSERT_FALSE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVar2Name, &mTestGuid1, NULL));

  // Make sure that a different GUID does not match.
  UT_ASSERT_FALSE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVar1Name, &mTestGuid2, NULL));

  // Make sure that the same name and GUID match.
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVar1Name, &mTestGuid1, NULL));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
WildcardPoliciesShouldMatchDigits (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  MatchCheckPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (L"Wildcard#VarName##"),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    L"Wildcard#VarName##"
  };
  CHAR16                        *CheckVar1Name = L"Wildcard1VarName12";
  CHAR16                        *CheckVar2Name = L"Wildcard2VarName34";
  CHAR16                        *CheckVarBName = L"WildcardBVarName56";
  CHAR16                        *CheckVarFName = L"WildcardFVarName0A";
  CHAR16                        *CheckVarZName = L"WildcardZVarName56";
  CHAR16                        *CheckVarLName = L"WildcardLVarName56";
  CHAR16                        *CheckVarHName = L"Wildcard#VarName56";

  // Make sure that all hexidecimal sets of wildcard numbers match.
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVar1Name, &mTestGuid1, NULL));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVar2Name, &mTestGuid1, NULL));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVarBName, &mTestGuid1, NULL));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVarFName, &mTestGuid1, NULL));

  // Make sure that the non-number charaters don't match.
  UT_ASSERT_FALSE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVarZName, &mTestGuid1, NULL));
  UT_ASSERT_FALSE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVarLName, &mTestGuid1, NULL));

  // Make sure that '#' signs don't match.
  UT_ASSERT_FALSE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVarHName, &mTestGuid1, NULL));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
WildcardPoliciesShouldMatchDigitsAdvanced (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  MatchCheckPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_300_HASHES_STRING),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_300_HASHES_STRING
  };
  CHAR16                        *CheckShorterString = L"01234567890123456789012345678901234567890123456789";
  CHAR16                        *CheckValidString   = L"01234567890123456789012345678901234567890123456789" \
                                                      "01234567890123456789012345678901234567890123456789" \
                                                      "01234567890123456789012345678901234567890123456789" \
                                                      "01234567890123456789012345678901234567890123456789" \
                                                      "01234567890123456789012345678901234567890123456789" \
                                                      "01234567890123456789012345678901234567890123456789";
  CHAR16  *CheckValidHexString = L"012345678901234567890123456789012345678901234F6789" \
                                 "01234567890123456789012345678901234567890123456789" \
                                 "012345678901ABC56789012345678901234567890123456789" \
                                 "01234567890123456789012345678901234567890123456789" \
                                 "012345678901234567890123456789012345678DEADBEEF789" \
                                 "01234ABCDEF123456789012345678901234567890123456789";
  CHAR16  *CheckLongerString = L"01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789";
  UINT8  MatchPriority;

  // Make sure that the shorter and the longer do not match.
  UT_ASSERT_FALSE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckShorterString, &mTestGuid1, NULL));
  UT_ASSERT_FALSE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckLongerString, &mTestGuid1, NULL));

  // Make sure that the valid one matches and has the expected priority.
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckValidString, &mTestGuid1, &MatchPriority));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckValidHexString, &mTestGuid1, &MatchPriority));
  UT_ASSERT_EQUAL (MatchPriority, MAX_UINT8);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
WildcardPoliciesShouldMatchNamespaces (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VARIABLE_POLICY_ENTRY  MatchCheckPolicy = {
    VARIABLE_POLICY_ENTRY_REVISION,
    sizeof (VARIABLE_POLICY_ENTRY),
    sizeof (VARIABLE_POLICY_ENTRY),
    TEST_GUID_1,
    TEST_POLICY_MIN_SIZE_NULL,
    TEST_POLICY_MAX_SIZE_NULL,
    TEST_POLICY_ATTRIBUTES_NULL,
    TEST_POLICY_ATTRIBUTES_NULL,
    VARIABLE_POLICY_TYPE_NO_LOCK
  };
  CHAR16                 *CheckVar1Name = L"Wildcard1VarName12";
  CHAR16                 *CheckVar2Name = L"Wildcard2VarName34";
  CHAR16                 *CheckVarBName = L"WildcardBVarName56";
  CHAR16                 *CheckVarHName = L"Wildcard#VarName56";

  // Make sure that all names in the same namespace match.
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy, CheckVar1Name, &mTestGuid1, NULL));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy, CheckVar2Name, &mTestGuid1, NULL));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy, CheckVarBName, &mTestGuid1, NULL));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy, CheckVarHName, &mTestGuid1, NULL));

  // Make sure that different namespace doesn't match.
  UT_ASSERT_FALSE (EvaluatePolicyMatch (&MatchCheckPolicy, CheckVar1Name, &mTestGuid2, NULL));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
MatchPrioritiesShouldFollowRules (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  MatchCheckPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (L"Wildcard1VarName12"),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    L"Wildcard1VarName12"
  };
  CHAR16                        CheckVar1Name[] = L"Wildcard1VarName12";
  CHAR16                        MatchVar1Name[] = L"Wildcard1VarName12";
  CHAR16                        MatchVar2Name[] = L"Wildcard#VarName12";
  CHAR16                        MatchVar3Name[] = L"Wildcard#VarName#2";
  CHAR16                        MatchVar4Name[] = L"Wildcard#VarName##";
  UINT8                         MatchPriority;

  // Check with a perfect match.
  CopyMem (&MatchCheckPolicy.Name, MatchVar1Name, sizeof (MatchVar1Name));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVar1Name, &mTestGuid1, &MatchPriority));
  UT_ASSERT_EQUAL (MatchPriority, 0);

  // Check with progressively lower priority matches.
  CopyMem (&MatchCheckPolicy.Name, MatchVar2Name, sizeof (MatchVar2Name));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVar1Name, &mTestGuid1, &MatchPriority));
  UT_ASSERT_EQUAL (MatchPriority, 1);
  CopyMem (&MatchCheckPolicy.Name, MatchVar3Name, sizeof (MatchVar3Name));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVar1Name, &mTestGuid1, &MatchPriority));
  UT_ASSERT_EQUAL (MatchPriority, 2);
  CopyMem (&MatchCheckPolicy.Name, MatchVar4Name, sizeof (MatchVar4Name));
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVar1Name, &mTestGuid1, &MatchPriority));
  UT_ASSERT_EQUAL (MatchPriority, 3);

  // Check against the entire namespace.
  MatchCheckPolicy.Header.Size = sizeof (VARIABLE_POLICY_ENTRY);
  UT_ASSERT_TRUE (EvaluatePolicyMatch (&MatchCheckPolicy.Header, CheckVar1Name, &mTestGuid1, &MatchPriority));
  UT_ASSERT_EQUAL (MatchPriority, MAX_UINT8);

  return UNIT_TEST_PASSED;
}

#endif // INTERNAL_UNIT_TEST

/// === POLICY MANIPULATION SUITE ==============================================

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldAllowNamespaceWildcards (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    L""
  };

  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldAllowStateVarsForNamespaces (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EXPANDED_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      0,    // Will be populated by init helper.
      0,    // Will be populated by init helper.
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE
    },
    {
      TEST_GUID_2,
      1,            // Value
      0             // Padding
    },
    L"",
    L""
  };

  UT_ASSERT_TRUE (InitExpVarPolicyStrings (&ValidationPolicy, NULL, TEST_VAR_2_NAME));

  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectNullPointers (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_EQUAL (RegisterVariablePolicy (NULL), EFI_INVALID_PARAMETER);
  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectBadRevisions (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };

  ValidationPolicy.Header.Version = MAX_UINT32;
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectBadSizes (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };

  ValidationPolicy.Header.Size = sizeof (VARIABLE_POLICY_ENTRY) - 2;
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectBadOffsets (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EXPANDED_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      0,    // Will be populated by init helper.
      0,    // Will be populated by init helper.
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE
    },
    {
      TEST_GUID_2,
      1,            // Value
      0             // Padding
    },
    L"",
    L""
  };

  UT_ASSERT_TRUE (InitExpVarPolicyStrings (&ValidationPolicy, TEST_VAR_1_NAME, TEST_VAR_2_NAME));

  // Check for an offset outside the size bounds.
  ValidationPolicy.Header.OffsetToName = ValidationPolicy.Header.Size + 1;
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  // Check for an offset inside the policy header.
  ValidationPolicy.Header.OffsetToName = sizeof (VARIABLE_POLICY_ENTRY) - 2;
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  // Check for an offset inside the state policy header.
  ValidationPolicy.Header.OffsetToName = sizeof (VARIABLE_POLICY_ENTRY) + 2;
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  // Check for a ridiculous offset.
  ValidationPolicy.Header.OffsetToName = MAX_UINT16;
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectMissingStateStrings (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EXPANDED_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      0,    // Will be populated by init helper.
      0,    // Will be populated by init helper.
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE
    },
    {
      TEST_GUID_2,
      1,            // Value
      0             // Padding
    },
    L"",
    L""
  };

  UT_ASSERT_TRUE (InitExpVarPolicyStrings (&ValidationPolicy, TEST_VAR_1_NAME, TEST_VAR_2_NAME));

  // Remove the state string and copy the Name into it's place.
  // Also adjust the offset.
  ValidationPolicy.Header.Size         = sizeof (VARIABLE_POLICY_ENTRY) + sizeof (VARIABLE_LOCK_ON_VAR_STATE_POLICY) + sizeof (TEST_VAR_1_NAME);
  ValidationPolicy.Header.OffsetToName = sizeof (VARIABLE_POLICY_ENTRY) + sizeof (VARIABLE_LOCK_ON_VAR_STATE_POLICY);
  CopyMem ((UINT8 *)&ValidationPolicy + ValidationPolicy.Header.OffsetToName, TEST_VAR_1_NAME, sizeof (TEST_VAR_1_NAME));

  // Make sure that this structure fails.
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectStringsMissingNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EXPANDED_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      0,    // Will be populated by init helper.
      0,    // Will be populated by init helper.
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE
    },
    {
      TEST_GUID_2,
      1,            // Value
      0             // Padding
    },
    L"",
    L""
  };

  UT_ASSERT_TRUE (InitExpVarPolicyStrings (&ValidationPolicy, TEST_VAR_1_NAME, TEST_VAR_2_NAME));

  // Removing the NULL from the Name should fail.
  ValidationPolicy.Header.Size = ValidationPolicy.Header.Size - sizeof (CHAR16);
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  // Removing the NULL from the State Name is a little trickier.
  // Copy the Name up one byte.
  ValidationPolicy.Header.OffsetToName = ValidationPolicy.Header.OffsetToName - sizeof (CHAR16);
  CopyMem ((UINT8 *)&ValidationPolicy + ValidationPolicy.Header.OffsetToName, TEST_VAR_1_NAME, sizeof (TEST_VAR_1_NAME));
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectMalformedStrings (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EXPANDED_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      0,    // Will be populated by init helper.
      0,    // Will be populated by init helper.
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE
    },
    {
      TEST_GUID_2,
      1,            // Value
      0             // Padding
    },
    L"",
    L""
  };

  UT_ASSERT_TRUE (InitExpVarPolicyStrings (&ValidationPolicy, TEST_VAR_1_NAME, TEST_VAR_2_NAME));

  // Bisecting the NULL from the Name should fail.
  ValidationPolicy.Header.Size = ValidationPolicy.Header.Size - 1;
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  // Bisecting the NULL from the State Name is a little trickier.
  // Copy the Name up one byte.
  ValidationPolicy.Header.OffsetToName = ValidationPolicy.Header.OffsetToName - 1;
  CopyMem ((UINT8 *)&ValidationPolicy + ValidationPolicy.Header.OffsetToName, TEST_VAR_1_NAME, sizeof (TEST_VAR_1_NAME));
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectUnpackedPolicies (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EXPANDED_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      0,    // Will be populated by init helper.
      0,    // Will be populated by init helper.
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE
    },
    {
      TEST_GUID_2,
      1,            // Value
      0             // Padding
    },
    L"",
    L""
  };

  UT_ASSERT_TRUE (InitExpVarPolicyStrings (&ValidationPolicy, TEST_VAR_1_NAME, TEST_VAR_2_NAME));

  // Increase the size and move the Name out a bit.
  ValidationPolicy.Header.Size         = ValidationPolicy.Header.Size + sizeof (CHAR16);
  ValidationPolicy.Header.OffsetToName = ValidationPolicy.Header.OffsetToName + sizeof (CHAR16);
  CopyMem ((UINT8 *)&ValidationPolicy + ValidationPolicy.Header.OffsetToName, TEST_VAR_1_NAME, sizeof (TEST_VAR_1_NAME));
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  // Reintialize without the state policy and try the same test.
  ValidationPolicy.Header.LockPolicyType = VARIABLE_POLICY_TYPE_NO_LOCK;
  UT_ASSERT_TRUE (InitExpVarPolicyStrings (&ValidationPolicy, TEST_VAR_1_NAME, NULL));
  ValidationPolicy.Header.Size         = ValidationPolicy.Header.Size + sizeof (CHAR16);
  ValidationPolicy.Header.OffsetToName = ValidationPolicy.Header.OffsetToName + sizeof (CHAR16);
  CopyMem ((UINT8 *)&ValidationPolicy + ValidationPolicy.Header.OffsetToName, TEST_VAR_1_NAME, sizeof (TEST_VAR_1_NAME));
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectInvalidNameCharacters (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // EXPANDED_VARIABLE_POLICY_ENTRY   ValidationPolicy = {
  //   {
  //     VARIABLE_POLICY_ENTRY_REVISION,
  //     0,    // Will be populated by init helper.
  //     0,    // Will be populated by init helper.
  //     TEST_GUID_1,
  //     TEST_POLICY_MIN_SIZE_NULL,
  //     TEST_POLICY_MAX_SIZE_NULL,
  //     TEST_POLICY_ATTRIBUTES_NULL,
  //     TEST_POLICY_ATTRIBUTES_NULL,
  //     VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE
  //   },
  //   {
  //     TEST_GUID_2,
  //     1,            // Value
  //     0             // Padding
  //   },
  //   L"",
  //   L""
  // };

  // Currently, there are no known invalid characters.
  // '#' in LockPolicy->Name are taken as literal.

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectBadPolicyConstraints (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };

  // Make sure that invalid MAXes are rejected.
  ValidationPolicy.Header.MaxSize = 0;
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectUnknownLockPolicies (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };

  ValidationPolicy.Header.LockPolicyType = VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE + 1;
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);
  ValidationPolicy.Header.LockPolicyType = VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE + 1;
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectPolicesWithTooManyWildcards (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_300_HASHES_STRING),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_300_HASHES_STRING
  };

  // 300 Hashes is currently larger than the possible maximum match priority.
  UT_ASSERT_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
RegisterShouldRejectDuplicatePolicies (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };

  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));
  UT_ASSERT_STATUS_EQUAL (RegisterVariablePolicy (&ValidationPolicy.Header), EFI_ALREADY_STARTED);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
MinAndMaxSizePoliciesShouldBeHonored (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_10,
      TEST_POLICY_MAX_SIZE_200,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };
  EFI_STATUS                    PolicyCheck;
  UINT8                         DummyData[TEST_POLICY_MAX_SIZE_200+1];

  // Without a policy, there should be no constraints on variable creation.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_NV_BS,
                  TEST_POLICY_MAX_SIZE_200+1,
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Set a policy to test against.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  // With a policy, make sure that sizes outsize the target range fail.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_NV_BS,
                  TEST_POLICY_MAX_SIZE_200+1,
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  // With a policy, make sure that sizes outsize the target range fail.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_NV_BS,
                  TEST_POLICY_MIN_SIZE_10-1,
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  // With a policy, make sure a valid variable is still valid.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_NV_BS,
                  TEST_POLICY_MIN_SIZE_10+1,
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
AttributeMustPoliciesShouldBeHonored (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };
  EFI_STATUS                    PolicyCheck;
  UINT8                         DummyData[12];

  // Without a policy, there should be no constraints on variable creation.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  TEST_POLICY_ATTRIBUTES_NULL,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Set a policy to test against.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  // With a policy, make sure that no attributes fail.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  TEST_POLICY_ATTRIBUTES_NULL,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  // With a policy, make sure that some -- but not all -- attributes fail.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  // With a policy, make sure that all attributes pass.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_NV_BS_RT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // With a policy, make sure that all attributes -- plus some -- pass.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
AttributeCantPoliciesShouldBeHonored (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };
  EFI_STATUS                    PolicyCheck;
  UINT8                         DummyData[12];

  // Without a policy, there should be no constraints on variable creation.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Set a policy to test against.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  // With a policy, make sure that forbidden attributes fail.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  // With a policy, make sure that a mixture of attributes -- including the forbidden -- fail.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  // With a policy, make sure that attributes without the forbidden pass.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_NV_BS_RT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
VariablesShouldBeDeletableRegardlessOfSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_10,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };
  EFI_STATUS                    PolicyCheck;
  UINT8                         DummyData[TEST_POLICY_MAX_SIZE_200+1];

  // Create a policy enforcing a minimum variable size.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  // Make sure that a normal set would fail.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_NV_BS,
                  TEST_POLICY_MIN_SIZE_10-1,
                  DummyData
                  );
  UT_ASSERT_STATUS_EQUAL (PolicyCheck, EFI_INVALID_PARAMETER);

  // Now make sure that a delete would succeed.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_NV_BS,
                  0,
                  NULL
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
LockNowPoliciesShouldBeHonored (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_NOW
    },
    TEST_VAR_1_NAME
  };
  EFI_STATUS                    PolicyCheck;
  UINT8                         DummyData[12];

  // Without a policy, there should be no constraints on variable creation.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Set a policy to test against.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  // With a policy, make sure that writes immediately fail.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
LockOnCreatePoliciesShouldBeHonored (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_ON_CREATE
    },
    TEST_VAR_1_NAME
  };
  EFI_STATUS                    PolicyCheck;
  UINT8                         DummyData[12];
  UINTN                         ExpectedDataSize;

  // Without a policy, there should be no constraints on variable creation.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Set a policy to test against.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  // Set consistent expectations on what the calls are looking for.
  expect_memory_count (StubGetVariableNull, VariableName, TEST_VAR_1_NAME, sizeof (TEST_VAR_1_NAME), 2);
  expect_memory_count (StubGetVariableNull, VendorGuid, &mTestGuid1, sizeof (mTestGuid1), 2);
  ExpectedDataSize = 0;
  expect_memory_count (StubGetVariableNull, DataSize, &ExpectedDataSize, sizeof (ExpectedDataSize), 2);

  // With a policy, make sure that writes still work, since the variable doesn't exist.
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, 0);                               // Size
  will_return (StubGetVariableNull, NULL);                            // DataPtr
  will_return (StubGetVariableNull, EFI_NOT_FOUND);                   // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // With a policy, make sure that a call with an "existing" variable fails.
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, 10);                              // Size
  will_return (StubGetVariableNull, NULL);                            // DataPtr
  will_return (StubGetVariableNull, EFI_BUFFER_TOO_SMALL);            // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
LockOnStatePoliciesShouldBeHonored (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EXPANDED_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      0,    // Will be populated by init helper.
      0,    // Will be populated by init helper.
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE
    },
    {
      TEST_GUID_2,
      20,           // Value
      0             // Padding
    },
    L"",
    L""
  };
  EFI_STATUS                      PolicyCheck;
  UINT8                           DummyData[12];
  UINT8                           ValidationStateVar;
  UINTN                           ExpectedDataSize;

  UT_ASSERT_TRUE (InitExpVarPolicyStrings (&ValidationPolicy, TEST_VAR_1_NAME, TEST_VAR_2_NAME));

  // Without a policy, there should be no constraints on variable creation.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Set a policy to test against.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  // Set consistent expectations on what the calls are looking for.
  expect_memory_count (StubGetVariableNull, VariableName, TEST_VAR_2_NAME, sizeof (TEST_VAR_2_NAME), 5);
  expect_memory_count (StubGetVariableNull, VendorGuid, &mTestGuid2, sizeof (mTestGuid2), 5);
  ExpectedDataSize = 1;
  expect_memory_count (StubGetVariableNull, DataSize, &ExpectedDataSize, sizeof (ExpectedDataSize), 5);

  // With a policy, make sure that writes still work, since the variable doesn't exist.
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, 0);                               // Size
  will_return (StubGetVariableNull, NULL);                            // DataPtr
  will_return (StubGetVariableNull, EFI_NOT_FOUND);                   // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // With a policy, make sure that a state variable that's too large doesn't lock the variable.
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, 10);                              // Size
  will_return (StubGetVariableNull, NULL);                            // DataPtr
  will_return (StubGetVariableNull, EFI_BUFFER_TOO_SMALL);            // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // With a policy, check a state variable with the wrong value.
  ValidationStateVar = 0;
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, sizeof (ValidationStateVar));     // Size
  will_return (StubGetVariableNull, &ValidationStateVar);             // DataPtr
  will_return (StubGetVariableNull, EFI_SUCCESS);                     // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // With a policy, check a state variable with another wrong value.
  ValidationStateVar = 10;
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, sizeof (ValidationStateVar));     // Size
  will_return (StubGetVariableNull, &ValidationStateVar);             // DataPtr
  will_return (StubGetVariableNull, EFI_SUCCESS);                     // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // With a policy, make sure that a call with a correct state variable fails.
  ValidationStateVar = 20;
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, sizeof (ValidationStateVar));     // Size
  will_return (StubGetVariableNull, &ValidationStateVar);             // DataPtr
  will_return (StubGetVariableNull, EFI_SUCCESS);                     // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
LockOnStatePoliciesShouldApplyToNamespaces (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EXPANDED_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      0,    // Will be populated by init helper.
      0,    // Will be populated by init helper.
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE
    },
    {
      TEST_GUID_2,
      20,           // Value
      0             // Padding
    },
    L"",
    L""
  };
  EFI_STATUS                      PolicyCheck;
  UINT8                           DummyData[12];
  UINT8                           ValidationStateVar;
  UINTN                           ExpectedDataSize;

  UT_ASSERT_TRUE (InitExpVarPolicyStrings (&ValidationPolicy, NULL, TEST_VAR_2_NAME));

  // Without a policy, there should be no constraints on variable creation.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_3_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Set a policy to test against.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  // Set consistent expectations on what the calls are looking for.
  expect_memory_count (StubGetVariableNull, VariableName, TEST_VAR_2_NAME, sizeof (TEST_VAR_2_NAME), 4);
  expect_memory_count (StubGetVariableNull, VendorGuid, &mTestGuid2, sizeof (mTestGuid2), 4);
  ExpectedDataSize = 1;
  expect_memory_count (StubGetVariableNull, DataSize, &ExpectedDataSize, sizeof (ExpectedDataSize), 4);

  // With a policy, make sure that writes still work, since the variable doesn't exist.
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, 0);                               // Size
  will_return (StubGetVariableNull, NULL);                            // DataPtr
  will_return (StubGetVariableNull, EFI_NOT_FOUND);                   // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, 0);                               // Size
  will_return (StubGetVariableNull, NULL);                            // DataPtr
  will_return (StubGetVariableNull, EFI_NOT_FOUND);                   // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_3_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // With a policy, make sure that a call with a correct state variable fails.
  ValidationStateVar = 20;
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, sizeof (ValidationStateVar));     // Size
  will_return (StubGetVariableNull, &ValidationStateVar);             // DataPtr
  will_return (StubGetVariableNull, EFI_SUCCESS);                     // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, sizeof (ValidationStateVar));     // Size
  will_return (StubGetVariableNull, &ValidationStateVar);             // DataPtr
  will_return (StubGetVariableNull, EFI_SUCCESS);                     // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_3_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
LockOnStateShouldHandleErrorsGracefully (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EXPANDED_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      0,    // Will be populated by init helper.
      0,    // Will be populated by init helper.
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE
    },
    {
      TEST_GUID_2,
      20,           // Value
      0             // Padding
    },
    L"",
    L""
  };
  EFI_STATUS                      PolicyCheck;
  UINT8                           DummyData[12];

  UT_ASSERT_TRUE (InitExpVarPolicyStrings (&ValidationPolicy, TEST_VAR_1_NAME, TEST_VAR_2_NAME));

  // Without a policy, there should be no constraints on variable creation.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Set a policy to test against.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  // Configure the stub to not care about parameters. We're testing errors.
  expect_any_always (StubGetVariableNull, VariableName);
  expect_any_always (StubGetVariableNull, VendorGuid);
  expect_any_always (StubGetVariableNull, DataSize);

  // With a policy, make sure that writes still work, since the variable doesn't exist.
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, 0);                               // Size
  will_return (StubGetVariableNull, NULL);                            // DataPtr
  will_return (StubGetVariableNull, EFI_NOT_FOUND);                   // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Verify that state variables that are the wrong size won't lock the variable.
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, 0);                               // Size
  will_return (StubGetVariableNull, NULL);                            // DataPtr
  will_return (StubGetVariableNull, EFI_BUFFER_TOO_SMALL);            // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Verify that unexpected errors default to locked.
  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, 0);                               // Size
  will_return (StubGetVariableNull, NULL);                            // DataPtr
  will_return (StubGetVariableNull, EFI_UNSUPPORTED);                 // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  will_return (StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL);     // Attributes
  will_return (StubGetVariableNull, 0);                               // Size
  will_return (StubGetVariableNull, NULL);                            // DataPtr
  will_return (StubGetVariableNull, EFI_NOT_READY);                   // Status
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
BestMatchPriorityShouldBeObeyed (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  ValidationPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (L"Wild12Card34Placeholder"),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    L"Wild12Card34Placeholder"
  };
  EFI_STATUS                    PolicyCheck;
  UINT8                         DummyData[70];
  CHAR16                        *PolicyName     = (CHAR16 *)((UINT8 *)&ValidationPolicy + sizeof (VARIABLE_POLICY_ENTRY));
  UINTN                         PolicyNameSize  = sizeof (L"Wild12Card34Placeholder");
  CHAR16                        *FourWildcards  = L"Wild##Card##Placeholder";
  CHAR16                        *ThreeWildcards = L"Wild##Card#4Placeholder";
  CHAR16                        *TwoWildcards   = L"Wild##Card34Placeholder";
  CHAR16                        *OneWildcard    = L"Wild#2Card34Placeholder";
  CHAR16                        *NoWildcards    = L"Wild12Card34Placeholder";

  // Create all of the policies from least restrictive to most restrictive.
  // NoWildcards should be the most restrictive.
  ValidationPolicy.Header.MaxSize = 60;
  ValidationPolicy.Header.Size    = ValidationPolicy.Header.OffsetToName;
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));
  ValidationPolicy.Header.Size   += (UINT16)PolicyNameSize;
  ValidationPolicy.Header.MaxSize = 50;
  CopyMem (PolicyName, FourWildcards, PolicyNameSize);
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));
  ValidationPolicy.Header.MaxSize = 40;
  CopyMem (PolicyName, ThreeWildcards, PolicyNameSize);
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));
  ValidationPolicy.Header.MaxSize = 30;
  CopyMem (PolicyName, TwoWildcards, PolicyNameSize);
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));
  ValidationPolicy.Header.MaxSize = 20;
  CopyMem (PolicyName, OneWildcard, PolicyNameSize);
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));
  ValidationPolicy.Header.MaxSize = 10;
  CopyMem (PolicyName, NoWildcards, PolicyNameSize);
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&ValidationPolicy.Header));

  // Verify that variables only matching the namespace have the most flexible policy.
  PolicyCheck = ValidateSetVariable (
                  L"ArbitraryName",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  65,
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));
  PolicyCheck = ValidateSetVariable (
                  L"ArbitraryName",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  55,
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  // Verify that variables matching increasing characters get increasing policy restrictions.
  PolicyCheck = ValidateSetVariable (
                  L"Wild77Card77Placeholder",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  55,
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));
  PolicyCheck = ValidateSetVariable (
                  L"Wild77Card77Placeholder",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  45,
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  PolicyCheck = ValidateSetVariable (
                  L"Wild77Card74Placeholder",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  45,
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));
  PolicyCheck = ValidateSetVariable (
                  L"Wild77Card74Placeholder",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  35,
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  PolicyCheck = ValidateSetVariable (
                  L"Wild77Card34Placeholder",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  35,
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));
  PolicyCheck = ValidateSetVariable (
                  L"Wild77Card34Placeholder",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  25,
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  PolicyCheck = ValidateSetVariable (
                  L"Wild72Card34Placeholder",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  25,
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));
  PolicyCheck = ValidateSetVariable (
                  L"Wild72Card34Placeholder",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  15,
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  PolicyCheck = ValidateSetVariable (
                  L"Wild12Card34Placeholder",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  15,
                  DummyData
                  );
  UT_ASSERT_TRUE (EFI_ERROR (PolicyCheck));
  PolicyCheck = ValidateSetVariable (
                  L"Wild12Card34Placeholder",
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_BS_RT_AT,
                  5,
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  return UNIT_TEST_PASSED;
}

/// === POLICY UTILITY SUITE ===================================================

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ShouldBeAbleToLockInterface (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  TestPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_NULL,
      TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };

  // Make sure it's not already locked.
  UT_ASSERT_FALSE (IsVariablePolicyInterfaceLocked ());
  // Lock it.
  UT_ASSERT_NOT_EFI_ERROR (LockVariablePolicy ());
  // Verify that it's locked.
  UT_ASSERT_TRUE (IsVariablePolicyInterfaceLocked ());

  // Verify that all state-changing commands fail.
  UT_ASSERT_TRUE (EFI_ERROR (LockVariablePolicy ()));
  UT_ASSERT_TRUE (EFI_ERROR (DisableVariablePolicy ()));
  UT_ASSERT_TRUE (EFI_ERROR (RegisterVariablePolicy (&TestPolicy.Header)));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ShouldBeAbleToDisablePolicyEnforcement (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  TestPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_10,
      TEST_POLICY_MAX_SIZE_200,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };
  EFI_STATUS                    PolicyCheck;
  UINT8                         DummyData[TEST_POLICY_MIN_SIZE_10-1];

  // Make sure that the policy enforcement is currently enabled.
  UT_ASSERT_TRUE (IsVariablePolicyEnabled ());
  // Add a policy before it's disabled.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&TestPolicy.Header));
  // Disable the policy enforcement.
  UT_ASSERT_NOT_EFI_ERROR (DisableVariablePolicy ());
  // Make sure that the policy enforcement is currently disabled.
  UT_ASSERT_FALSE (IsVariablePolicyEnabled ());

  // Check to make sure that a policy violation still passes.
  PolicyCheck = ValidateSetVariable (
                  TEST_VAR_1_NAME,
                  &mTestGuid1,
                  VARIABLE_ATTRIBUTE_NV_BS,
                  sizeof (DummyData),
                  DummyData
                  );
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ShouldNotBeAbleToDisablePoliciesTwice (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Make sure that the policy enforcement is currently enabled.
  UT_ASSERT_TRUE (IsVariablePolicyEnabled ());
  // Disable the policy enforcement.
  UT_ASSERT_NOT_EFI_ERROR (DisableVariablePolicy ());
  // Make sure that the policy enforcement is currently disabled.
  UT_ASSERT_FALSE (IsVariablePolicyEnabled ());
  // Try to disable again and verify failure.
  UT_ASSERT_TRUE (EFI_ERROR (DisableVariablePolicy ()));

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ShouldBeAbleToAddNewPoliciesAfterDisabled (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  TestPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_10,
      TEST_POLICY_MAX_SIZE_200,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };
  EFI_STATUS                    PolicyCheck;

  // Make sure that the policy enforcement is currently enabled.
  UT_ASSERT_TRUE (IsVariablePolicyEnabled ());
  // Disable the policy enforcement.
  UT_ASSERT_NOT_EFI_ERROR (DisableVariablePolicy ());

  // Make sure that new policy creation still works, it just won't be enforced.
  PolicyCheck = RegisterVariablePolicy (&TestPolicy.Header);
  UT_ASSERT_NOT_EFI_ERROR (PolicyCheck);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ShouldBeAbleToLockAfterDisabled (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Make sure that the policy enforcement is currently enabled.
  UT_ASSERT_TRUE (IsVariablePolicyEnabled ());
  // Disable the policy enforcement.
  UT_ASSERT_NOT_EFI_ERROR (DisableVariablePolicy ());

  // Make sure that we can lock in this state.
  UT_ASSERT_FALSE (IsVariablePolicyInterfaceLocked ());
  UT_ASSERT_NOT_EFI_ERROR (LockVariablePolicy ());
  UT_ASSERT_TRUE (IsVariablePolicyInterfaceLocked ());

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ShouldBeAbleToDumpThePolicyTable (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  TestPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_10,
      TEST_POLICY_MAX_SIZE_200,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };
  EFI_STATUS                    PolicyCheck;
  UINT32                        DumpSize;
  UINT32                        BufferSize;
  VOID                          *DumpBuffer;

  // For good measure, test some parameter validation.
  UT_ASSERT_STATUS_EQUAL (DumpVariablePolicy (NULL, NULL), EFI_INVALID_PARAMETER);
  DumpSize = 10;
  UT_ASSERT_STATUS_EQUAL (DumpVariablePolicy (NULL, &DumpSize), EFI_INVALID_PARAMETER);

  // Now for the actual test case.

  // Allocate a buffer to hold the output.
  BufferSize = sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME);
  DumpBuffer = AllocatePool (BufferSize);
  UT_ASSERT_NOT_EQUAL (DumpBuffer, NULL);

  // Verify that the current table size is 0.
  DumpSize = BufferSize;
  UT_ASSERT_NOT_EFI_ERROR (DumpVariablePolicy (DumpBuffer, &DumpSize));
  UT_ASSERT_EQUAL (DumpSize, 0);

  // Now, set a new policy.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&TestPolicy.Header));

  // Make sure that the new return is non-zero and fails as expected.
  DumpSize    = 0;
  PolicyCheck = DumpVariablePolicy (NULL, &DumpSize);
  UT_ASSERT_STATUS_EQUAL (PolicyCheck, EFI_BUFFER_TOO_SMALL);
  UT_ASSERT_EQUAL (DumpSize, BufferSize);

  // Now verify that we can fetch the dump.
  DumpSize = BufferSize;
  UT_ASSERT_NOT_EFI_ERROR (DumpVariablePolicy (DumpBuffer, &DumpSize));
  UT_ASSERT_EQUAL (DumpSize, BufferSize);
  UT_ASSERT_MEM_EQUAL (&TestPolicy, DumpBuffer, BufferSize);

  // Always put away your toys.
  FreePool (DumpBuffer);

  return UNIT_TEST_PASSED;
}

/**
  Test Case.

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ShouldBeAbleToDumpThePolicyTableAfterDisabled (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SIMPLE_VARIABLE_POLICY_ENTRY  TestPolicy = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_1_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_1,
      TEST_POLICY_MIN_SIZE_10,
      TEST_POLICY_MAX_SIZE_200,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_1_NAME
  };
  SIMPLE_VARIABLE_POLICY_ENTRY  TestPolicy2 = {
    {
      VARIABLE_POLICY_ENTRY_REVISION,
      sizeof (VARIABLE_POLICY_ENTRY) + sizeof (TEST_VAR_2_NAME),
      sizeof (VARIABLE_POLICY_ENTRY),
      TEST_GUID_2,
      TEST_POLICY_MIN_SIZE_10,
      TEST_POLICY_MAX_SIZE_200,
      TEST_POLICY_ATTRIBUTES_NULL,
      TEST_POLICY_ATTRIBUTES_NULL,
      VARIABLE_POLICY_TYPE_NO_LOCK
    },
    TEST_VAR_2_NAME
  };
  EFI_STATUS                    PolicyCheck;
  UINT32                        DumpSize;
  VOID                          *DumpBuffer;

  DumpBuffer = NULL;
  DumpSize   = 0;

  // Register a new policy.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&TestPolicy.Header));
  // Make sure that we can dump the policy.
  PolicyCheck = DumpVariablePolicy (DumpBuffer, &DumpSize);
  UT_ASSERT_STATUS_EQUAL (PolicyCheck, EFI_BUFFER_TOO_SMALL);
  DumpBuffer = AllocatePool (DumpSize);
  UT_ASSERT_NOT_EFI_ERROR (DumpVariablePolicy (DumpBuffer, &DumpSize));
  UT_ASSERT_MEM_EQUAL (DumpBuffer, &TestPolicy, DumpSize);

  // Clean up from this step.
  FreePool (DumpBuffer);
  DumpBuffer = NULL;
  DumpSize   = 0;

  // Now disable the engine.
  DisableVariablePolicy ();

  // Now register a new policy and make sure that both can be dumped.
  UT_ASSERT_NOT_EFI_ERROR (RegisterVariablePolicy (&TestPolicy2.Header));
  // Make sure that we can dump the policy.
  PolicyCheck = DumpVariablePolicy (DumpBuffer, &DumpSize);
  UT_ASSERT_STATUS_EQUAL (PolicyCheck, EFI_BUFFER_TOO_SMALL);
  DumpBuffer = AllocatePool (DumpSize);
  UT_ASSERT_NOT_EFI_ERROR (DumpVariablePolicy (DumpBuffer, &DumpSize));

  // Finally, make sure that both policies are in the dump.
  UT_ASSERT_MEM_EQUAL (DumpBuffer, &TestPolicy, TestPolicy.Header.Size);
  UT_ASSERT_MEM_EQUAL (
    (UINT8 *)DumpBuffer + TestPolicy.Header.Size,
    &TestPolicy2,
    TestPolicy2.Header.Size
    );

  // Always put away your toys.
  FreePool (DumpBuffer);

  return UNIT_TEST_PASSED;
}

/// === TEST ENGINE ================================================================================

/**
  Unit Test Entry Point.
  **/
EFI_STATUS
EFIAPI
UnitTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework = NULL;
  UNIT_TEST_SUITE_HANDLE      ArchTests;
  UNIT_TEST_SUITE_HANDLE      PolicyTests;
  UNIT_TEST_SUITE_HANDLE      UtilityTests;

 #ifdef INTERNAL_UNIT_TEST
  UNIT_TEST_SUITE_HANDLE  InternalTests;
 #endif // INTERNAL_UNIT_TEST

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Add all test suites and tests.
  //
  Status = CreateUnitTestSuite (&ArchTests, Framework, "Variable Policy Architectural Tests", "VarPolicy.Arch", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for ArchTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (
    ArchTests,
    "Deinitialization should fail if not previously initialized",
    "VarPolicy.Arch.OnlyDeinit",
    ShouldFailDeinitWithoutInit,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    ArchTests,
    "Initialization followed by deinitialization should succeed",
    "VarPolicy.Arch.InitDeinit",
    ShouldBeAbleToInitAndDeinitTheLibrary,
    NULL,
    NULL,
    NULL
    );
  AddTestCase (
    ArchTests,
    "The initialization function fail if called twice without a deinit",
    "VarPolicy.Arch.InitTwice",
    ShouldNotBeAbleToInitializeTheLibraryTwice,
    NULL,
    LibCleanup,
    NULL
    );
  AddTestCase (
    ArchTests,
    "API functions should be unavailable until library is initialized",
    "VarPolicy.Arch.UninitApiOff",
    ApiCommandsShouldNotRespondIfLibIsUninitialized,
    NULL,
    LibCleanup,
    NULL
    );

 #ifdef INTERNAL_UNIT_TEST
  Status = CreateUnitTestSuite (&InternalTests, Framework, "Variable Policy Internal Tests", "VarPolicy.Internal", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for InternalTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (
    InternalTests,
    "Policy matching should use name and GUID",
    "VarPolicy.Internal.NameGuid",
    PoliciesShouldMatchByNameAndGuid,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    InternalTests,
    "# sign wildcards should match digits",
    "VarPolicy.Internal.WildDigits",
    WildcardPoliciesShouldMatchDigits,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    InternalTests,
    "Digit wildcards should check edge cases",
    "VarPolicy.Internal.WildDigitsAdvanced",
    WildcardPoliciesShouldMatchDigitsAdvanced,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    InternalTests,
    "Empty names should match an entire namespace",
    "VarPolicy.Internal.WildNamespace",
    WildcardPoliciesShouldMatchNamespaces,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    InternalTests,
    "Match priority should weight correctly based on wildcards",
    "VarPolicy.Internal.Priorities",
    MatchPrioritiesShouldFollowRules,
    LibInitMocked,
    LibCleanup,
    NULL
    );
 #endif // INTERNAL_UNIT_TEST

  Status = CreateUnitTestSuite (&PolicyTests, Framework, "Variable Policy Manipulation Tests", "VarPolicy.Policy", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for PolicyTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (
    PolicyTests,
    "RegisterShouldAllowNamespaceWildcards",
    "VarPolicy.Policy.AllowNamespace",
    RegisterShouldAllowNamespaceWildcards,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldAllowStateVarsForNamespaces",
    "VarPolicy.Policy.AllowStateNamespace",
    RegisterShouldAllowStateVarsForNamespaces,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectNullPointers",
    "VarPolicy.Policy.NullPointers",
    RegisterShouldRejectNullPointers,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectBadRevisions",
    "VarPolicy.Policy.BadRevisions",
    RegisterShouldRejectBadRevisions,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectBadSizes",
    "VarPolicy.Policy.BadSizes",
    RegisterShouldRejectBadSizes,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectBadOffsets",
    "VarPolicy.Policy.BadOffsets",
    RegisterShouldRejectBadOffsets,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectMissingStateStrings",
    "VarPolicy.Policy.MissingStateString",
    RegisterShouldRejectMissingStateStrings,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectStringsMissingNull",
    "VarPolicy.Policy.MissingNull",
    RegisterShouldRejectStringsMissingNull,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectMalformedStrings",
    "VarPolicy.Policy.MalformedStrings",
    RegisterShouldRejectMalformedStrings,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectUnpackedPolicies",
    "VarPolicy.Policy.PolicyPacking",
    RegisterShouldRejectUnpackedPolicies,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectInvalidNameCharacters",
    "VarPolicy.Policy.InvalidCharacters",
    RegisterShouldRejectInvalidNameCharacters,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectBadPolicyConstraints",
    "VarPolicy.Policy.BadConstraints",
    RegisterShouldRejectBadPolicyConstraints,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectUnknownLockPolicies",
    "VarPolicy.Policy.BadLocks",
    RegisterShouldRejectUnknownLockPolicies,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectPolicesWithTooManyWildcards",
    "VarPolicy.Policy.TooManyWildcards",
    RegisterShouldRejectPolicesWithTooManyWildcards,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "RegisterShouldRejectDuplicatePolicies",
    "VarPolicy.Policy.DuplicatePolicies",
    RegisterShouldRejectDuplicatePolicies,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "Variables that exceed min or max sizes should be rejected",
    "VarPolicy.Policy.MinMax",
    MinAndMaxSizePoliciesShouldBeHonored,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "AttributeMustPoliciesShouldBeHonored",
    "VarPolicy.Policy.AttrMust",
    AttributeMustPoliciesShouldBeHonored,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "AttributeCantPoliciesShouldBeHonored",
    "VarPolicy.Policy.AttrCant",
    AttributeCantPoliciesShouldBeHonored,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "VariablesShouldBeDeletableRegardlessOfSize",
    "VarPolicy.Policy.DeleteIgnoreSize",
    VariablesShouldBeDeletableRegardlessOfSize,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "LockNowPoliciesShouldBeHonored",
    "VarPolicy.Policy.VARIABLE_POLICY_TYPE_LOCK_NOW",
    LockNowPoliciesShouldBeHonored,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "LockOnCreatePoliciesShouldBeHonored",
    "VarPolicy.Policy.VARIABLE_POLICY_TYPE_LOCK_ON_CREATE",
    LockOnCreatePoliciesShouldBeHonored,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "LockOnStatePoliciesShouldBeHonored",
    "VarPolicy.Policy.LockState",
    LockOnStatePoliciesShouldBeHonored,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "LockOnStatePoliciesShouldApplyToNamespaces",
    "VarPolicy.Policy.NamespaceLockState",
    LockOnStatePoliciesShouldApplyToNamespaces,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "LockOnStateShouldHandleErrorsGracefully",
    "VarPolicy.Policy.LockStateErrors",
    LockOnStateShouldHandleErrorsGracefully,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    PolicyTests,
    "BestMatchPriorityShouldBeObeyed",
    "VarPolicy.Policy.BestMatch",
    BestMatchPriorityShouldBeObeyed,
    LibInitMocked,
    LibCleanup,
    NULL
    );

  Status = CreateUnitTestSuite (&UtilityTests, Framework, "Variable Policy Utility Tests", "VarPolicy.Utility", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for UtilityTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (
    UtilityTests,
    "API commands that change state should not respond after interface is locked",
    "VarPolicy.Utility.InterfaceLock",
    ShouldBeAbleToLockInterface,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    UtilityTests,
    "All policies should pass once enforcement is disabled",
    "VarPolicy.Utility.DisableEnforcement",
    ShouldBeAbleToDisablePolicyEnforcement,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    UtilityTests,
    "Disabling enforcement twice should produce an error",
    "VarPolicy.Utility.DisableEnforcementTwice",
    ShouldNotBeAbleToDisablePoliciesTwice,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    UtilityTests,
    "ShouldBeAbleToAddNewPoliciesAfterDisabled",
    "VarPolicy.Utility.AddAfterDisable",
    ShouldBeAbleToAddNewPoliciesAfterDisabled,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    UtilityTests,
    "ShouldBeAbleToLockAfterDisabled",
    "VarPolicy.Utility.LockAfterDisable",
    ShouldBeAbleToLockAfterDisabled,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    UtilityTests,
    "Should be able to dump the policy table",
    "VarPolicy.Utility.DumpTable",
    ShouldBeAbleToDumpThePolicyTable,
    LibInitMocked,
    LibCleanup,
    NULL
    );
  AddTestCase (
    UtilityTests,
    "ShouldBeAbleToDumpThePolicyTableAfterDisabled",
    "VarPolicy.Utility.DumpTableAfterDisable",
    ShouldBeAbleToDumpThePolicyTableAfterDisabled,
    LibInitMocked,
    LibCleanup,
    NULL
    );

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);
  ASSERT_EFI_ERROR (Status);

EXIT:
  if (Framework != NULL) {
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
  return UnitTestMain ();
}
