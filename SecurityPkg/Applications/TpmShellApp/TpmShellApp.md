# TpmShellApp

A UEFI shell application for testing TPM 2.0 Physical Presence Interface operations,
including querying and configuring PCR banks. The app communicates exclusively through the
`EFI_TCG2_PROTOCOL` — it has no direct dependencies on TPM command libraries, physical
presence libraries, or TPM hardware.

## Table of Contents

- [Overview](#overview)
- [Commands](#commands)
- [Build Integration](#build-integration)
- [Protocol Dependency](#protocol-dependency)
- [Output and Debugging](#output-and-debugging)

## Overview

TpmShellApp is a `UEFI_APPLICATION` that validates TPM 2.0 PCR bank operations via the
TCG2 Protocol. It is designed to test that a platform's Physical Presence Interface
correctly handles PCR bank change requests, including verifying that unsupported operations
are gracefully rejected.

The app uses positional command-line arguments (no flags or switches) and outputs all
results through `Print()` (from `UefiLib`), which writes directly to the UEFI shell
console.

## Commands

```text
TpmShellApp help             Show usage information
TpmShellApp getpcr           Show supported and active PCR banks
TpmShellApp setpcr <mask>    Request a PCR bank configuration change
TpmShellApp enableall        Request enabling all supported PCR banks
TpmShellApp dumplog          Dump the TCG2 event log
TpmShellApp replay           Replay event log and verify PCRs
TpmShellApp getresult        Show the result of the last SetActivePcrBanks call
```

### `help`

Prints usage information, PCR bank bitmask values, and examples.

### `getpcr`

Calls `Tcg2Protocol->GetCapability()` and displays:

- **Supported PCR banks** — algorithms the TPM hardware supports (`HashAlgorithmBitmap`).
- **Active PCR banks** — algorithms currently enabled (`ActivePcrBanks`).

Both are filtered by the firmware's registered hash algorithms (see
[Hash Algorithm Filtering](#hash-algorithm-filtering) below).

Example output:

```text
[TPM 2.0 Information]

Supported PCR banks: 2
 * SHA256

Active PCR banks: 2
 * SHA256
```

### `setpcr <mask>`

Calls `Tcg2Protocol->SetActivePcrBanks()` with the provided hex bitmask. The mask is a
combination of `EFI_TCG2_BOOT_HASH_ALG_*` values:

| Value | Algorithm |
| ----- | --------- |
| `0x01` | SHA1 |
| `0x02` | SHA256 |
| `0x04` | SHA384 |
| `0x08` | SHA512 |
| `0x10` | SM3_256 |

Values can be combined: `0x06` = SHA256 + SHA384.

The mask parameter accepts hex with or without a `0x` prefix.

```text
TpmShellApp setpcr 0x2       Enable SHA256 only
TpmShellApp setpcr 0x6       Enable SHA256 + SHA384
TpmShellApp setpcr 2         Also valid (no prefix)
```

> **Note**: `SetActivePcrBanks` submits a Physical Presence request. The actual bank
> change takes effect on the next reboot, processed by
> `Tcg2PhysicalPresenceLibProcessRequest()` during BDS.

### `enableall`

Calls `GetCapability()` to discover all supported hash algorithms, then calls
`SetActivePcrBanks()` with the full `HashAlgorithmBitmap`. This is equivalent to
requesting that all supported PCR banks be enabled.

### `dumplog`

Calls `Tcg2Protocol->GetEventLog()` with `EFI_TCG2_EVENT_LOG_FORMAT_TCG_2` (crypto-agile
format) to retrieve the firmware's TCG2 event log. Parses the Spec ID Event header to
discover the hash algorithms and digest sizes, then walks each `TCG_PCR_EVENT2` entry
displaying:

- **PCR index** — which PCR was extended.
- **Event type** — the TCG event type (e.g., `EV_EFI_VARIABLE_DRIVER_CONFIG`,
  `EV_SEPARATOR`).
- **Digest(s)** — the full hex digest for each algorithm in the event.
- **Event data size** — the number of bytes in the event payload.

Prints a total event count at the end.

### `replay`

Replays the TCG2 event log to compute expected PCR values, then reads the
actual PCR values from the TPM via `SubmitCommand` (TPM2_PCR_Read) and
compares them.

For each PCR that was extended in the log, the command displays:

- **Replayed digest** — the PCR value computed locally by replaying all
  extend operations.
- **Actual digest** — the current PCR value read from the TPM.
- **Result** — `PASS` if they match, `FAIL` if they differ.

Prints a summary with total verified, passed, and failed counts.

Supports SHA1, SHA256, SHA384, and SHA512 for replay hashing. Algorithms
not supported by `BaseCryptLib` (e.g., SM3_256) are skipped.

### `getresult`

Calls `Tcg2Protocol->GetResultOfSetActivePcrBanks()` which queries the Physical Presence
library for the result of the most recent `SetActivePcrBanks` operation. Displays:

- **Operation present** — whether a previous operation result exists (`YES` / `NO`).
- **Response code** — the TCG PP return code (0 = success).

## Build Integration

### INF

The app is defined in `SecurityPkg/Applications/TpmShellApp/TpmShellApp.inf`:

| Property | Value |
| -------- | ----- |
| `MODULE_TYPE` | `UEFI_APPLICATION` |
| `ENTRY_POINT` | `TpmShellAppEntry` |
| `FILE_GUID` | `A3B2D4F1-7E6C-4A89-B5D0-3C1F8E2A9D07` |

Dependencies:

| Section | Items |
| ------- | ----- |
| Packages | `MdePkg`, `MdeModulePkg`, `CryptoPkg`, `ShellPkg` |
| LibraryClasses | `BaseLib`, `BaseCryptLib`, `BaseMemoryLib`, `IntrinsicLib`, `ShellLib`, `UefiApplicationEntryPoint`, `UefiBootServicesTableLib`, `UefiLib` |
| Protocols | `gEfiTcg2ProtocolGuid` |

The app intentionally has **no dependency** on `SecurityPkg`, TPM command libraries,
physical presence libraries, or runtime services. All TPM interaction goes through the
TCG2 Protocol.

### DSC

Add the INF to the platform DSC components section:

```ini
SecurityPkg/Applications/TpmShellApp/TpmShellApp.inf
```

The platform DSC must also map `IntrinsicLib` for the `UEFI_APPLICATION` module type.
`IntrinsicLib` provides compiler-generated `memcpy`/`memset` intrinsics that the VS2022
toolchain emits for large memory operations such as CopyMem and ZeroMem:

```ini
[LibraryClasses.common.UEFI_APPLICATION]
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

The app is typically included unconditionally (outside any TPM enable guard) so it can be
built regardless of TPM enablement. It will report a clear error when the TCG2 Protocol is
not available.

### FDF

Add the INF to the appropriate firmware volume in the platform FDF:

```ini
INF SecurityPkg/Applications/TpmShellApp/TpmShellApp.inf
```

## Protocol Dependency

TpmShellApp uses only `EFI_TCG2_PROTOCOL` (`gEfiTcg2ProtocolGuid`). This protocol is
installed by `Tcg2Dxe.efi`, which requires TPM to be enabled in the platform
configuration.

If the protocol is not found, the app prints:

```text
TCG2 Protocol not found - Not Found
  TPM 2.0 may not be enabled on this platform.
```

### Protocol Functions Used

| Function | Command | Purpose |
| -------- | ------- | ------- |
| `GetCapability` | `getpcr`, `enableall` | Query supported/active PCR banks |
| `SetActivePcrBanks` | `setpcr`, `enableall` | Submit PP request for bank change |
| `GetEventLog` | `dumplog`, `replay` | Retrieve the crypto-agile event log |
| `SubmitCommand` | `replay` | Send TPM2_PCR_Read to read actual PCR values |
| `GetResultOfSetActivePcrBanks` | `getresult` | Query result of last bank change request |

### Hash Algorithm Filtering

The `HashAlgorithmBitmap` and `ActivePcrBanks` reported by `GetCapability` are already
filtered by the firmware's hash library configuration. The chain is:

1. `PcdTpm2HashMask` gates which `HashInstanceLib` modules register.
2. `Tcg2Dxe` intersects the registered hash bitmap with the TPM's hardware capabilities.
3. `GetCapability` only reports the intersection.

This means the app may not see all algorithms the TPM hardware supports. To expose
additional algorithms, update `PcdTpm2HashMask` in the platform DSC.

## Output and Debugging

All output uses `Print()` from `UefiLib`, so it appears directly on the UEFI shell
console — no serial log or `AdvancedLogger` configuration is required to see results.
