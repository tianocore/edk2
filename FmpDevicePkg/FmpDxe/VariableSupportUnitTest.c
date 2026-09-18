/** @file
  Unit tests for FMP variable update checkpoints.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UnitTestLib.h>

#include "FmpDxe.h"
#include "VariableSupport.h"

#define UNIT_TEST_APP_NAME     "FmpDxe Variable Support Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

STATIC FMP_CONTROLLER_STATE  mStoredState;
STATIC FIRMWARE_MANAGEMENT_PRIVATE_DATA  mPrivate;
STATIC EFI_STATUS            mGetStatus;
STATIC EFI_STATUS            mSetStatus;
STATIC UINTN                 mStoredSize;
STATIC UINTN                 mSetCalls;

CHAR16  *mImageIdName = L"FmpDxeVariableUnitTest";

STATIC
EFI_STATUS
EFIAPI
MockGetVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data OPTIONAL
  )
{
  if (EFI_ERROR (mGetStatus)) {
    return mGetStatus;
  }

  if ((Data == NULL) || (*DataSize < mStoredSize)) {
    *DataSize = mStoredSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (Data, &mStoredState, mStoredSize);
  *DataSize = mStoredSize;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MockSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  mSetCalls++;
  if (EFI_ERROR (mSetStatus)) {
    return mSetStatus;
  }

  if ((Data == NULL) || (DataSize != sizeof (mStoredState))) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&mStoredState, Data, sizeof (mStoredState));
  return EFI_SUCCESS;
}

EFI_RUNTIME_SERVICES  MockRuntime = {
  {
    EFI_RUNTIME_SERVICES_SIGNATURE,
    EFI_RUNTIME_SERVICES_REVISION,
    sizeof (EFI_RUNTIME_SERVICES),
    0,
    0
  },
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  MockGetVariable,
  NULL,
  MockSetVariable,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

STATIC
VOID
ResetStore (
  VOID
  )
{
  ZeroMem (&mStoredState, sizeof (mStoredState));
  ZeroMem (&mPrivate, sizeof (mPrivate));
  mPrivate.FmpStateVariableName            = L"FmpState";
  mStoredState.LastAttemptStatusValid  = TRUE;
  mStoredState.LastAttemptVersionValid = TRUE;
  mStoredState.LastAttemptStatus       = LAST_ATTEMPT_STATUS_SUCCESS;
  mStoredState.LastAttemptVersion      = 7;
  mGetStatus                           = EFI_SUCCESS;
  mSetStatus                           = EFI_SUCCESS;
  mStoredSize                          = sizeof (mStoredState);
  mSetCalls                            = 0;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
CheckpointPersistsVersionAndFailure (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS            Status;
  FMP_CONTROLLER_STATE  *State;

  ResetStore ();
  Status = SetUpdateInProgressInVariable (&mPrivate, 9);

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (mSetCalls, 1);
  UT_ASSERT_EQUAL (mStoredState.LastAttemptVersion, 9);
  UT_ASSERT_EQUAL (mStoredState.LastAttemptStatus, LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL);

  ZeroMem (&mPrivate, sizeof (mPrivate));
  mPrivate.FmpStateVariableName = L"FmpState";
  State                         = GetFmpControllerState (&mPrivate);
  UT_ASSERT_NOT_NULL (State);
  UT_ASSERT_EQUAL (GetLastAttemptVersionFromFmpControllerState (State), 9);
  UT_ASSERT_EQUAL (GetLastAttemptStatusFromFmpControllerState (State), LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL);
  FreePool (State);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
FullStoreBlocksUpdate (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  ResetStore ();
  mSetStatus = EFI_OUT_OF_RESOURCES;
  Status     = SetUpdateInProgressInVariable (&mPrivate, 9);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_OUT_OF_RESOURCES);
  UT_ASSERT_EQUAL (mStoredState.LastAttemptVersion, 7);
  UT_ASSERT_EQUAL (mStoredState.LastAttemptStatus, LAST_ATTEMPT_STATUS_SUCCESS);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
ReadFailureBlocksUpdate (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  ResetStore ();
  mGetStatus = EFI_DEVICE_ERROR;
  Status     = SetUpdateInProgressInVariable (&mPrivate, 9);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_DEVICE_ERROR);
  UT_ASSERT_EQUAL (mSetCalls, 0);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
MissingStateBlocksUpdate (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  ResetStore ();
  mGetStatus = EFI_NOT_FOUND;
  Status     = SetUpdateInProgressInVariable (&mPrivate, 9);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);
  UT_ASSERT_EQUAL (mSetCalls, 0);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
WriteFailureBlocksUpdate (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  ResetStore ();
  mSetStatus = EFI_DEVICE_ERROR;
  Status     = SetUpdateInProgressInVariable (&mPrivate, 9);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_DEVICE_ERROR);
  UT_ASSERT_EQUAL (mStoredState.LastAttemptVersion, 7);
  UT_ASSERT_EQUAL (mStoredState.LastAttemptStatus, LAST_ATTEMPT_STATUS_SUCCESS);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
MalformedStateBlocksUpdate (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  ResetStore ();
  mStoredSize = sizeof (mStoredState) - 1;
  Status      = SetUpdateInProgressInVariable (&mPrivate, 9);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_COMPROMISED_DATA);
  UT_ASSERT_EQUAL (mSetCalls, 0);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
DurableCheckpointNeedsNoRewrite (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  ResetStore ();
  mStoredState.LastAttemptVersion = 9;
  mStoredState.LastAttemptStatus  = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
  mSetStatus                      = EFI_OUT_OF_RESOURCES;
  Status                          = SetUpdateInProgressInVariable (&mPrivate, 9);

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (mSetCalls, 0);
  return UNIT_TEST_PASSED;
}

STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      CheckpointTests;

  Framework = NULL;
  Status    = InitUnitTestFramework (
                &Framework,
                UNIT_TEST_APP_NAME,
                gEfiCallerBaseName,
                UNIT_TEST_APP_VERSION
                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CreateUnitTestSuite (
             &CheckpointTests,
             Framework,
             "FMP variable checkpoints",
             "FmpDxe.VariableSupport",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return Status;
  }

  AddTestCase (CheckpointTests, "Checkpoint persists version and failure", "Persist", CheckpointPersistsVersionAndFailure, NULL, NULL, NULL);
  AddTestCase (CheckpointTests, "Full store blocks update", "FullStore", FullStoreBlocksUpdate, NULL, NULL, NULL);
  AddTestCase (CheckpointTests, "Read failure blocks update", "ReadFailure", ReadFailureBlocksUpdate, NULL, NULL, NULL);
  AddTestCase (CheckpointTests, "Missing state blocks update", "MissingState", MissingStateBlocksUpdate, NULL, NULL, NULL);
  AddTestCase (CheckpointTests, "Write failure blocks update", "WriteFailure", WriteFailureBlocksUpdate, NULL, NULL, NULL);
  AddTestCase (CheckpointTests, "Malformed state blocks update", "MalformedState", MalformedStateBlocksUpdate, NULL, NULL, NULL);
  AddTestCase (CheckpointTests, "Durable checkpoint needs no rewrite", "NoRewrite", DurableCheckpointNeedsNoRewrite, NULL, NULL, NULL);

  Status = RunAllTestSuites (Framework);
  FreeUnitTestFramework (Framework);
  return Status;
}

#define VariableSupportUnitTestMain  main

INT32
VariableSupportUnitTestMain (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  return EFI_ERROR (UnitTestingEntry ()) ? 1 : 0;
}
