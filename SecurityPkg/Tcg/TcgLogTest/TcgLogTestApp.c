/** @file
  UEFI Shell UnitTest application that validates TCG2 event log dynamic
  scaling after ReadyToBoot.

  This application locates the TcgLogTestProtocol produced by TcgLogTestDxe to
  retrieve pre-ReadyToBoot test logs, then exercises post-ReadyToBoot scaling.

  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UnitTestLib.h>
#include <Protocol/Tcg2Protocol.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Tpm2Acpi.h>

#include "TcgLogTest.h"
#include "TcgLogTestCommon.h"

#define UNIT_TEST_NAME     "TCG Log Scaling Test"
#define UNIT_TEST_VERSION  "1.0"

// Must match TCG_EVENT_LOG_MAX_SCALE_COUNT in Tcg2Dxe.c.
#define TCG_EVENT_LOG_MAX_SCALE_COUNT  4

// Must match TCG_LOG_TRUNCATION_EVENT_STRING in Tcg2Dxe.c.
#define TCG_LOG_TRUNCATION_EVENT_STRING  "TCG Event Log Truncated"

// Number of extra events to log
#define MAX_NUM_EXTRA_EVENTS  8

STATIC EFI_TCG2_PROTOCOL      *mTcg2Protocol       = NULL;
STATIC TCG_LOG_TEST_PROTOCOL  *mTcgLogTestProtocol = NULL;

/**
  Locate the fixed-size ACPI TCG event log region published by Tcg2Dxe via
  the TPM2 ACPI table LAML/LASA fields.

  The fields only exist for revision >= 4; the Header.Length must include
  them or they are considered absent.

  @param[out] Lasa  On success, physical address of the ACPI event log region.
  @param[out] Laml  On success, length in bytes of the ACPI event log region.

  @retval EFI_SUCCESS            ACPI event log located.
  @retval EFI_INVALID_PARAMETER  NULL argument.
  @retval EFI_NOT_FOUND          TPM2 ACPI table not present, revision < 4,
                                 header length excludes LAML/LASA, or the
                                 fields are zero.
**/
STATIC
EFI_STATUS
GetAcpiEventLog (
  OUT EFI_PHYSICAL_ADDRESS  *Lasa,
  OUT UINT32                *Laml
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  *Header;
  EFI_TPM2_ACPI_TABLE_V4       *TableV4;
  EFI_TPM2_ACPI_TABLE_V5       *TableV5;
  UINT32                       FoundLaml;
  UINT64                       FoundLasa;

  if ((Lasa == NULL) || (Laml == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Header = (EFI_ACPI_DESCRIPTION_HEADER *)EfiLocateFirstAcpiTable (
                                            EFI_ACPI_5_0_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE
                                            );
  if (Header == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: TPM2 ACPI table not found\n", __func__));
    return EFI_NOT_FOUND;
  }

  // LAML/LASA are optional and only exist for revision 4 and above. Confirm
  // the table's Header.Length is large enough to include those trailing fields
  // before dereferencing them.
  switch (Header->Revision) {
    case EFI_TPM2_ACPI_TABLE_REVISION_4:
      if (Header->Length < sizeof (EFI_TPM2_ACPI_TABLE_V4)) {
        DEBUG ((DEBUG_ERROR, "%a: TPM2 ACPI rev4 length %u < %u\n", __func__, Header->Length, (UINT32)sizeof (EFI_TPM2_ACPI_TABLE_V4)));
        return EFI_NOT_FOUND;
      }

      TableV4   = (EFI_TPM2_ACPI_TABLE_V4 *)Header;
      FoundLaml = TableV4->Laml;
      FoundLasa = TableV4->Lasa;
      break;

    case EFI_TPM2_ACPI_TABLE_REVISION_5:
      if (Header->Length < sizeof (EFI_TPM2_ACPI_TABLE_V5)) {
        DEBUG ((DEBUG_ERROR, "%a: TPM2 ACPI rev5 length %u < %u\n", __func__, Header->Length, (UINT32)sizeof (EFI_TPM2_ACPI_TABLE_V5)));
        return EFI_NOT_FOUND;
      }

      TableV5   = (EFI_TPM2_ACPI_TABLE_V5 *)Header;
      FoundLaml = TableV5->Laml;
      FoundLasa = TableV5->Lasa;
      break;

    default:
      DEBUG ((DEBUG_ERROR, "%a: TPM2 ACPI revision %u does not carry LAML/LASA\n", __func__, Header->Revision));
      return EFI_NOT_FOUND;
  }

  if ((FoundLaml == 0) || (FoundLasa == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: TPM2 ACPI LAML/LASA are zero\n", __func__));
    return EFI_NOT_FOUND;
  }

  *Lasa = (EFI_PHYSICAL_ADDRESS)FoundLasa;
  *Laml = FoundLaml;

  return EFI_SUCCESS;
}

/**
  Test that the DXE driver ran and its pre-ReadyToBoot log contains PASS.

  This runs on the second boot after TestPostReadyToBootScaling enabled the
  DXE driver and rebooted.  The DXE driver ran before ReadyToBoot on this
  boot, so results are available via the protocol.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED              Log contains PASS and no FAIL.
  @retval UNIT_TEST_ERROR_TEST_FAILED   Assertion failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestPreReadyToBootScaling (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  CHAR8       *LogBuffer;
  UINTN       LogSize;

  // The prerequisite is skipped on resume from a reboot, so locate the
  // protocol here if it was not already set.
  if (mTcgLogTestProtocol == NULL) {
    Status = gBS->LocateProtocol (&gTcgLogTestProtocolGuid, NULL, (VOID **)&mTcgLogTestProtocol);
    UT_ASSERT_NOT_EFI_ERROR (Status);
  }

  Status = mTcgLogTestProtocol->GetLog (mTcgLogTestProtocol, &LogBuffer, &LogSize);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("GetLog failed: %r\n", Status);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  // Dump the DXE driver's log for visibility.
  UT_LOG_INFO ("TcgLogTestDxe Log (%u bytes):\n%a\n", LogSize, LogBuffer);

  // Verify the log contains "PASS".
  UT_ASSERT_NOT_NULL (AsciiStrStr (LogBuffer, "PASS"));

  // Verify the log does not contain "FAIL".
  UT_ASSERT_TRUE (AsciiStrStr (LogBuffer, "FAIL") == NULL);

  return UNIT_TEST_PASSED;
}

/**
  Test post-ReadyToBoot scaling: log events until the log scales.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED              Scaling verified.
  @retval UNIT_TEST_ERROR_TEST_FAILED   Assertion failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestPostReadyToBootScaling (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Scaled;

  Status = TcgLogTestLogEventsUntilScaled (mTcg2Protocol, &Scaled);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_TRUE (Scaled);

  UT_LOG_INFO ("Post-ReadyToBoot scaling succeeded\n");

  return UNIT_TEST_PASSED;
}

/**
  Test that the TCG event log can only be dynamically scaled up to
  TCG_EVENT_LOG_MAX_SCALE_COUNT times, after which the log is marked as
  truncated.

  TestPostReadyToBootScaling already consumed one scale in this boot, so this
  test exhausts the remaining (TCG_EVENT_LOG_MAX_SCALE_COUNT - 1) scales and
  then verifies the next attempt fails and the log is marked truncated.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED              Scaling limit and truncation verified.
  @retval UNIT_TEST_ERROR_TEST_FAILED   Assertion failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestScaleLimitTruncatesLog (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS            Status;
  BOOLEAN               Scaled;
  BOOLEAN               Truncated;
  UINTN                 ScaleCount;
  EFI_PHYSICAL_ADDRESS  LogBase;
  EFI_PHYSICAL_ADDRESS  LastEntry;

  // TestPostReadyToBootScaling scaled once, scale the remaining amount.
  for (ScaleCount = 1; ScaleCount < TCG_EVENT_LOG_MAX_SCALE_COUNT; ScaleCount++) {
    Scaled = FALSE;
    Status = TcgLogTestLogEventsUntilScaled (mTcg2Protocol, &Scaled);
    UT_LOG_INFO ("Scale %u/%u: status=%r scaled=%d\n", ScaleCount + 1, TCG_EVENT_LOG_MAX_SCALE_COUNT, Status, Scaled);
    UT_ASSERT_NOT_EFI_ERROR (Status);
    UT_ASSERT_TRUE (Scaled);
  }

  // Attempt to scale once more which should fail.
  Scaled = FALSE;
  Status = TcgLogTestLogEventsUntilScaled (mTcg2Protocol, &Scaled);
  UT_LOG_INFO ("Scale beyond limit: status=%r scaled=%d\n", Status, Scaled);
  UT_ASSERT_TRUE (EFI_ERROR (Status));
  UT_ASSERT_FALSE (Scaled);

  // Verify GetEventLog reports the log as truncated.
  Status = mTcg2Protocol->GetEventLog (
                            mTcg2Protocol,
                            EFI_TCG2_EVENT_LOG_FORMAT_TCG_2,
                            &LogBase,
                            &LastEntry,
                            &Truncated
                            );

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_TRUE (Truncated);

  UT_LOG_INFO ("Test scaling limit succeeded\n");

  return UNIT_TEST_PASSED;
}

/**
  Test that an EV_NO_ACTION truncation marker is appended to the
  FinalEventLog when it becomes truncated.

  Activates FinalEventLog via GetEventLog, then logs events until
  HashLogExtendEvent reports an error. Because the FinalEventLog does not scale
  it is possible to truncate at which point the truncation marker is appended
  as the final entry. The test then walks the table and verifies the last entry
  is an EV_NO_ACTION carrying the truncation marker string.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED              Truncation marker located and verified.
  @retval UNIT_TEST_ERROR_TEST_FAILED   Assertion failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestFinalEventLogTruncationMarker (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                   Status;
  EFI_PHYSICAL_ADDRESS         LogBase;
  EFI_PHYSICAL_ADDRESS         LastEntry;
  BOOLEAN                      Truncated;
  BOOLEAN                      Scaled;
  EFI_TCG2_FINAL_EVENTS_TABLE  *FinalTable;
  UINT8                        *CurrentEvent;
  UINT32                       EventType;
  UINT32                       EventSize;
  UINT8                        *EventData;
  UINT64                       Index;

  // Activate FinalEventLog logging.
  Status = mTcg2Protocol->GetEventLog (
                            mTcg2Protocol,
                            EFI_TCG2_EVENT_LOG_FORMAT_TCG_2,
                            &LogBase,
                            &LastEntry,
                            &Truncated
                            );

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_FALSE (Truncated);

  // Log events until HashLogExtendEvent reports an error. When the
  // FinalEventLog becomes truncated, HashLogExtendEvent will return
  // EFI_VOLUME_FULL.
  Scaled = FALSE;
  do {
    Status = TcgLogTestLogEventsUntilScaled (mTcg2Protocol, &Scaled);
  } while (!EFI_ERROR (Status));

  UT_LOG_INFO ("Log events stopped: status=%r", Status);
  UT_ASSERT_TRUE (Status == EFI_VOLUME_FULL);

  // Walk the FinalEventsTable and verify the last entry is the truncation marker.
  FinalTable = NULL;
  Status     = EfiGetSystemConfigurationTable (&gEfiTcg2FinalEventsTableGuid, (VOID **)&FinalTable);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (FinalTable);

  UT_LOG_INFO ("FinalEventsTable: Version=%lu NumberOfEvents=%lu\n", FinalTable->Version, FinalTable->NumberOfEvents);
  UT_ASSERT_TRUE (FinalTable->NumberOfEvents > 0);

  CurrentEvent = (UINT8 *)(FinalTable + 1);
  EventType    = 0;
  EventSize    = 0;
  EventData    = NULL;

  for (Index = 0; Index < FinalTable->NumberOfEvents; Index++) {
    if (!TcgLogTestAdvanceEvent (
           &CurrentEvent,
           (UINT8 *)(UINTN)MAX_ADDRESS,
           NULL,
           &EventType,
           &EventSize,
           &EventData
           ))
    {
      UT_LOG_ERROR ("AdvanceEvent failed at index %lu\n", Index);
      return UNIT_TEST_ERROR_TEST_FAILED;
    }
  }

  UT_LOG_INFO ("Last FinalEventLog entry: type=0x%x size=%u\n", EventType, EventSize);
  UT_ASSERT_EQUAL (EventType, EV_NO_ACTION);
  UT_ASSERT_NOT_NULL (EventData);
  UT_ASSERT_TRUE (EventSize >= sizeof (TCG_LOG_TRUNCATION_EVENT_STRING));
  UT_ASSERT_MEM_EQUAL (EventData, TCG_LOG_TRUNCATION_EVENT_STRING, sizeof (TCG_LOG_TRUNCATION_EVENT_STRING));

  return UNIT_TEST_PASSED;
}

/**
  Test that a snapshot of the event log plus the FinalEventLog entries
  exactly reconstructs a later snapshot of the event log.

  After Tcg2Dxe begins maintaining the FinalEventLog (triggered by the first
  call to GetEventLog), every subsequent event is appended to both the normal
  event log and the FinalEventLog. Therefore:

      Snapshot1 + FinalEventLog_entries == Snapshot2

  where Snapshot1 is the normal log captured immediately after the first
  GetEventLog call and Snapshot2 is the normal log captured after additional
  events have been logged.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED              Reconstruction matches.
  @retval UNIT_TEST_ERROR_TEST_FAILED   Assertion failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestSnapshotPlusFinalMatchesEventLog (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                   Status;
  EFI_PHYSICAL_ADDRESS         LogLocation1;
  EFI_PHYSICAL_ADDRESS         LastEntry1;
  EFI_PHYSICAL_ADDRESS         LogLocation2;
  EFI_PHYSICAL_ADDRESS         LastEntry2;
  BOOLEAN                      Truncated;
  EFI_TCG2_FINAL_EVENTS_TABLE  *FinalTable;
  UINT8                        *Snapshot;
  UINTN                        SnapshotSize1;
  UINTN                        SnapshotSize2;
  UINTN                        FinalEntriesSize;
  UINT8                        *FinalEntriesStart;
  UINT8                        *LogPtr;
  UINT64                       Index;

  // First snapshot. Also activates FinalEventLog logging.
  Status = mTcg2Protocol->GetEventLog (
                            mTcg2Protocol,
                            EFI_TCG2_EVENT_LOG_FORMAT_TCG_2,
                            &LogLocation1,
                            &LastEntry1,
                            &Truncated
                            );

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_FALSE (Truncated);

  // Snapshot length runs from LogLocation1 to one byte past LastEntry1.
  LogPtr = (UINT8 *)(UINTN)LastEntry1;
  if (!TcgLogTestAdvanceEvent (
         &LogPtr,
         (UINT8 *)(UINTN)MAX_ADDRESS,
         NULL,
         NULL,
         NULL,
         NULL
         ))
  {
    UT_LOG_ERROR ("AdvanceEvent failed past LastEntry1\n");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  SnapshotSize1 = (UINTN)LogPtr - (UINTN)LogLocation1;
  UT_LOG_INFO ("Snapshot1: base=0x%lx size=%u\n", LogLocation1, SnapshotSize1);

  Snapshot = AllocateCopyPool (SnapshotSize1, (VOID *)(UINTN)LogLocation1);
  UT_ASSERT_NOT_NULL (Snapshot);

  // Log a small number of additional events. Few enough not to trigger
  // scaling, but enough to make the test meaningful.
  for (Index = 0; Index < MAX_NUM_EXTRA_EVENTS; Index++) {
    Status = TcgLogTestLogSingleEvent (mTcg2Protocol);
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("LogSingleEvent %u failed: %r\n", Index, Status);
      FreePool (Snapshot);
      return UNIT_TEST_ERROR_TEST_FAILED;
    }
  }

  // Retrieve the FinalEventLog from the system configuration table.
  FinalTable = NULL;
  Status     = EfiGetSystemConfigurationTable (&gEfiTcg2FinalEventsTableGuid, (VOID **)&FinalTable);
  if (EFI_ERROR (Status) || (FinalTable == NULL)) {
    UT_LOG_ERROR ("FinalEventsTable lookup failed: %r\n", Status);
    FreePool (Snapshot);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  UT_LOG_INFO ("FinalEventsTable: NumberOfEvents=%lu\n", FinalTable->NumberOfEvents);
  UT_ASSERT_EQUAL (FinalTable->NumberOfEvents, MAX_NUM_EXTRA_EVENTS);

  // Walk all FinalEventLog entries to compute their combined byte size.
  FinalEntriesStart = (UINT8 *)(FinalTable + 1);
  LogPtr            = FinalEntriesStart;
  for (Index = 0; Index < FinalTable->NumberOfEvents; Index++) {
    if (!TcgLogTestAdvanceEvent (
           &LogPtr,
           (UINT8 *)(UINTN)MAX_ADDRESS,
           NULL,
           NULL,
           NULL,
           NULL
           ))
    {
      UT_LOG_ERROR ("AdvanceEvent failed at FinalEventLog index %lu\n", Index);
      FreePool (Snapshot);
      return UNIT_TEST_ERROR_TEST_FAILED;
    }
  }

  FinalEntriesSize = (UINTN)LogPtr - (UINTN)FinalEntriesStart;
  UT_LOG_INFO ("FinalEventLog entries total bytes: %u\n", FinalEntriesSize);

  // Second snapshot.
  Status = mTcg2Protocol->GetEventLog (
                            mTcg2Protocol,
                            EFI_TCG2_EVENT_LOG_FORMAT_TCG_2,
                            &LogLocation2,
                            &LastEntry2,
                            &Truncated
                            );

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_FALSE (Truncated);

  LogPtr = (UINT8 *)(UINTN)LastEntry2;
  if (!TcgLogTestAdvanceEvent (
         &LogPtr,
         (UINT8 *)(UINTN)MAX_ADDRESS,
         NULL,
         NULL,
         NULL,
         NULL
         ))
  {
    UT_LOG_ERROR ("AdvanceEvent failed past LastEntry2\n");
    FreePool (Snapshot);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  SnapshotSize2 = (UINTN)LogPtr - (UINTN)LogLocation2;
  UT_LOG_INFO ("Snapshot2: base=0x%lx size=%u\n", LogLocation2, SnapshotSize2);

  // Snapshot2 must be exactly Snapshot1 followed by the FinalEventLog entries.
  UT_ASSERT_EQUAL (SnapshotSize2, SnapshotSize1 + FinalEntriesSize);
  UT_ASSERT_MEM_EQUAL ((VOID *)(UINTN)LogLocation2, Snapshot, SnapshotSize1);
  UT_ASSERT_MEM_EQUAL ((VOID *)((UINTN)LogLocation2 + SnapshotSize1), FinalEntriesStart, FinalEntriesSize);

  FreePool (Snapshot);
  return UNIT_TEST_PASSED;
}

/**
  Verify the ACPI event log region contains same events as the normal event log.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED             Contents match.
  @retval UNIT_TEST_ERROR_TEST_FAILED  Assertion failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestAcpiEventLogMirrorsNormalLog (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  AcpiLasa;
  UINT32                AcpiLaml;
  EFI_PHYSICAL_ADDRESS  LogLocation;
  EFI_PHYSICAL_ADDRESS  LastEntry;
  BOOLEAN               Truncated;
  UINT8                 *LogPtr;
  UINTN                 NormalSize;

  Status = GetAcpiEventLog (&AcpiLasa, &AcpiLaml);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = mTcg2Protocol->GetEventLog (
                            mTcg2Protocol,
                            EFI_TCG2_EVENT_LOG_FORMAT_TCG_2,
                            &LogLocation,
                            &LastEntry,
                            &Truncated
                            );

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_FALSE (Truncated);

  LogPtr = (UINT8 *)(UINTN)LastEntry;
  if (!TcgLogTestAdvanceEvent (
         &LogPtr,
         (UINT8 *)(UINTN)MAX_ADDRESS,
         NULL,
         NULL,
         NULL,
         NULL
         ))
  {
    UT_LOG_ERROR ("AdvanceEvent past LastEntry failed\n");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  NormalSize = (UINTN)LogPtr - (UINTN)LogLocation;
  UT_LOG_INFO ("Normal log: base=0x%lx size=%u; ACPI region: base=0x%lx size=%u\n", LogLocation, NormalSize, AcpiLasa, AcpiLaml);

  // The normal log must fit within the ACPI region for the mirror test to be
  // meaningful; otherwise the ACPI region has already truncated and this test
  // cannot make a byte-for-byte comparison.
  UT_ASSERT_TRUE (NormalSize <= AcpiLaml);
  UT_ASSERT_MEM_EQUAL ((VOID *)(UINTN)AcpiLasa, (VOID *)(UINTN)LogLocation, NormalSize);

  return UNIT_TEST_PASSED;
}

/**
  Verify that when the normal event log scales the ACPI region receives
  an EV_NO_ACTION truncation marker as its final event. Walks the ACPI
  region event-by-event, tracks the last valid event, and asserts that
  it is EV_NO_ACTION carrying truncation event marker.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED             Truncation marker located.
  @retval UNIT_TEST_ERROR_TEST_FAILED  Assertion failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestAcpiEventLogTruncationMarker (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  AcpiLasa;
  UINT32                AcpiLaml;
  UINT8                 *EventPtr;
  UINT8                 *RegionEnd;
  UINT32                PcrIndex;
  UINT32                EventType;
  UINT32                EventSize;
  UINT8                 *EventData;
  UINT32                LastType;
  UINT32                LastSize;
  UINT8                 *LastData;
  UINTN                 EventCount;

  Status = GetAcpiEventLog (&AcpiLasa, &AcpiLaml);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  EventPtr   = (UINT8 *)(UINTN)AcpiLasa;
  RegionEnd  = EventPtr + AcpiLaml;
  LastType   = 0;
  LastSize   = 0;
  LastData   = NULL;
  EventCount = 0;

  while (TcgLogTestAdvanceEvent (&EventPtr, RegionEnd, &PcrIndex, &EventType, &EventSize, &EventData)) {
    LastType = EventType;
    LastSize = EventSize;
    LastData = EventData;
    EventCount++;

    if (EventPtr >= RegionEnd) {
      break;
    }
  }

  UT_LOG_INFO ("ACPI region events walked: %u, last type=0x%x size=%u\n", EventCount, LastType, LastSize);
  UT_ASSERT_TRUE (EventCount > 0);
  UT_ASSERT_NOT_NULL (LastData);

  // The last valid event in the ACPI region must be the truncation marker.
  UT_ASSERT_EQUAL (LastType, EV_NO_ACTION);
  UT_ASSERT_TRUE (LastSize >= sizeof (TCG_LOG_TRUNCATION_EVENT_STRING));
  UT_ASSERT_MEM_EQUAL (LastData, TCG_LOG_TRUNCATION_EVENT_STRING, sizeof (TCG_LOG_TRUNCATION_EVENT_STRING));

  return UNIT_TEST_PASSED;
}

/**
  Save the unit test framework state and perform a cold reboot.

  @param[in] Context  Unit test context (unused).
**/
STATIC
VOID
EFIAPI
SaveAndReboot (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SaveFrameworkState (NULL, 0);
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
}

/**
  Cleanup for TestPostReadyToBootScaling: enable the DXE pre-ReadyToBoot test
  for the next boot, then save and reboot so the DXE driver runs before
  ReadyToBoot on the second boot.

  @param[in] Context  Unit test context (unused).
**/
STATIC
VOID
EFIAPI
EnableDxeTestAndReboot (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mTcgLogTestProtocol != NULL) {
    Status = mTcgLogTestProtocol->Enable (mTcgLogTestProtocol, TRUE);
    DEBUG ((DEBUG_INFO, "%a: Enable (TRUE) - %r\n", __func__, Status));
  } else {
    DEBUG ((DEBUG_ERROR, "%a: mTcgLogTestProtocol is NULL, cannot enable\n", __func__));
  }

  SaveAndReboot (Context);
}

/**
  Prerequisite: locate the TCG2 and TcgLogTest protocols.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED                      Protocols located.
  @retval UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Protocol not found.
**/
UNIT_TEST_STATUS
EFIAPI
LocateProtocols (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **)&mTcg2Protocol);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  Status = gBS->LocateProtocol (&gTcgLogTestProtocolGuid, NULL, (VOID **)&mTcgLogTestProtocol);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  return UNIT_TEST_PASSED;
}

