/** @file

  Unit tests for the PRM Context Buffer Library.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>

#include <Guid/ZeroGuid.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/PrmContextBufferLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UnitTestLib.h>
#include <Protocol/PrmConfig.h>

#define UNIT_TEST_NAME     "PRM Context Buffer Library Unit Test"
#define UNIT_TEST_VERSION  "0.1"

/// === TEST DATA ==================================================================================

EFI_HANDLE  mTestProtocolHandle;

// *----------------------------------------------------------------------------------*
// * Context Structures                                                               *
// *----------------------------------------------------------------------------------*
typedef struct {
  PRM_MODULE_CONTEXT_BUFFERS    *InstallationStructure;
  EFI_HANDLE                    Handle;
  PRM_GUID_SEARCH_TYPE          GuidSearchType;
  EFI_GUID                      *Guid;
  EFI_GUID                      *ExpectedModuleGuid;
  EFI_STATUS                    ExpectedStatus;
} PRM_CONTEXT_BUFFERS_TEST_CONTEXT;

typedef struct {
  EFI_GUID                      *HandlerGuid;
  PRM_MODULE_CONTEXT_BUFFERS    *ContextBuffers;
  PRM_CONTEXT_BUFFER            *ExpectedContextBuffer;
  EFI_STATUS                    ExpectedStatus;
} PRM_CONTEXT_BUFFER_TEST_CONTEXT;

// *----------------------------------------------------------------------------------*
// * Test GUIDs                                                                       *
// *----------------------------------------------------------------------------------*

// {52960b90-2f3a-4917-b91a-ed5f599a8809}
#define HANDLER_TEST_GUID_1  {0x52960b90, 0x2f3a, 0x4917, { 0xb9, 0x1a, 0xed, 0x5f, 0x59, 0x9a, 0x88, 0x09 }}
EFI_GUID  mHandlerTestGuid1 = HANDLER_TEST_GUID_1;

// {9316a80d-06dc-417b-b21d-6b3c2ae4ed6f}
#define HANDLER_TEST_GUID_2  {0x9316a80d, 0x06dc, 0x417b, { 0xb2, 0x1d, 0x6b, 0x3c, 0x2a, 0xe4, 0xed, 0x6f }}
EFI_GUID  mHandlerTestGuid2 = HANDLER_TEST_GUID_2;

// {d32ac8ba-6cc6-456f-9ed9-9233fa310434}
#define HANDLER_TEST_GUID_3  {0xd32ac8ba, 0x6cc6, 0x456f, { 0x9e, 0xd9, 0x92, 0x33, 0xfa, 0x31, 0x04, 0x34 }}
EFI_GUID  mHandlerTestGuid3 = HANDLER_TEST_GUID_3;

// {faadaa95-070b-4a34-a919-18305dc07370}
#define MODULE_TEST_GUID_1  {0xfaadaa95, 0x070b, 0x4a34, { 0xa9, 0x19, 0x18, 0x30, 0x5d, 0xc0, 0x73, 0x70 }}
EFI_GUID  mModuleTestGuid1 = MODULE_TEST_GUID_1;

// {0ea24584-731c-4863-9100-75780af509a7}
#define MODULE_TEST_GUID_2  {0x0ea24584, 0x731c, 0x4863, { 0x91, 0x00, 0x75, 0x78, 0x0a, 0xf5, 0x09, 0xa7 }}
EFI_GUID  mModuleTestGuid2 = MODULE_TEST_GUID_2;

// {f456b7a1-82a6-4427-8486-87e3a602df43}
#define MODULE_TEST_GUID_3  {0xf456b7a1, 0x82a6, 0x4427, { 0x84, 0x86, 0x87, 0xe3, 0xa6, 0x02, 0xdf, 0x43 }}
EFI_GUID  mModuleTestGuid3 = MODULE_TEST_GUID_3;

// {4a941a9c-9dcf-471b-94b5-d9e2d8c64a1b}
#define NEGATIVE_TEST_GUID  {0x4a941a9c, 0x9dcf, 0x471b,  {0x94, 0xb5, 0xd9, 0xe2, 0xd8, 0xc6, 0x4a, 0x1b }}
EFI_GUID  mNegativeTestGuid = NEGATIVE_TEST_GUID;

// *----------------------------------------------------------------------------------*
// * PRM Static Test Structures                                                       *
// *----------------------------------------------------------------------------------*

PRM_DATA_BUFFER  mTestStaticDataBuffer1 = {
  {
    PRM_DATA_BUFFER_HEADER_SIGNATURE,
    sizeof (PRM_DATA_BUFFER)
  }
  // No data in the buffer (only a header)
};

PRM_CONTEXT_BUFFER  mTestPrmContextBuffer1 = {
  PRM_CONTEXT_BUFFER_SIGNATURE,             // Signature
  PRM_CONTEXT_BUFFER_INTERFACE_VERSION,     // Version
  0,                                        // Reserved
  HANDLER_TEST_GUID_1,                      // HandlerGuid
  &mTestStaticDataBuffer1                   // StaticDataBuffer
};

PRM_CONTEXT_BUFFER  mTestPrmContextBuffer2[2] = {
  // Context buffer #1
  {
    PRM_CONTEXT_BUFFER_SIGNATURE,           // Signature
    PRM_CONTEXT_BUFFER_INTERFACE_VERSION,   // Version
    0,                                      // Reserved
    HANDLER_TEST_GUID_2,                    // HandlerGuid
    NULL                                    // StaticDataBuffer
  },
  // Context buffer #2
  {
    PRM_CONTEXT_BUFFER_SIGNATURE,           // Signature
    PRM_CONTEXT_BUFFER_INTERFACE_VERSION,   // Version
    0,                                      // Reserved
    HANDLER_TEST_GUID_3,                    // HandlerGuid
    &mTestStaticDataBuffer1                 // StaticDataBuffer (reuse buffer StaticDataBuffer1)
  }
};

PRM_MODULE_CONTEXT_BUFFERS  mTestPrmModuleContextBuffers1 = {
  MODULE_TEST_GUID_1,
  1,
  &mTestPrmContextBuffer1,
  NULL
};

PRM_MODULE_CONTEXT_BUFFERS  mTestPrmModuleContextBuffers2 = {
  MODULE_TEST_GUID_2,
  1,
  &mTestPrmContextBuffer1,
  NULL
};

PRM_MODULE_CONTEXT_BUFFERS  mTestPrmModuleContextBuffers3 = {
  MODULE_TEST_GUID_3,
  2,
  &mTestPrmContextBuffer2[0],
  NULL
};

// *----------------------------------------------------------------------------------*
// * Test Contexts                                                                    *
// *----------------------------------------------------------------------------------*

// * Searches by module GUID *
//                                                   +--------------------------------+--------+----------------+--------------------+--------------------+--------------------+
//                                                   + InstallationStructure          | Handle | GuidSearchType | Guid               | ExpectedModuleGuid | ExpectedStatus     |
//                                                   +--------------------------------+--------+----------------+--------------------+--------------------+--------------------+
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers00 = { &mTestPrmModuleContextBuffers1, NULL, ByModuleGuid, &mModuleTestGuid1, &mModuleTestGuid1, EFI_SUCCESS };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers01 = { &mTestPrmModuleContextBuffers2, NULL, ByModuleGuid, &mModuleTestGuid2, &mModuleTestGuid2, EFI_SUCCESS };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers02 = { &mTestPrmModuleContextBuffers3, NULL, ByModuleGuid, &mModuleTestGuid3, &mModuleTestGuid3, EFI_SUCCESS };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers03 = { &mTestPrmModuleContextBuffers3, NULL, ByModuleGuid, &mNegativeTestGuid, &gZeroGuid, EFI_NOT_FOUND };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers04 = { &mTestPrmModuleContextBuffers1, NULL, ByModuleGuid, &gZeroGuid, &gZeroGuid, EFI_NOT_FOUND };

// * Searches by handler GUID *
//                                                   +--------------------------------+--------+----------------+--------------------+--------------------+--------------------+
//                                                   + InstallationStructure          | Handle | GuidSearchType | Guid               | ExpectedModuleGuid | ExpectedStatus     |
//                                                   +--------------------------------+--------+----------------+--------------------+--------------------+--------------------+
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers05 = { &mTestPrmModuleContextBuffers1, NULL, ByHandlerGuid, &mHandlerTestGuid1, &mModuleTestGuid1, EFI_SUCCESS };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers06 = { &mTestPrmModuleContextBuffers1, NULL, ByHandlerGuid, &gZeroGuid, &gZeroGuid, EFI_NOT_FOUND };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers07 = { &mTestPrmModuleContextBuffers2, NULL, ByHandlerGuid, &mHandlerTestGuid1, &mModuleTestGuid2, EFI_SUCCESS };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers08 = { &mTestPrmModuleContextBuffers2, NULL, ByHandlerGuid, &mNegativeTestGuid, &gZeroGuid, EFI_NOT_FOUND };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers09 = { &mTestPrmModuleContextBuffers3, NULL, ByHandlerGuid, &mHandlerTestGuid1, &gZeroGuid, EFI_NOT_FOUND };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers10 = { &mTestPrmModuleContextBuffers3, NULL, ByHandlerGuid, &mHandlerTestGuid2, &mModuleTestGuid3, EFI_SUCCESS };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers11 = { &mTestPrmModuleContextBuffers3, NULL, ByHandlerGuid, &mHandlerTestGuid3, &mModuleTestGuid3, EFI_SUCCESS };
PRM_CONTEXT_BUFFERS_TEST_CONTEXT  mContextBuffers12 = { &mTestPrmModuleContextBuffers3, NULL, ByHandlerGuid, &gZeroGuid, &gZeroGuid, EFI_NOT_FOUND };

PRM_CONTEXT_BUFFERS_TEST_CONTEXT  *mContextBuffersArray[] = {
  &mContextBuffers00,
  &mContextBuffers01,
  &mContextBuffers02,
  &mContextBuffers03,
  &mContextBuffers04,
  &mContextBuffers05,
  &mContextBuffers06,
  &mContextBuffers07,
  &mContextBuffers08,
  &mContextBuffers09,
  &mContextBuffers10,
  &mContextBuffers11,
  &mContextBuffers12
};

//                                                 +----------------------+----------------------------------+------------------------------------------+--------------------+
//                                                 + HandlerGuid          | ContextBuffers                   | ExpectedContextBuffer                    | ExpectedStatus     |
//                                                 +----------------------+----------------------------------+------------------------------------------+--------------------+
PRM_CONTEXT_BUFFER_TEST_CONTEXT  mContextBuffer00 = { &mHandlerTestGuid1, &mTestPrmModuleContextBuffers1, &mTestPrmContextBuffer1, EFI_SUCCESS };
PRM_CONTEXT_BUFFER_TEST_CONTEXT  mContextBuffer01 = { &mHandlerTestGuid1, &mTestPrmModuleContextBuffers2, &mTestPrmContextBuffer1, EFI_SUCCESS };
PRM_CONTEXT_BUFFER_TEST_CONTEXT  mContextBuffer02 = { &mHandlerTestGuid2, &mTestPrmModuleContextBuffers3, &mTestPrmContextBuffer2[0], EFI_SUCCESS };
PRM_CONTEXT_BUFFER_TEST_CONTEXT  mContextBuffer03 = { &mHandlerTestGuid3, &mTestPrmModuleContextBuffers3, &mTestPrmContextBuffer2[1], EFI_SUCCESS };
PRM_CONTEXT_BUFFER_TEST_CONTEXT  mContextBuffer04 = { &mNegativeTestGuid, &mTestPrmModuleContextBuffers1, NULL, EFI_NOT_FOUND };
PRM_CONTEXT_BUFFER_TEST_CONTEXT  mContextBuffer05 = { &gZeroGuid, &mTestPrmModuleContextBuffers3, NULL, EFI_NOT_FOUND };

PRM_CONTEXT_BUFFER_TEST_CONTEXT  *mContextBufferArray[] = {
  &mContextBuffer00,
  &mContextBuffer01,
  &mContextBuffer02,
  &mContextBuffer03,
  &mContextBuffer04,
  &mContextBuffer05
};

/// === HELPER FUNCTIONS ===========================================================================

// None

/// === TEST CASES =================================================================================

/// ===== BASIC SUITE ==================================================

/**
  Verifies that passing NULL arguments to all library functions fails with EFI_INVALID_PARAMETER.

  @param[in]  Context             [Optional] An optional context parameter.
                                  Not used in this unit test.

  @retval  UNIT_TEST_PASSED                      Unit test case prerequisites are met.
  @retval  UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Test case should be skipped..

**/
UNIT_TEST_STATUS
EFIAPI
NullPointerArgumentsShouldFailGracefully (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_GUID                    Guid;
  PRM_CONTEXT_BUFFER          *ContextBufferPtr;
  PRM_MODULE_CONTEXT_BUFFERS  ModuleContextBuffers;
  PRM_MODULE_CONTEXT_BUFFERS  *ModuleContextBuffersPtr;

  UT_ASSERT_EQUAL (FindContextBufferInModuleBuffers (NULL, NULL, NULL), EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (FindContextBufferInModuleBuffers (NULL, &ModuleContextBuffers, (CONST PRM_CONTEXT_BUFFER **)&ContextBufferPtr), EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (FindContextBufferInModuleBuffers (&Guid, NULL, (CONST PRM_CONTEXT_BUFFER **)&ContextBufferPtr), EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (FindContextBufferInModuleBuffers (&Guid, &ModuleContextBuffers, NULL), EFI_INVALID_PARAMETER);

  UT_ASSERT_EQUAL (GetModuleContextBuffers (ByModuleGuid, NULL, NULL), EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (GetModuleContextBuffers (ByModuleGuid, NULL, (CONST PRM_MODULE_CONTEXT_BUFFERS **)&ModuleContextBuffersPtr), EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (GetModuleContextBuffers (ByModuleGuid, &Guid, NULL), EFI_INVALID_PARAMETER);

  UT_ASSERT_EQUAL (GetContextBuffer (NULL, NULL, NULL), EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (GetContextBuffer (NULL, &ModuleContextBuffers, (CONST PRM_CONTEXT_BUFFER **)&ContextBufferPtr), EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (GetContextBuffer (&Guid, NULL, (CONST PRM_CONTEXT_BUFFER **)&ContextBufferPtr), EFI_NOT_FOUND);
  UT_ASSERT_EQUAL (GetContextBuffer (&Guid, &ModuleContextBuffers, NULL), EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/// ===== FUNCTIONAL CORRECTNESS SUITE ==================================================

/**
  Functional Correctness pre-requisite function.

  Installs a gPrmConfigProtocolGuid protocol instance as specified by the provided
  context in preparation for unit test execution

  @param[in]  Context         [Optional] An optional parameter that enables:
                              A pointer to a PRM_CONTEXT_BUFFERS_TEST_CONTEXT structure with
                              context information for this unit test.

  @retval  UNIT_TEST_PASSED                      Unit test case prerequisites
                                                 are met.
  @retval  UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Test case should be skipped.

**/
STATIC
UNIT_TEST_STATUS
EFIAPI
InitializeFunctionalCorrectness (
  IN  UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                        Status;
  PRM_CONFIG_PROTOCOL               *PrmConfigProtocol;
  PRM_MODULE_CONTEXT_BUFFERS        *ModuleContextBuffers;
  PRM_CONTEXT_BUFFERS_TEST_CONTEXT  *TestContext;

  UT_ASSERT_NOT_NULL (Context);
  TestContext          = (PRM_CONTEXT_BUFFERS_TEST_CONTEXT *)Context;
  ModuleContextBuffers = TestContext->InstallationStructure;

  PrmConfigProtocol = AllocateZeroPool (sizeof (*PrmConfigProtocol));
  if (PrmConfigProtocol == NULL) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  CopyGuid (&PrmConfigProtocol->ModuleContextBuffers.ModuleGuid, &ModuleContextBuffers->ModuleGuid);
  PrmConfigProtocol->ModuleContextBuffers.BufferCount = ModuleContextBuffers->BufferCount;
  PrmConfigProtocol->ModuleContextBuffers.Buffer      = ModuleContextBuffers->Buffer;

  Status =  gBS->InstallProtocolInterface (
                   &TestContext->Handle,
                   &gPrmConfigProtocolGuid,
                   EFI_NATIVE_INTERFACE,
                   (VOID *)PrmConfigProtocol
                   );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Functional Correctness cleanup function.

  Uninstalls the gPrmConfigProtocolGuid protocol instance as specified by the
  provided context. This is used to clean up the mocked protocol database after
  unit test execution.

  @param[in]  Context       [Optional] An optional parameter that enables:
                            A pointer to a PRM_CONTEXT_BUFFERS_TEST_CONTEXT structure with
                            context information for this unit test.

  @retval  UNIT_TEST_PASSED                Test case cleanup succeeded.
  @retval  UNIT_TEST_ERROR_CLEANUP_FAILED  Test case cleanup failed.

**/
STATIC
VOID
EFIAPI
DeInitializeFunctionalCorrectness (
  IN  UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                        Status;
  PRM_CONFIG_PROTOCOL               *PrmConfigProtocol;
  PRM_CONTEXT_BUFFERS_TEST_CONTEXT  *TestContext;

  TestContext = (PRM_CONTEXT_BUFFERS_TEST_CONTEXT *)Context;

  Status = gBS->HandleProtocol (
                  TestContext->Handle,
                  &gPrmConfigProtocolGuid,
                  (VOID **)&PrmConfigProtocol
                  );

  if (!EFI_ERROR (Status)) {
    Status =  gBS->UninstallProtocolInterface (
                     TestContext->Handle,
                     &gPrmConfigProtocolGuid,
                     PrmConfigProtocol
                     );
    if (!EFI_ERROR (Status)) {
      FreePool (PrmConfigProtocol);
    }
  }
}

/**
  Verifies that the correct PRM_MODULE_CONTEXT_BUFFERS structure instance is found
  for a given PRM module or PRM handler GUID.

  @param[in]  Context       [Optional] An optional context parameter.
                            A pointer to a PRM_CONTEXT_BUFFERS_TEST_CONTEXT structure with
                            context information for this unit test.

  @retval  UNIT_TEST_PASSED                      Unit test case prerequisites are met.
  @retval  UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Test case should be skipped..

**/
UNIT_TEST_STATUS
EFIAPI
VerifyGetModuleContextBuffers (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                        Status;
  PRM_MODULE_CONTEXT_BUFFERS        *ContextBuffers;
  PRM_CONTEXT_BUFFERS_TEST_CONTEXT  *TestContext;

  ContextBuffers = NULL;
  TestContext    = (PRM_CONTEXT_BUFFERS_TEST_CONTEXT *)Context;

  Status = GetModuleContextBuffers (TestContext->GuidSearchType, TestContext->Guid, (CONST PRM_MODULE_CONTEXT_BUFFERS **)&ContextBuffers);
  UT_ASSERT_STATUS_EQUAL (Status, TestContext->ExpectedStatus);

  if (!EFI_ERROR (TestContext->ExpectedStatus)) {
    UT_ASSERT_TRUE (CompareGuid (TestContext->ExpectedModuleGuid, &ContextBuffers->ModuleGuid));
    UT_LOG_INFO (
      "%a: Searching by %a GUID ({%g}) returned ContextBuffers at 0x%x\n",
      __FUNCTION__,
      ((TestContext->GuidSearchType == ByModuleGuid) ? "module" : "handler"),
      TestContext->Guid,
      (UINTN)ContextBuffers
      );
  }

  return UNIT_TEST_PASSED;
}

/**
  Verifies that the expected PRM_CONTEXT_BUFFER instance is found for the given HandlerGuid
  in the provided PRM_MODULE_CONTEXT_BUFFERS structure.

  @param[in]  Context       [Optional] An optional context parameter.
                            A pointer to a PRM_CONTEXT_BUFFERS_TEST_CONTEXT structure with
                            context information for this unit test.

  @retval  UNIT_TEST_PASSED                      Unit test case prerequisites are met.
  @retval  UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Test case should be skipped..

**/
UNIT_TEST_STATUS
EFIAPI
VerifyFindContextBufferInModuleBuffers (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                       Status;
  PRM_CONTEXT_BUFFER               *FoundContextBuffer;
  PRM_CONTEXT_BUFFER_TEST_CONTEXT  *TestContext;

  FoundContextBuffer = NULL;
  TestContext        = (PRM_CONTEXT_BUFFER_TEST_CONTEXT *)Context;

  Status = FindContextBufferInModuleBuffers (TestContext->HandlerGuid, TestContext->ContextBuffers, (CONST PRM_CONTEXT_BUFFER **)&FoundContextBuffer);
  UT_ASSERT_STATUS_EQUAL (Status, TestContext->ExpectedStatus);

  if (!EFI_ERROR (TestContext->ExpectedStatus)) {
    UT_ASSERT_NOT_NULL (FoundContextBuffer);
    UT_ASSERT_TRUE (FoundContextBuffer == TestContext->ExpectedContextBuffer);
  }

  return UNIT_TEST_PASSED;
}

/**
  Verifies that the expected PRM_CONTEXT_BUFFER instance is found for the given HandlerGuid.

  This function checks both the case when a PRM_MODULE_CONTEXT_BUFFERS structure pointer is provided and
  not provided.

  NOTES:
  - In the future, this function should mock the internal calls to other library functions but the direct
    calls are left in place for now.
  - The PrmModuleContextBuffers being NULL is not actually tested at the moment. In the future, that case
    should also be added.

  @param[in]  Context       [Optional] An optional context parameter.
                            A pointer to a PRM_CONTEXT_BUFFERS_TEST_CONTEXT structure with
                            context information for this unit test.

  @retval  UNIT_TEST_PASSED                      Unit test case prerequisites are met.
  @retval  UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Test case should be skipped..

**/
UNIT_TEST_STATUS
EFIAPI
VerifyGetContextBuffer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                       Status;
  PRM_CONTEXT_BUFFER               *FoundContextBuffer;
  PRM_CONTEXT_BUFFER_TEST_CONTEXT  *TestContext;

  FoundContextBuffer = NULL;
  TestContext        = (PRM_CONTEXT_BUFFER_TEST_CONTEXT *)Context;

  Status = GetContextBuffer (TestContext->HandlerGuid, TestContext->ContextBuffers, (CONST PRM_CONTEXT_BUFFER **)&FoundContextBuffer);

  UT_ASSERT_STATUS_EQUAL (Status, TestContext->ExpectedStatus);

  if (!EFI_ERROR (TestContext->ExpectedStatus)) {
    UT_ASSERT_NOT_NULL (FoundContextBuffer);
    UT_ASSERT_TRUE (FoundContextBuffer == TestContext->ExpectedContextBuffer);
  }

  return UNIT_TEST_PASSED;
}

/// === TEST ENGINE ================================================================================

/**
  Entry point for the PRM Context Buffer Library unit tests.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
int
main (
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      BasicTests;
  UNIT_TEST_SUITE_HANDLE      FunctionalCorrectnessTests;
  CHAR8                       TestCaseClassNameString[256];
  CHAR8                       TestCaseDescriptionString[256];

  Framework = NULL;

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
  Status = CreateUnitTestSuite (&BasicTests, Framework, "Basic Context Buffer Tests", "PrmContextBufferLib.Basic", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for PrmContextBufferLib.Basic\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (
    BasicTests,
    "",
    "PrmContextBufferLib.Basic.NullPointerGracefulFailure",
    NullPointerArgumentsShouldFailGracefully,
    NULL,
    NULL,
    NULL
    );

  Status = CreateUnitTestSuite (&FunctionalCorrectnessTests, Framework, "Functional Correctness Tests", "PrmContextBufferLib.Functional", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for PrmContextBufferLib.Functional\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Add Functional Correctness unit tests
  //
  for (Index = 0; Index < ARRAY_SIZE (mContextBuffersArray); Index++) {
    ZeroMem (&TestCaseClassNameString[0], ARRAY_SIZE (TestCaseClassNameString));
    ZeroMem (&TestCaseDescriptionString[0], ARRAY_SIZE (TestCaseDescriptionString));

    AsciiSPrint (
      &TestCaseClassNameString[0],
      ARRAY_SIZE (TestCaseClassNameString),
      "PrmContextBufferLib.Functional.VerifyGetModuleContextBuffers%d",
      Index + 1
      );
    AsciiSPrint (
      &TestCaseDescriptionString[0],
      ARRAY_SIZE (TestCaseDescriptionString),
      "Verify Get PRM Module Context Buffers Structure by %a GUID %d\n",
      ((mContextBuffersArray[Index]->GuidSearchType == ByModuleGuid) ? "module" : "handler"),
      Index + 1
      );

    AddTestCase (
      FunctionalCorrectnessTests,
      &TestCaseDescriptionString[0],
      &TestCaseClassNameString[0],
      VerifyGetModuleContextBuffers,
      InitializeFunctionalCorrectness,
      DeInitializeFunctionalCorrectness,
      mContextBuffersArray[Index]
      );
  }

  for (Index = 0; Index < ARRAY_SIZE (mContextBufferArray); Index++) {
    ZeroMem (&TestCaseClassNameString[0], ARRAY_SIZE (TestCaseClassNameString));
    ZeroMem (&TestCaseDescriptionString[0], ARRAY_SIZE (TestCaseDescriptionString));

    AsciiSPrint (
      &TestCaseClassNameString[0],
      ARRAY_SIZE (TestCaseClassNameString),
      "PrmContextBufferLib.Functional.VerifyFindContextBufferInModuleBuffers%d",
      Index + 1
      );
    AsciiSPrint (
      &TestCaseDescriptionString[0],
      ARRAY_SIZE (TestCaseDescriptionString),
      "Verify Find PRM Context Buffer by Handler GUID %d\n",
      Index + 1
      );

    AddTestCase (
      FunctionalCorrectnessTests,
      &TestCaseDescriptionString[0],
      &TestCaseClassNameString[0],
      VerifyFindContextBufferInModuleBuffers,
      NULL,
      NULL,
      mContextBufferArray[Index]
      );
  }

  for (Index = 0; Index < ARRAY_SIZE (mContextBufferArray); Index++) {
    ZeroMem (&TestCaseClassNameString[0], ARRAY_SIZE (TestCaseClassNameString));
    ZeroMem (&TestCaseDescriptionString[0], ARRAY_SIZE (TestCaseDescriptionString));

    AsciiSPrint (
      &TestCaseClassNameString[0],
      ARRAY_SIZE (TestCaseClassNameString),
      "PrmContextBufferLib.Functional.VerifyGetContextBuffer%d",
      Index + 1
      );
    AsciiSPrint (
      &TestCaseDescriptionString[0],
      ARRAY_SIZE (TestCaseDescriptionString),
      "Verify Get PRM Context Buffer by Handler GUID %d\n",
      Index + 1
      );

    AddTestCase (
      FunctionalCorrectnessTests,
      &TestCaseDescriptionString[0],
      &TestCaseClassNameString[0],
      VerifyGetContextBuffer,
      NULL,
      NULL,
      mContextBufferArray[Index]
      );
  }

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
