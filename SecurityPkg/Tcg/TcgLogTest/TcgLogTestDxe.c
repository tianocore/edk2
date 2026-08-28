/** @file
  DXE driver that validates TCG2 event log dynamic scaling before ReadyToBoot.

  On entry the driver checks the NV variable TcgLogTestEnable. If not set,
  the driver installs the protocol (with SetEnabled only) and returns without
  running any tests. When the variable is set, the driver runs the
  pre-ReadyToBoot scaling test, logs results into an internal buffer, clears
  the variable, and installs the protocol so TcgLogTestApp can retrieve logs.

  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/Tcg2Protocol.h>
#include <IndustryStandard/UefiTcgPlatform.h>

#include "TcgLogTest.h"
#include "TcgLogTestCommon.h"

#define TCG_LOG_TEST_MAX_LOG_SIZE  4096

STATIC CHAR8              mLogBuffer[TCG_LOG_TEST_MAX_LOG_SIZE];
STATIC UINTN              mLogOffset        = 0;
STATIC EFI_TCG2_PROTOCOL  *mTcg2Protocol    = NULL;
STATIC EFI_HANDLE         mTcgLogTestHandle = NULL;

/**
  Append a formatted message to the internal log buffer.

  @param[in] Format  Printf-style format string.
  @param[in] ...     Variable arguments for the format string.
**/
STATIC
VOID
EFIAPI
LogAppend (
  IN CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST  Args;
  UINTN    Remaining;
  UINTN    Written;

  if (mLogOffset >= TCG_LOG_TEST_MAX_LOG_SIZE - 1) {
    return;
  }

  Remaining = TCG_LOG_TEST_MAX_LOG_SIZE - mLogOffset - 1;

  VA_START (Args, Format);
  Written = AsciiVSPrint (mLogBuffer + mLogOffset, Remaining, Format, Args);
  VA_END (Args);

  mLogOffset += Written;
}

/**
  Protocol function: retrieve the pre-ReadyToBoot test log.

  @param[in]  This       Protocol instance.
  @param[out] LogBuffer  On success, pointer to the internal log buffer.
  @param[out] LogSize    On success, size of the log data in bytes.

  @retval EFI_SUCCESS            Log retrieved.
  @retval EFI_INVALID_PARAMETER  LogBuffer or LogSize is NULL.
  @retval EFI_NOT_STARTED        The test has not run this boot.
**/
STATIC
EFI_STATUS
EFIAPI
TcgLogTestGetLog (
  IN  TCG_LOG_TEST_PROTOCOL  *This,
  OUT CHAR8                  **LogBuffer,
  OUT UINTN                  *LogSize
  )
{
  if ((LogBuffer == NULL) || (LogSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mLogOffset == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Log is empty.\n", __func__));
    return EFI_NOT_STARTED;
  }

  *LogBuffer = mLogBuffer;
  *LogSize   = mLogOffset + 1;
  return EFI_SUCCESS;
}

/**
  Protocol function: enable or disable the DXE test for the next boot.

  @param[in] This    Protocol instance.
  @param[in] Enable  TRUE to enable, FALSE to disable.

  @retval EFI_SUCCESS  Variable updated successfully.
  @retval Other        SetVariable failure.
**/
STATIC
EFI_STATUS
EFIAPI
TcgLogTestEnable (
  IN TCG_LOG_TEST_PROTOCOL  *This,
  IN BOOLEAN                Enable
  )
{
  BOOLEAN  Dummy;

  if (Enable) {
    Dummy = TRUE;
    // Create the variable to signal the test should run.
    return gRT->SetVariable (
                  TCG_LOG_TEST_ENABLE_VARIABLE_NAME,
                  &gTcgLogTestProtocolGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (Dummy),
                  &Dummy
                  );
  } else {
    // Delete the variable entirely.
    return gRT->SetVariable (
                  TCG_LOG_TEST_ENABLE_VARIABLE_NAME,
                  &gTcgLogTestProtocolGuid,
                  0,
                  0,
                  NULL
                  );
  }
}

STATIC TCG_LOG_TEST_PROTOCOL  mTcgLogTestProtocol = {
  TcgLogTestGetLog,
  TcgLogTestEnable
};

/**
  Check whether the NV enable variable exists.

  @retval TRUE   Variable exists (test is enabled).
  @retval FALSE  Variable absent.
**/
STATIC
BOOLEAN
IsTestEnabled (
  VOID
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Value;
  UINTN       Size;

  Size   = sizeof (Value);
  Status = gRT->GetVariable (
                  TCG_LOG_TEST_ENABLE_VARIABLE_NAME,
                  &gTcgLogTestProtocolGuid,
                  NULL,
                  &Size,
                  &Value
                  );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Exercise dynamic scaling before ReadyToBoot. Verifies that scaling occurs
  (Tcg2Dxe signals gTcg2EventLogScaledGuid).
**/
STATIC
VOID
TestPreReadyToBootScaling (
  VOID
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Scaled;

  Status = TcgLogTestLogEventsUntilScaled (mTcg2Protocol, &Scaled);
  if (EFI_ERROR (Status) || !Scaled) {
    DEBUG ((DEBUG_ERROR, "%a: LogEventsUntilScaled failed - %r, Scaled=%d\n", __func__, Status, Scaled));
    LogAppend ("FAIL: Pre-ReadyToBoot: LogEventsUntilScaled - %r\n", Status);
    return;
  }

  DEBUG ((DEBUG_INFO, "%a: Pre-ReadyToBoot scaling succeeded\n", __func__));
  LogAppend ("PASS: Pre-ReadyToBoot scaling succeeded\n");
}

/**
  Entry point for TcgLogTestDxe.

  @param[in] ImageHandle  Image handle.
  @param[in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS  Driver initialized and protocol installed.
**/
EFI_STATUS
EFIAPI
TcgLogTestDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  // Install the protocol so the TcgLogTestApp can enable the test and/or acquire the logs.
  Status = gBS->InstallProtocolInterface (
                  &mTcgLogTestHandle,
                  &gTcgLogTestProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mTcgLogTestProtocol
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InstallProtocolInterface failed: %r\n", __func__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: Protocol installed, checking enable state\n", __func__));

  // Only run the test if it has been enabled.
  if (!IsTestEnabled ()) {
    DEBUG ((DEBUG_INFO, "%a: Test not enabled, skipping\n", __func__));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "%a: Test enabled, running pre-ReadyToBoot test\n", __func__));

  // Clear the enable variable so we don't re-run on the next boot.
  Status = TcgLogTestEnable (&mTcgLogTestProtocol, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to clear enable variable: %r\n", __func__, Status));
  }

  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **)&mTcg2Protocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate TCG2 protocol: %r\n", __func__, Status));
    LogAppend ("FAIL: TCG2 protocol not found - %r\n", Status);
    return EFI_SUCCESS;
  }

  // Run the pre-ReadyToBoot scaling test.
  TestPreReadyToBootScaling ();

  return EFI_SUCCESS;
}