/**
  Entry point for TcgLogTestApp.

  @param[in] ImageHandle  Image handle.
  @param[in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS  Tests dispatched and framework freed.
  @retval Other        Framework initialization failed.
**/
EFI_STATUS
EFIAPI
TcgLogTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      Suite;

  Framework = NULL;

  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InitUnitTestFramework failed: %r\n", __func__, Status));
    return Status;
  }

  Status = CreateUnitTestSuite (&Suite, Framework, "TCG Log Scaling Tests", "TcgLogTest", NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  AddTestCase (Suite, "ACPI event log mirrors normal log", "AcpiLogMirrors", TestAcpiEventLogMirrorsNormalLog, LocateProtocols, SaveAndReboot, NULL);
  AddTestCase (Suite, "Post-ReadyToBoot scaling succeeds", "PostRtbScaling", TestPostReadyToBootScaling, LocateProtocols, NULL, NULL);
  AddTestCase (Suite, "ACPI event log gets truncation marker", "AcpiLogTruncates", TestAcpiEventLogTruncationMarker, LocateProtocols, NULL, NULL);
  AddTestCase (Suite, "Scaling occurs X times before truncating", "ScaleLimitTruncates", TestScaleLimitTruncatesLog, LocateProtocols, EnableDxeTestAndReboot, NULL);
  AddTestCase (Suite, "Snapshot plus FinalEventLog matches full log", "SnapshotPlusFinalMatches", TestSnapshotPlusFinalMatchesEventLog, LocateProtocols, NULL, NULL);
  AddTestCase (Suite, "FinalEventLog gets truncation marker", "FinalEventLogTruncates", TestFinalEventLogTruncationMarker, LocateProtocols, NULL, NULL);
  AddTestCase (Suite, "Pre-ReadyToBoot DXE results contain PASS", "PreRtbResults", TestPreReadyToBootScaling, LocateProtocols, SaveAndReboot, NULL);

  Status = RunAllTestSuites (Framework);

Done:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}
