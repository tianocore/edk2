# TcgLogTest

TcgLogTest validates the dynamic event log scaling functionality implemented
by `Tcg2Dxe`. It consists of a DXE driver (`TcgLogTestDxe`) and a UEFI shell
unit test application (`TcgLogTestApp`) that coordinate across multiple boots
to exercise scaling both before and after `ReadyToBoot`.

## Components

### TcgLogTestDxe (DXE_DRIVER)

A DXE driver that runs pre-ReadyToBoot scaling tests on demand. It installs
the `TCG_LOG_TEST_PROTOCOL` which allows the test application to enable/disable
the tests and retrieve results.

**Entry flow:**

1. Installs the `TCG_LOG_TEST_PROTOCOL` on a new handle.
2. Checks the NV variable `TcgLogTestEnable` (existence-based: present =
   enabled, absent = disabled).
3. If disabled:
   - Returns immediately. The protocol is still available for the test app
     to call `Enable` on.
4. If enabled:
   - Deletes the enable variable. This makes it so the test only runs once.
   - Locates `EFI_TCG2_PROTOCOL`.
   - Runs `TestPreReadyToBootScaling`.
   - Records results in an internal log buffer which can be acquired via
     `GetLog`.

#### Protocol

The `TCG_LOG_TEST_PROTOCOL` provides the following function(s):

| Function | Description |
| -------- | ----------- |
| `GetLog` | Returns a pointer to the DXE driver's internal ASCII log buffer and its size. Returns `EFI_NOT_STARTED` if the test did not run this boot. |
| `Enable` | Creates or deletes the `TcgLogTestEnable` NV variable to enable or disable the DXE test for the next boot. |

The `TCG_LOG_TEST_PROTOCOL` GUID is defined in `TcgLogTest.h` and declared
in `SecurityPkg.dec`.

```code
#define TCG_LOG_TEST_PROTOCOL_GUID \
  { 0xA3C12F80, 0x7D9E, 0x4B5A, { 0x91, 0xE4, 0x6C, 0xF8, 0x2D, 0xA1, 0xB7, 0x03 } }
```

#### NV Variable

The enable/disable mechanism uses an NV variable rather than UnitTest saved
context because the DXE driver and the test application are separate binaries.
The DXE driver does not use `UnitTestLib` and cannot access the framework's
persisted state. An NV variable is the standard cross-module communication
channel in UEFI.

| Attribute | Value |
| --------- | ----- |
| Name | `TcgLogTestEnable` |
| Vendor GUID | `gTcgLogTestProtocolGuid` |
| Attributes | `NV + BS` |
| Semantics | Existence-based: variable present = enabled, variable absent = disabled |

#### Test: TestPreReadyToBootScaling

Executed before `ReadyToBoot` when the NV variable is present indicating the
test was enabled. Exercises dynamic scaling before `ReadyToBoot` has fired.

1. Calls `TcgLogTestLogEventsUntilScaled` to repeatedly log `EV_NO_ACTION`
   events to PCR 8. Internally, the helper registers a notification callback
   on `gTcg2EventLogScaledGuid` and stops as soon as `Tcg2Dxe` signals that
   GUID, indicating the event log was dynamically scaled.
2. Writes `PASS` or `FAIL` (with details) to the internal log buffer.

### TcgLogTestApp (UEFI_APPLICATION)

A UnitTest framework shell application that runs post-ReadyToBoot scaling tests.

The suite registers five test cases. Each case shares a `LocateProtocols`
prerequisite that resolves `EFI_TCG2_PROTOCOL` and `TCG_LOG_TEST_PROTOCOL`.

| # | Test Case (Class Name) | Function | Cleanup |
| - | ---------------------- | -------- | ------- |
| 1 | `PostRtbScaling` | `TestPostReadyToBootScaling` | none |
| 2 | `ScaleLimitTruncates` | `TestScaleLimitTruncatesLog` | `EnableDxeTestAndReboot` |
| 3 | `SnapshotPlusFinalMatches` | `TestSnapshotPlusFinalMatchesEventLog` | none |
| 4 | `FinalEventLogTruncates` | `TestFinalEventLogTruncationMarker` | none |
| 5 | `PreRtbResults` | `TestPreReadyToBootScaling` | `SaveAndReboot` |

#### Test: TestPostReadyToBootScaling

Executed after `ReadyToBoot` in the UEFI shell. Exercises dynamic scaling
after `ReadyToBoot` has fired.

1. Calls `TcgLogTestLogEventsUntilScaled` to repeatedly log `EV_NO_ACTION`
   events to PCR 8. Internally, the helper registers a notification callback
   on `gTcg2EventLogScaledGuid` and stops as soon as `Tcg2Dxe` signals that
   GUID, indicating the event log was dynamically scaled.

#### Test: TestScaleLimitTruncatesLog

Verifies that the normal event log can only be dynamically scaled up to
`TCG_EVENT_LOG_MAX_SCALE_COUNT` times, after which scaling is refused and
`GetEventLog` reports the log as truncated.

1. `TestPostReadyToBootScaling` already consumed one scale, so this test
   calls `TcgLogTestLogEventsUntilScaled` repeatedly to exhaust the remaining
   `TCG_EVENT_LOG_MAX_SCALE_COUNT - 1` scales.
2. Calls `TcgLogTestLogEventsUntilScaled` once more and asserts it returns
   an error and reports `Scaled = FALSE`.
3. Calls `GetEventLog` and asserts the `Truncated` flag is `TRUE`.

#### Test: TestSnapshotPlusFinalMatchesEventLog

Validates the relationship between the normal event log and the
`FinalEventLog`: after the first `GetEventLog` call activates FinalEventLog
logging, every newly logged event must be appended to **both** the normal
log and the FinalEventLog. Therefore:

```text
Snapshot1 + FinalEventLog_entries == Snapshot2
```

1. Calls `GetEventLog` to capture `Snapshot1` (and activate FinalEventLog
   logging).
2. Calls `TcgLogTestLogSingleEvent` `MAX_NUM_EXTRA_EVENTS` times so the new
   entries are small enough to avoid triggering scaling.
3. Locates `EFI_TCG2_FINAL_EVENTS_TABLE` from the system configuration table
   and asserts `NumberOfEvents == MAX_NUM_EXTRA_EVENTS`.
4. Walks the FinalEventLog entries to compute their combined byte size.
5. Calls `GetEventLog` again to capture `Snapshot2`.
6. Asserts `Snapshot2 == Snapshot1 || FinalEventLog_entries` byte-for-byte.

#### Test: TestFinalEventLogTruncationMarker

Verifies that when the `FinalEventLog` fills up, `Tcg2Dxe` appends a final
`EV_NO_ACTION` event carrying the `TCG_LOG_TRUNCATION_EVENT_STRING` marker
(`"TCG Event Log Truncated"`) so consumers can recognise the truncated state.

1. Calls `GetEventLog` to activate FinalEventLog logging and assert it is
   not yet truncated.
2. Calls `TcgLogTestLogEventsUntilScaled` in a loop until `HashLogExtendEvent`
   returns `EFI_VOLUME_FULL`, signalling FinalEventLog truncation. (The normal
   log can still scale; the FinalEventLog cannot.)
3. Locates `EFI_TCG2_FINAL_EVENTS_TABLE` and walks all `NumberOfEvents` entries
   to land on the last entry.
4. Asserts the last entry is `EV_NO_ACTION` and its payload begins with the
   ASCII bytes of `TCG_LOG_TRUNCATION_EVENT_STRING`.

#### Test: TestPreReadyToBootScaling Results

Verifies the DXE driver's pre-ReadyToBoot results.

1. Locates `TCG_LOG_TEST_PROTOCOL` and calls `GetLog`.
2. Dumps the DXE driver's log for visibility.
3. Asserts the log contains `"PASS"` and does not contain `"FAIL"`.

## Three-Boot Reboot Flow

The tests require three boots to complete because scaling must be tested in
two different phases of the boot process, and each phase requires a separate
boot. The final boot should guarantee that the TCG event log is not polluted
with the test `NO_ACTION_EVENT` events used to scale the log.

```text
Boot 1 (Post-ReadyToBoot scaling + truncation)
├── TcgLogTestDxe:
│   ├── Installs TCG_LOG_TEST_PROTOCOL.
│   ├── NV variable absent → Test not enabled → SKIPPED.
├── TcgLogTestApp:
│   ├── Launched from UEFI shell. (UnitTest Framework)
│   ├── Test Prerequisites:
│   │   └── Calls LocateProtocols() to locate the TCG2 and TcgLogTest protocols.
│   ├── TestPostReadyToBootScaling():
│   │   ├── Calls TcgLogTestLogEventsUntilScaled() to scale the event log once.
│   │   └── PASS.
│   ├── TestScaleLimitTruncatesLog():
│   │   ├── Scales the remaining (TCG_EVENT_LOG_MAX_SCALE_COUNT - 1) times.
│   │   ├── Attempts one more scale and asserts it fails.
│   │   ├── Calls GetEventLog() and asserts Truncated == TRUE.
│   │   └── PASS.
│   └── Test Cleanup (for TestScaleLimitTruncatesLog):
│       └── Calls EnableDxeTestAndReboot().
│           ├── Calls Enable (TRUE) to create the NV variable.
│           └── Calls SaveAndReboot() to SaveFrameworkState + EfiResetCold.
│
Boot 2 (DXE pre-ReadyToBoot test + FinalEventLog tests + DXE results)
├── TcgLogTestDxe:
│   ├── Installs TCG_LOG_TEST_PROTOCOL.
│   ├── NV variable present → Test enabled → Deletes the NV variable → Runs.
│   ├── Calls TestPreReadyToBootScaling():
│   │   ├── Calls TcgLogTestLogEventsUntilScaled() to scale the event log.
│   │   ├── PASS.
│   │   └── Logs results into internal buffer for later access via GetLog().
├── TcgLogTestApp:
│   ├── Resumes execution from UEFI shell. (UnitTest Framework)
│   ├── TestPostReadyToBootScaling() → already PASSED → SKIPPED.
│   ├── TestScaleLimitTruncatesLog() → already PASSED → SKIPPED.
│   ├── TestSnapshotPlusFinalMatchesEventLog():
│   │   ├── Captures Snapshot1 of the normal log and activates FinalEventLog.
│   │   ├── Logs MAX_NUM_EXTRA_EVENTS individual events.
│   │   ├── Reads FinalEventsTable and asserts NumberOfEvents matches.
│   │   ├── Captures Snapshot2 and asserts Snapshot1 || FinalEntries == Snapshot2.
│   │   └── PASS.
│   ├── TestFinalEventLogTruncationMarker():
│   │   ├── Logs events until HashLogExtendEvent returns EFI_VOLUME_FULL.
│   │   ├── Walks FinalEventsTable to the last entry.
│   │   ├── Asserts the entry is EV_NO_ACTION carrying TCG_LOG_TRUNCATION_EVENT_STRING.
│   │   └── PASS.
│   ├── TestPreReadyToBootScaling() (DXE results):
│   │   ├── Calls GetLog() to acquire the TcgLogTestDxe log.
│   │   ├── Verifies PASS in TcgLogTestDxe log.
│   │   └── PASS.
│   └── Test Cleanup (for TestPreReadyToBootScaling):
│       └── Calls SaveAndReboot() to SaveFrameworkState + EfiResetCold.
│
Boot 3 (Final Report/Results)
├── TcgLogTestDxe:
│   ├── Installs TCG_LOG_TEST_PROTOCOL.
│   ├── NV variable absent → Test not enabled → Exit.
├── TcgLogTestApp:
│   ├── Resumes execution from UEFI shell. (UnitTest Framework)
│   ├── All tests already PASSED → SKIPPED.
│   └── Reports final results, cleans up framework state.
```

## Shared Code (TcgLogTestCommon)

Common functions compiled into both binaries:

| Function | Description |
| -------- | ----------- |
| `TcgLogTestAdvanceEvent` | Parses one TCG 2.0 event entry, advancing the pointer to the next event. Handles SHA-1/256/384/512/SM3 digest algorithms. |
| `TcgLogTestLogEventsUntilScaled` | Builds a test event and logs it repeatedly via `HashLogExtendEvent` until `Tcg2Dxe` signals `gTcg2EventLogScaledGuid` (indicating the event log was dynamically scaled) or `HashLogExtendEvent` returns an error. |
| `TcgLogTestLogSingleEvent` | Builds and logs exactly one fixed test event via `HashLogExtendEvent`. Used by tests that need to add a small, deterministic number of events without triggering scaling. |

## Truncation Marker Event

When the `FinalEventLog` fills up and `Tcg2Dxe` can no longer append new
entries, it writes a final `EV_NO_ACTION` event whose data payload is the
ASCII string `"TCG Event Log Truncated"` (`TCG_LOG_TRUNCATION_EVENT_STRING`).
`TestFinalEventLogTruncationMarker` exercises this code path and verifies the
marker is present as the last entry of the table. The string constant in the
test sources must stay in sync with the definition in `Tcg2Dxe.c`.

## Platform Integration

### DSC

Add both modules to the platform DSC under the `[Components]` section,
typically gated behind a TPM enable flag:

```ini
!if $(TPM2_ENABLE) == TRUE
  SecurityPkg/Tcg/TcgLogTest/TcgLogTestDxe.inf
  SecurityPkg/Tcg/TcgLogTest/TcgLogTestApp.inf
!endif
```

### FDF

Add both modules to the platform FDF so they are included in the firmware
volume, typically gated behind a TPM enable flag. The DXE driver must be in
the DXE FV so it loads during DXE dispatch. The test application can be in
the same FV or a separate one accessible from the UEFI shell:

```ini
!if $(TPM2_ENABLE) == TRUE
  INF SecurityPkg/Tcg/TcgLogTest/TcgLogTestDxe.inf
  INF SecurityPkg/Tcg/TcgLogTest/TcgLogTestApp.inf
!endif
```

### Running the Test

1. Boot to the UEFI shell.
2. Run the test application: `TcgLogTestApp.efi`
3. The system will automatically reboot twice more to complete the three-boot
   flow.
4. On the third boot, the framework reports final results to the shell.
