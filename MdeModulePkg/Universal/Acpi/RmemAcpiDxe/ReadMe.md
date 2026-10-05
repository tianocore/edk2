# Reserved-Memory Reporting through ACPI <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Copyright (c) Microsoft Corporation. <!-- [CODE_FIRST] 13253 -->
SPDX-License-Identifier: BSD-2-Clause-Patent <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Status <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Reserved-Memory Reporting (RMEM) Revision 1 is the Microsoft-recommended <!-- [CODE_FIRST] 13253 -->
firmware interface for silicon partners and OEMs to report reserved <!-- [CODE_FIRST] 13253 -->
physical-memory ranges on Windows devices. This document defines the `RMEM` <!-- [CODE_FIRST] 13253 -->
ACPI signature, wire format, category values, producer interfaces, validation <!-- [CODE_FIRST] 13253 -->
rules, and publication policy. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
RMEM is defined by EDK II/Mu and is not currently an ACPI or UEFI industry <!-- [CODE_FIRST] 13253 -->
standard. Industry-standard designation would require approval by the <!-- [CODE_FIRST] 13253 -->
appropriate standards body and an official ACPI signature allocation. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Motivation <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Firmware can reserve physical memory for security services, device operation, <!-- [CODE_FIRST] 13253 -->
firmware runtime use, shared memory, crash handling, and other platform <!-- [CODE_FIRST] 13253 -->
functions. Operating systems generally report an aggregate hardware-reserved <!-- [CODE_FIRST] 13253 -->
amount without explaining the purpose of each range. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
RMEM helps: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- Explain differences between installed and operating-system-visible memory. <!-- [CODE_FIRST] 13253 -->
- Diagnose unexpectedly large reservations. <!-- [CODE_FIRST] 13253 -->
- Compare firmware configurations across systems. <!-- [CODE_FIRST] 13253 -->
- Attribute reservations without a vendor-specific kernel-mode or user-mode <!-- [CODE_FIRST] 13253 -->
  driver. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Goals <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- Report the base address, size, purpose category, and diagnostic label for <!-- [CODE_FIRST] 13253 -->
  each authoritative reserved-memory range. <!-- [CODE_FIRST] 13253 -->
- Keep platform-specific discovery separate from common table construction. <!-- [CODE_FIRST] 13253 -->
- Support reservations known before DXE and reservations finalized during DXE. <!-- [CODE_FIRST] 13253 -->
- Publish one versioned, checksummed ACPI table. <!-- [CODE_FIRST] 13253 -->
- Avoid changing the system memory map or granting access to reported ranges. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Non-Goals <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- Defining ownership, access permissions, or security policy for a range. <!-- [CODE_FIRST] 13253 -->
- Replacing the UEFI memory map, ACPI resource descriptions, or existing <!-- [CODE_FIRST] 13253 -->
  architecture-specific reservation mechanisms. <!-- [CODE_FIRST] 13253 -->
- Allowing an operating system or application to access reported memory. <!-- [CODE_FIRST] 13253 -->
- Reporting device MMIO, uninstalled address space, alignment holes, or <!-- [CODE_FIRST] 13253 -->
  ordinary usable memory. <!-- [CODE_FIRST] 13253 -->
- Standardizing platform-specific discovery mechanisms. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Architecture <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Range discovery remains with platform and silicon modules. The common RMEM DXE <!-- [CODE_FIRST] 13253 -->
publisher owns validation, conflict handling, serialization, and ACPI table <!-- [CODE_FIRST] 13253 -->
installation. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
```mermaid <!-- [CODE_FIRST] 13253 -->
flowchart LR <!-- [CODE_FIRST] 13253 -->
  PreDxeProducer["Pre-DXE Platform or Silicon Producer"] <!-- [CODE_FIRST] 13253 -->
  DxeProducer["DXE Platform or Silicon Producer"] <!-- [CODE_FIRST] 13253 -->
  Hob[("RMEM Record GUID HOBs")] <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  subgraph CommonDriver["Common RMEM DXE Driver"] <!-- [CODE_FIRST] 13253 -->
    Publisher["RMEM ACPI Publisher"] <!-- [CODE_FIRST] 13253 -->
  end <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  Table[("RMEM ACPI Table")] <!-- [CODE_FIRST] 13253 -->
  Consumer["Operating-System Diagnostic Consumer"] <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  PreDxeProducer -->|"BuildGuidDataHob()"| Hob <!-- [CODE_FIRST] 13253 -->
  Publisher -->|"GetFirstGuidHob() / GetNextGuidHob()"| Hob <!-- [CODE_FIRST] 13253 -->
  Hob -->|"RMEM_HOB_RECORD data"| Publisher <!-- [CODE_FIRST] 13253 -->
  DxeProducer -->|"AddReservedRange()"| Publisher <!-- [CODE_FIRST] 13253 -->
  Publisher -->|"InstallAcpiTable()"| Table <!-- [CODE_FIRST] 13253 -->
  Table --> Consumer <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  classDef rmem fill:#1e3a5f,stroke:#0f172a,color:#fff <!-- [CODE_FIRST] 13253 -->
  class Hob,Publisher,Table rmem <!-- [CODE_FIRST] 13253 -->
  style CommonDriver fill:#1e3a5f,stroke:#0f172a,stroke-width:3px,color:#fff <!-- [CODE_FIRST] 13253 -->
``` <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
A platform may use the pre-DXE path, the DXE path, or both. Each reservation <!-- [CODE_FIRST] 13253 -->
should have one owning producer and one transport path. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
### Pre-DXE Producers <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
A producer creates one `RMEM_HOB_RECORD` GUID HOB for each static reservation <!-- [CODE_FIRST] 13253 -->
that is positively known before DXE. The producer must use an authoritative <!-- [CODE_FIRST] 13253 -->
reservation source. A gap in the UEFI memory map is not sufficient evidence <!-- [CODE_FIRST] 13253 -->
that a range is reserved DRAM. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The HOB record is defined in <!-- [CODE_FIRST] 13253 -->
`MdeModulePkg/Include/Guid/ReservedMemoryReportingHob.h`. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
### DXE Producers <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
A DXE producer locates `EDKII_RMEM_REGISTRATION_PROTOCOL` and calls <!-- [CODE_FIRST] 13253 -->
`AddReservedRange()` for a reservation allocated, discovered, or finalized <!-- [CODE_FIRST] 13253 -->
during DXE. The publisher copies the label before returning. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The registration protocol is defined in <!-- [CODE_FIRST] 13253 -->
`MdeModulePkg/Include/Protocol/ReservedMemoryReporting.h`. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
### Common Publisher <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
`RmemAcpiDxe` performs the following operations: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
1. Imports and validates all RMEM GUID HOB instances. <!-- [CODE_FIRST] 13253 -->
2. Installs the DXE registration protocol. <!-- [CODE_FIRST] 13253 -->
3. Accepts registrations until `ReadyToBoot`. <!-- [CODE_FIRST] 13253 -->
4. Applies the same validation and conflict policy to both producer paths. <!-- [CODE_FIRST] 13253 -->
5. Freezes the entry set at `ReadyToBoot`. <!-- [CODE_FIRST] 13253 -->
6. Constructs, checksums, and installs one RMEM ACPI table. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Revision 1 Table Layout <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The Revision 1 table contains a standard 36-byte ACPI description header, a <!-- [CODE_FIRST] 13253 -->
2-byte entry count, a 2-byte entry offset, and zero or more packed 48-byte <!-- [CODE_FIRST] 13253 -->
entries. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
```text <!-- [CODE_FIRST] 13253 -->
+----------------------+------------+-------------+----------+----------+----------+---------+----------+ <!-- [CODE_FIRST] 13253 -->
| ACPI header          | EntryCount | EntryOffset | Base     | Size     | Category | Flags   | Label    | <!-- [CODE_FIRST] 13253 -->
| 36 bytes             | 2 bytes    | 2 bytes     | 8 bytes  | 8 bytes  | 2 bytes  | 2 bytes | 28 bytes | <!-- [CODE_FIRST] 13253 -->
+----------------------+------------+-------------+----------+----------+----------+---------+----------+ <!-- [CODE_FIRST] 13253 -->
|<----------- table header: 40 bytes ----------->|<------------ each entry: 48 bytes ------------>| <!-- [CODE_FIRST] 13253 -->
``` <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
For Revision 1, `EntryOffset` is 40 and the total table length is <!-- [CODE_FIRST] 13253 -->
`EntryOffset + (48 * EntryCount)` bytes. <!-- [CODE_FIRST] 13253 -->
The entry array therefore begins at an 8-byte-aligned offset from the table <!-- [CODE_FIRST] 13253 -->
base. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
<table> <!-- [CODE_FIRST] 13253 -->
  <thead> <!-- [CODE_FIRST] 13253 -->
    <tr><th>Table offset</th><th>Size</th><th>Field</th><th>Description</th></tr> <!-- [CODE_FIRST] 13253 -->
  </thead> <!-- [CODE_FIRST] 13253 -->
  <tbody> <!-- [CODE_FIRST] 13253 -->
    <tr><td>0</td><td>36</td><td><code>Header</code></td><td>Standard ACPI description header</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>36</td><td>2</td><td><code>EntryCount</code></td><td>Number of entries following the header</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>38</td><td>2</td><td><code>EntryOffset</code></td><td>Byte offset from the table start to the first entry</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>40</td><td><code>48 * EntryCount</code></td><td><code>Entries</code></td><td>Packed array of RMEM entries</td></tr> <!-- [CODE_FIRST] 13253 -->
  </tbody> <!-- [CODE_FIRST] 13253 -->
</table> <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Each entry has the following layout: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
<table> <!-- [CODE_FIRST] 13253 -->
  <thead> <!-- [CODE_FIRST] 13253 -->
    <tr><th>Entry offset</th><th>Size</th><th>Field</th><th>Description</th></tr> <!-- [CODE_FIRST] 13253 -->
  </thead> <!-- [CODE_FIRST] 13253 -->
  <tbody> <!-- [CODE_FIRST] 13253 -->
    <tr><td>0</td><td>8</td><td><code>Base</code></td><td>First physical byte of the reserved range</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>8</td><td>8</td><td><code>Size</code></td><td>Range length in bytes</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>16</td><td>2</td><td><code>Category</code></td><td>Numeric purpose category</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>18</td><td>2</td><td><code>Flags</code></td><td>Entry attributes</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>20</td><td>28</td><td><code>Label</code></td><td>Null-terminated, zero-padded ASCII label</td></tr> <!-- [CODE_FIRST] 13253 -->
  </tbody> <!-- [CODE_FIRST] 13253 -->
</table> <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The authoritative structure definitions are in <!-- [CODE_FIRST] 13253 -->
`MdeModulePkg/Include/Guid/ReservedMemoryReportingTable.h`. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Categories <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Revision 1 defines the following wire values: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
<table> <!-- [CODE_FIRST] 13253 -->
  <thead> <!-- [CODE_FIRST] 13253 -->
    <tr><th>Value</th><th>Category</th><th>Intended use</th></tr> <!-- [CODE_FIRST] 13253 -->
  </thead> <!-- [CODE_FIRST] 13253 -->
  <tbody> <!-- [CODE_FIRST] 13253 -->
    <tr><td>0</td><td>Unknown</td><td>Invalid sentinel for missing or uninitialized values</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>1</td><td>Security</td><td>Isolated execution, security processors, or protected services</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>2</td><td>SharedMemory</td><td>Memory shared across firmware execution environments</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>3</td><td>DisplayFramebuffer</td><td>Pre-OS or persistent display framebuffer memory</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>4</td><td>GpuReserved</td><td>Memory reserved for graphics use</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>5</td><td>AiAcceleratorReserved</td><td>Memory reserved for AI acceleration</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>6</td><td>FirmwareRuntime</td><td>Runtime data, services, or crash diagnostics</td></tr> <!-- [CODE_FIRST] 13253 -->
    <tr><td>7</td><td>Other</td><td>A reservation that does not fit another category</td></tr> <!-- [CODE_FIRST] 13253 -->
  </tbody> <!-- [CODE_FIRST] 13253 -->
</table> <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
`RmemCategoryMax` is an exclusive implementation bound and is not a valid wire <!-- [CODE_FIRST] 13253 -->
value. Producers must provide a category greater than `RmemCategoryUnknown` and <!-- [CODE_FIRST] 13253 -->
less than `RmemCategoryMax`. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Flags <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Revision 1 defines `RMEM_ENTRY_FLAG_ADDRESS_HIDDEN` in bit 0. When this flag is <!-- [CODE_FIRST] 13253 -->
clear, `Base` contains the first physical byte of the range. When this flag is <!-- [CODE_FIRST] 13253 -->
set, the serialized `Base` field must be zero and consumers must treat the <!-- [CODE_FIRST] 13253 -->
address as intentionally redacted. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Producers must provide the actual base address to the GUID HOB or registration <!-- [CODE_FIRST] 13253 -->
protocol even when the address is hidden. The publisher uses the actual address <!-- [CODE_FIRST] 13253 -->
for range and overlap validation and redacts it only when constructing the ACPI <!-- [CODE_FIRST] 13253 -->
table. All undefined flag bits must be zero. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Validation and Conflict Policy <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The publisher currently: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- Logs, asserts in debug builds, and skips HOBs with an unexpected payload <!-- [CODE_FIRST] 13253 -->
  size or invalid entry. <!-- [CODE_FIRST] 13253 -->
- Requires base addresses and sizes to be aligned to 4 KiB. <!-- [CODE_FIRST] 13253 -->
- Rejects zero-sized ranges, physical-address arithmetic overflow, and ranges <!-- [CODE_FIRST] 13253 -->
  beyond the address width reported by the CPU HOB. <!-- [CODE_FIRST] 13253 -->
- Does not start if the CPU HOB is missing or reports an invalid physical <!-- [CODE_FIRST] 13253 -->
  address width. <!-- [CODE_FIRST] 13253 -->
- Rejects unknown, maximum, and out-of-range categories. <!-- [CODE_FIRST] 13253 -->
- Rejects undefined flag bits. <!-- [CODE_FIRST] 13253 -->
- Rejects labels that are not null-terminated within the fixed label field. <!-- [CODE_FIRST] 13253 -->
- Rejects all overlaps, including exact duplicates. <!-- [CODE_FIRST] 13253 -->
- Applies overlap validation to actual addresses before redacting hidden <!-- [CODE_FIRST] 13253 -->
  addresses. <!-- [CODE_FIRST] 13253 -->
- Rejects registration after finalization. <!-- [CODE_FIRST] 13253 -->
- Limits the table to 64 entries. <!-- [CODE_FIRST] 13253 -->
- Continues publishing valid entries after rejecting an invalid registration. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Revision 1 producers and consumers must follow these policies to ensure <!-- [CODE_FIRST] 13253 -->
consistent validation and publication behavior. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Security and Privacy <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The table exposes physical addresses, sizes, categories, and labels to software <!-- [CODE_FIRST] 13253 -->
that can retrieve firmware tables. Producers must not include secrets, product <!-- [CODE_FIRST] 13253 -->
codenames, unique device information, or memory contents in a label. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Only authoritative firmware producers should register entries. Consumers must <!-- [CODE_FIRST] 13253 -->
treat the table as untrusted input and validate its signature, length, revision, <!-- [CODE_FIRST] 13253 -->
checksum, entry count, strings, and arithmetic before using it. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
RMEM is diagnostic metadata. It does not grant access to a reported range and <!-- [CODE_FIRST] 13253 -->
must not be used as the sole source for access-control or memory-ownership <!-- [CODE_FIRST] 13253 -->
decisions. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Windows PowerShell Retrieval <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Windows exposes ACPI tables to user mode through `GetSystemFirmwareTable`. The <!-- [CODE_FIRST] 13253 -->
following PowerShell example retrieves the Revision 1 `RMEM` table, validates <!-- [CODE_FIRST] 13253 -->
its contents, compares its total with Windows' hardware-reserved memory, and <!-- [CODE_FIRST] 13253 -->
prints each decoded entry. The hardware-reserved value is calculated from the <!-- [CODE_FIRST] 13253 -->
Windows-reported physically installed memory minus the physical memory available <!-- [CODE_FIRST] 13253 -->
to the operating system. The script reads only system accounting and table <!-- [CODE_FIRST] 13253 -->
metadata and does not access the reported physical ranges. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
```powershell <!-- [CODE_FIRST] 13253 -->
$ErrorActionPreference = "Stop" <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
if (-not ("RmemComparison.NativeMethods" -as [type])) { <!-- [CODE_FIRST] 13253 -->
  Add-Type -TypeDefinition @" <!-- [CODE_FIRST] 13253 -->
using System; <!-- [CODE_FIRST] 13253 -->
using System.Runtime.InteropServices; <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
namespace RmemComparison <!-- [CODE_FIRST] 13253 -->
{ <!-- [CODE_FIRST] 13253 -->
  [StructLayout(LayoutKind.Sequential)] <!-- [CODE_FIRST] 13253 -->
  public struct MemoryStatusEx <!-- [CODE_FIRST] 13253 -->
  { <!-- [CODE_FIRST] 13253 -->
    public uint Length; <!-- [CODE_FIRST] 13253 -->
    public uint MemoryLoad; <!-- [CODE_FIRST] 13253 -->
    public ulong TotalPhysical; <!-- [CODE_FIRST] 13253 -->
    public ulong AvailablePhysical; <!-- [CODE_FIRST] 13253 -->
    public ulong TotalPageFile; <!-- [CODE_FIRST] 13253 -->
    public ulong AvailablePageFile; <!-- [CODE_FIRST] 13253 -->
    public ulong TotalVirtual; <!-- [CODE_FIRST] 13253 -->
    public ulong AvailableVirtual; <!-- [CODE_FIRST] 13253 -->
    public ulong AvailableExtendedVirtual; <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  public static class NativeMethods <!-- [CODE_FIRST] 13253 -->
  { <!-- [CODE_FIRST] 13253 -->
    [DllImport("kernel32.dll", SetLastError = true)] <!-- [CODE_FIRST] 13253 -->
    [return: MarshalAs(UnmanagedType.Bool)] <!-- [CODE_FIRST] 13253 -->
    public static extern bool GetPhysicallyInstalledSystemMemory( <!-- [CODE_FIRST] 13253 -->
      out ulong totalMemoryKilobytes); <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
    [DllImport("kernel32.dll", SetLastError = true)] <!-- [CODE_FIRST] 13253 -->
    [return: MarshalAs(UnmanagedType.Bool)] <!-- [CODE_FIRST] 13253 -->
    public static extern bool GlobalMemoryStatusEx( <!-- [CODE_FIRST] 13253 -->
      ref MemoryStatusEx buffer); <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
    [DllImport("kernel32.dll", SetLastError = true)] <!-- [CODE_FIRST] 13253 -->
    public static extern uint GetSystemFirmwareTable( <!-- [CODE_FIRST] 13253 -->
      uint providerSignature, <!-- [CODE_FIRST] 13253 -->
      uint tableId, <!-- [CODE_FIRST] 13253 -->
      IntPtr buffer, <!-- [CODE_FIRST] 13253 -->
      uint bufferSize); <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
"@ <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
function ConvertTo-ProviderSignature { <!-- [CODE_FIRST] 13253 -->
  param([Parameter(Mandatory)] [string]$Text) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  $bytes = [Text.Encoding]::ASCII.GetBytes($Text) <!-- [CODE_FIRST] 13253 -->
  ([uint32]$bytes[0] -shl 24) -bor <!-- [CODE_FIRST] 13253 -->
    ([uint32]$bytes[1] -shl 16) -bor <!-- [CODE_FIRST] 13253 -->
    ([uint32]$bytes[2] -shl 8) -bor <!-- [CODE_FIRST] 13253 -->
    [uint32]$bytes[3] <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
function ConvertTo-TableId { <!-- [CODE_FIRST] 13253 -->
  param([Parameter(Mandatory)] [string]$Text) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  [BitConverter]::ToUInt32([Text.Encoding]::ASCII.GetBytes($Text), 0) <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
function Format-ByteCount { <!-- [CODE_FIRST] 13253 -->
  param([Parameter(Mandatory)] [uint64]$Bytes) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  "{0:N3} MiB (0x{1:X})" -f ($Bytes / 1MB), $Bytes <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
function Resolve-RmemCategory { <!-- [CODE_FIRST] 13253 -->
  param([Parameter(Mandatory)] [uint16]$Value) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  switch ($Value) { <!-- [CODE_FIRST] 13253 -->
    1 { "Security" } <!-- [CODE_FIRST] 13253 -->
    2 { "SharedMemory" } <!-- [CODE_FIRST] 13253 -->
    3 { "DisplayFramebuffer" } <!-- [CODE_FIRST] 13253 -->
    4 { "GpuReserved" } <!-- [CODE_FIRST] 13253 -->
    5 { "AiAcceleratorReserved" } <!-- [CODE_FIRST] 13253 -->
    6 { "FirmwareRuntime" } <!-- [CODE_FIRST] 13253 -->
    7 { "Other" } <!-- [CODE_FIRST] 13253 -->
    default { "Unknown ($Value)" } <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
function Add-UInt64Checked { <!-- [CODE_FIRST] 13253 -->
  param( <!-- [CODE_FIRST] 13253 -->
    [Parameter(Mandatory)] [uint64]$Left, <!-- [CODE_FIRST] 13253 -->
    [Parameter(Mandatory)] [uint64]$Right, <!-- [CODE_FIRST] 13253 -->
    [Parameter(Mandatory)] [string]$Description <!-- [CODE_FIRST] 13253 -->
  ) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  if ($Left -gt ([uint64]::MaxValue - $Right)) { <!-- [CODE_FIRST] 13253 -->
    throw "$Description overflows UInt64." <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  $Left + $Right <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
[uint64]$installedKilobytes = 0 <!-- [CODE_FIRST] 13253 -->
if (-not [RmemComparison.NativeMethods]::GetPhysicallyInstalledSystemMemory( <!-- [CODE_FIRST] 13253 -->
    [ref]$installedKilobytes)) { <!-- [CODE_FIRST] 13253 -->
  $errorCode = [Runtime.InteropServices.Marshal]::GetLastWin32Error() <!-- [CODE_FIRST] 13253 -->
  throw "GetPhysicallyInstalledSystemMemory failed (Win32 error $errorCode)." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
if ($installedKilobytes -gt ([uint64]::MaxValue / 1KB)) { <!-- [CODE_FIRST] 13253 -->
  throw "The installed-memory byte count overflows UInt64." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
$memoryStatus = [RmemComparison.MemoryStatusEx]::new() <!-- [CODE_FIRST] 13253 -->
$memoryStatus.Length = [Runtime.InteropServices.Marshal]::SizeOf( <!-- [CODE_FIRST] 13253 -->
  [type][RmemComparison.MemoryStatusEx]) <!-- [CODE_FIRST] 13253 -->
if (-not [RmemComparison.NativeMethods]::GlobalMemoryStatusEx( <!-- [CODE_FIRST] 13253 -->
    [ref]$memoryStatus)) { <!-- [CODE_FIRST] 13253 -->
  $errorCode = [Runtime.InteropServices.Marshal]::GetLastWin32Error() <!-- [CODE_FIRST] 13253 -->
  throw "GlobalMemoryStatusEx failed (Win32 error $errorCode)." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
[uint64]$installedBytes = $installedKilobytes * 1KB <!-- [CODE_FIRST] 13253 -->
[uint64]$windowsBytes = $memoryStatus.TotalPhysical <!-- [CODE_FIRST] 13253 -->
if ($installedBytes -lt $windowsBytes) { <!-- [CODE_FIRST] 13253 -->
  throw "Windows reports more usable memory than physically installed memory." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
[uint64]$hardwareReservedBytes = $installedBytes - $windowsBytes <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
$provider = ConvertTo-ProviderSignature "ACPI" <!-- [CODE_FIRST] 13253 -->
$tableId = ConvertTo-TableId "RMEM" <!-- [CODE_FIRST] 13253 -->
$size = [RmemComparison.NativeMethods]::GetSystemFirmwareTable( <!-- [CODE_FIRST] 13253 -->
  $provider, $tableId, [IntPtr]::Zero, 0) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
if ($size -eq 0) { <!-- [CODE_FIRST] 13253 -->
  throw "The currently booted firmware does not expose an RMEM ACPI table." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
if ($size -gt [int]::MaxValue) { <!-- [CODE_FIRST] 13253 -->
  throw "The RMEM table is too large to retrieve safely." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
$buffer = [Runtime.InteropServices.Marshal]::AllocHGlobal([int]$size) <!-- [CODE_FIRST] 13253 -->
try { <!-- [CODE_FIRST] 13253 -->
  $written = [RmemComparison.NativeMethods]::GetSystemFirmwareTable( <!-- [CODE_FIRST] 13253 -->
    $provider, $tableId, $buffer, $size) <!-- [CODE_FIRST] 13253 -->
  if ($written -eq 0) { <!-- [CODE_FIRST] 13253 -->
    $errorCode = [Runtime.InteropServices.Marshal]::GetLastWin32Error() <!-- [CODE_FIRST] 13253 -->
    throw "GetSystemFirmwareTable failed (Win32 error $errorCode)." <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  if ($written -ne $size) { <!-- [CODE_FIRST] 13253 -->
    throw "RMEM table size changed while reading: requested=$size written=$written." <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  $table = [byte[]]::new($written) <!-- [CODE_FIRST] 13253 -->
  [Runtime.InteropServices.Marshal]::Copy($buffer, $table, 0, [int]$written) <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
finally { <!-- [CODE_FIRST] 13253 -->
  [Runtime.InteropServices.Marshal]::FreeHGlobal($buffer) <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
$headerSize = 40 <!-- [CODE_FIRST] 13253 -->
$entrySize = 48 <!-- [CODE_FIRST] 13253 -->
if ($table.Length -lt $headerSize) { <!-- [CODE_FIRST] 13253 -->
  throw "RMEM table is shorter than its $headerSize-byte Revision 1 header." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
$signature = [Text.Encoding]::ASCII.GetString($table, 0, 4) <!-- [CODE_FIRST] 13253 -->
$tableLength = [BitConverter]::ToUInt32($table, 4) <!-- [CODE_FIRST] 13253 -->
$revision = $table[8] <!-- [CODE_FIRST] 13253 -->
$entryCount = [BitConverter]::ToUInt16($table, 36) <!-- [CODE_FIRST] 13253 -->
$entryOffset = [BitConverter]::ToUInt16($table, 38) <!-- [CODE_FIRST] 13253 -->
$expectedLength = [uint64]$entryOffset + ([uint64]$entryCount * $entrySize) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
if ($entryCount -gt 64) { <!-- [CODE_FIRST] 13253 -->
  throw "RMEM entry count $entryCount exceeds the Revision 1 limit of 64." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
if (($signature -ne "RMEM") -or <!-- [CODE_FIRST] 13253 -->
    ($revision -ne 1) -or <!-- [CODE_FIRST] 13253 -->
    ($entryOffset -ne $headerSize) -or <!-- [CODE_FIRST] 13253 -->
    ($tableLength -ne $expectedLength) -or <!-- [CODE_FIRST] 13253 -->
    ($tableLength -ne $table.Length)) { <!-- [CODE_FIRST] 13253 -->
  throw "RMEM header is inconsistent: signature=$signature revision=$revision length=$tableLength entries=$entryCount entryOffset=$entryOffset." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
$checksum = 0 <!-- [CODE_FIRST] 13253 -->
foreach ($value in $table) { <!-- [CODE_FIRST] 13253 -->
  $checksum = ($checksum + $value) -band 0xFF <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
if ($checksum -ne 0) { <!-- [CODE_FIRST] 13253 -->
  throw "RMEM checksum is invalid." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
$entries = @(for ($index = 0; $index -lt $entryCount; $index++) { <!-- [CODE_FIRST] 13253 -->
  $offset = $entryOffset + ($index * $entrySize) <!-- [CODE_FIRST] 13253 -->
  [uint64]$base = [BitConverter]::ToUInt64($table, $offset) <!-- [CODE_FIRST] 13253 -->
  [uint64]$rangeSize = [BitConverter]::ToUInt64($table, $offset + 8) <!-- [CODE_FIRST] 13253 -->
  [uint16]$category = [BitConverter]::ToUInt16($table, $offset + 16) <!-- [CODE_FIRST] 13253 -->
  [uint16]$flags = [BitConverter]::ToUInt16($table, $offset + 18) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  if (($category -lt 1) -or ($category -gt 7)) { <!-- [CODE_FIRST] 13253 -->
    throw "RMEM entry $index contains unsupported category $category." <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  if (($flags -band 0xFFFE) -ne 0) { <!-- [CODE_FIRST] 13253 -->
    throw "RMEM entry $index contains unsupported flags 0x$($flags.ToString('X4'))." <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  $addressHidden = ($flags -band 0x01) -ne 0 <!-- [CODE_FIRST] 13253 -->
  if (($rangeSize -eq 0) -or <!-- [CODE_FIRST] 13253 -->
      (($base % 0x1000) -ne 0) -or <!-- [CODE_FIRST] 13253 -->
      (($rangeSize % 0x1000) -ne 0) -or <!-- [CODE_FIRST] 13253 -->
      ($addressHidden -and ($base -ne 0)) -or <!-- [CODE_FIRST] 13253 -->
      (-not $addressHidden -and <!-- [CODE_FIRST] 13253 -->
       ($base -gt ([uint64]::MaxValue - ($rangeSize - 1))))) { <!-- [CODE_FIRST] 13253 -->
    throw "RMEM entry $index contains an invalid physical range." <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  $labelBytes = $table[($offset + 20)..($offset + 47)] <!-- [CODE_FIRST] 13253 -->
  $terminator = [Array]::IndexOf($labelBytes, [byte]0) <!-- [CODE_FIRST] 13253 -->
  if ($terminator -lt 0) { <!-- [CODE_FIRST] 13253 -->
    throw "RMEM entry $index has no null-terminated label." <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
  [pscustomobject]@{ <!-- [CODE_FIRST] 13253 -->
    Index = $index <!-- [CODE_FIRST] 13253 -->
    Base = $base <!-- [CODE_FIRST] 13253 -->
    End = if ($addressHidden) { <!-- [CODE_FIRST] 13253 -->
      [decimal]0 <!-- [CODE_FIRST] 13253 -->
    } else { <!-- [CODE_FIRST] 13253 -->
      [decimal]$base + [decimal]$rangeSize <!-- [CODE_FIRST] 13253 -->
    } <!-- [CODE_FIRST] 13253 -->
    Size = $rangeSize <!-- [CODE_FIRST] 13253 -->
    Category = Resolve-RmemCategory $category <!-- [CODE_FIRST] 13253 -->
    Flags = $flags <!-- [CODE_FIRST] 13253 -->
    AddressHidden = $addressHidden <!-- [CODE_FIRST] 13253 -->
    Label = [Text.Encoding]::ASCII.GetString($labelBytes, 0, $terminator) <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
}) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
[uint64]$rawTotal = 0 <!-- [CODE_FIRST] 13253 -->
[uint64]$visibleTotal = 0 <!-- [CODE_FIRST] 13253 -->
[uint64]$hiddenTotal = 0 <!-- [CODE_FIRST] 13253 -->
foreach ($entry in $entries) { <!-- [CODE_FIRST] 13253 -->
  $rawTotal = Add-UInt64Checked $rawTotal $entry.Size "RMEM total" <!-- [CODE_FIRST] 13253 -->
  if ($entry.AddressHidden) { <!-- [CODE_FIRST] 13253 -->
    $hiddenTotal = Add-UInt64Checked ` <!-- [CODE_FIRST] 13253 -->
      $hiddenTotal $entry.Size "RMEM hidden total" <!-- [CODE_FIRST] 13253 -->
  } else { <!-- [CODE_FIRST] 13253 -->
    $visibleTotal = Add-UInt64Checked ` <!-- [CODE_FIRST] 13253 -->
      $visibleTotal $entry.Size "RMEM visible total" <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
$visibleEntries = @( <!-- [CODE_FIRST] 13253 -->
  $entries | <!-- [CODE_FIRST] 13253 -->
    Where-Object { -not $_.AddressHidden } | <!-- [CODE_FIRST] 13253 -->
    Sort-Object Base <!-- [CODE_FIRST] 13253 -->
) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
for ($index = 1; $index -lt $visibleEntries.Count; $index++) { <!-- [CODE_FIRST] 13253 -->
  $previous = $visibleEntries[$index - 1] <!-- [CODE_FIRST] 13253 -->
  $current = $visibleEntries[$index] <!-- [CODE_FIRST] 13253 -->
  if ([decimal]$current.Base -lt $previous.End) { <!-- [CODE_FIRST] 13253 -->
    throw "RMEM entries $($previous.Index) and $($current.Index) overlap." <!-- [CODE_FIRST] 13253 -->
  } <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
[decimal]$difference = <!-- [CODE_FIRST] 13253 -->
  [decimal]$rawTotal - [decimal]$hardwareReservedBytes <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Write-Host "Windows memory accounting" <!-- [CODE_FIRST] 13253 -->
Write-Host "  Physically installed : $(Format-ByteCount $installedBytes)" <!-- [CODE_FIRST] 13253 -->
Write-Host "  OS-usable physical   : $(Format-ByteCount $windowsBytes)" <!-- [CODE_FIRST] 13253 -->
Write-Host "  Hardware reserved    : $(Format-ByteCount $hardwareReservedBytes)" <!-- [CODE_FIRST] 13253 -->
Write-Host "" <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Write-Host "RMEM accounting" <!-- [CODE_FIRST] 13253 -->
Write-Host "  Entries              : $entryCount" <!-- [CODE_FIRST] 13253 -->
Write-Host "  Visible entry total  : $(Format-ByteCount $visibleTotal)" <!-- [CODE_FIRST] 13253 -->
Write-Host "  Hidden entry total   : $(Format-ByteCount $hiddenTotal)" <!-- [CODE_FIRST] 13253 -->
Write-Host "  RMEM total           : $(Format-ByteCount $rawTotal)" <!-- [CODE_FIRST] 13253 -->
Write-Host ("  RMEM - Windows       : {0:N3} MiB" -f ($difference / 1MB)) <!-- [CODE_FIRST] 13253 -->
Write-Host "" <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
if ($hiddenTotal -ne 0) { <!-- [CODE_FIRST] 13253 -->
  Write-Warning "Hidden entries are included by size, but their overlap cannot be independently checked." <!-- [CODE_FIRST] 13253 -->
} <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Write-Warning "A nonzero RMEM - Windows difference may indicate missing RMEM coverage or a difference in reporting scope." <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Write-Host "" <!-- [CODE_FIRST] 13253 -->
Write-Host "RMEM totals by category" <!-- [CODE_FIRST] 13253 -->
$entries | <!-- [CODE_FIRST] 13253 -->
  Group-Object Category | <!-- [CODE_FIRST] 13253 -->
  ForEach-Object { <!-- [CODE_FIRST] 13253 -->
    [uint64]$categoryTotal = 0 <!-- [CODE_FIRST] 13253 -->
    foreach ($entry in $_.Group) { <!-- [CODE_FIRST] 13253 -->
      $categoryTotal = Add-UInt64Checked ` <!-- [CODE_FIRST] 13253 -->
        $categoryTotal $entry.Size "RMEM category total" <!-- [CODE_FIRST] 13253 -->
    } <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
    [pscustomobject]@{ <!-- [CODE_FIRST] 13253 -->
      Category = $_.Name <!-- [CODE_FIRST] 13253 -->
      Entries = $_.Count <!-- [CODE_FIRST] 13253 -->
      TotalMiB = [Math]::Round($categoryTotal / 1MB, 3) <!-- [CODE_FIRST] 13253 -->
    } <!-- [CODE_FIRST] 13253 -->
  } | <!-- [CODE_FIRST] 13253 -->
  Format-Table -AutoSize <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Write-Host "RMEM entries" <!-- [CODE_FIRST] 13253 -->
$entries | <!-- [CODE_FIRST] 13253 -->
  Select-Object Index, <!-- [CODE_FIRST] 13253 -->
    @{Name = "Base"; Expression = { <!-- [CODE_FIRST] 13253 -->
      if ($_.AddressHidden) { "<hidden>" } else { "0x{0:X16}" -f $_.Base } <!-- [CODE_FIRST] 13253 -->
    }}, <!-- [CODE_FIRST] 13253 -->
    @{Name = "SizeMiB"; Expression = { [Math]::Round($_.Size / 1MB, 3) }}, <!-- [CODE_FIRST] 13253 -->
    Category, <!-- [CODE_FIRST] 13253 -->
    @{Name = "Flags"; Expression = { "0x{0:X4}" -f $_.Flags }}, <!-- [CODE_FIRST] 13253 -->
    Label | <!-- [CODE_FIRST] 13253 -->
  Format-Table -AutoSize <!-- [CODE_FIRST] 13253 -->
``` <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
### Expected PowerShell Output <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The values below are illustrative. Actual addresses, sizes, categories, and <!-- [CODE_FIRST] 13253 -->
labels depend on the platform firmware and boot configuration. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
```text <!-- [CODE_FIRST] 13253 -->
Windows memory accounting <!-- [CODE_FIRST] 13253 -->
  Physically installed : 8,192.000 MiB (0x200000000) <!-- [CODE_FIRST] 13253 -->
  OS-usable physical   : 7,408.000 MiB (0x1CF000000) <!-- [CODE_FIRST] 13253 -->
  Hardware reserved    : 784.000 MiB (0x31000000) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
RMEM accounting <!-- [CODE_FIRST] 13253 -->
  Entries              : 4 <!-- [CODE_FIRST] 13253 -->
  Visible entry total  : 529.000 MiB (0x21100000) <!-- [CODE_FIRST] 13253 -->
  Hidden entry total   : 255.000 MiB (0xFF00000) <!-- [CODE_FIRST] 13253 -->
  RMEM total           : 784.000 MiB (0x31000000) <!-- [CODE_FIRST] 13253 -->
  RMEM - Windows       : 0.000 MiB <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
RMEM totals by category <!-- [CODE_FIRST] 13253 -->
Category        Entries TotalMiB <!-- [CODE_FIRST] 13253 -->
--------        ------- -------- <!-- [CODE_FIRST] 13253 -->
GpuReserved           1      512 <!-- [CODE_FIRST] 13253 -->
Security              1      255 <!-- [CODE_FIRST] 13253 -->
SharedMemory          1        1 <!-- [CODE_FIRST] 13253 -->
FirmwareRuntime       1       16 <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
RMEM entries <!-- [CODE_FIRST] 13253 -->
Index Base               SizeMiB Category        Flags Label <!-- [CODE_FIRST] 13253 -->
----- ----               ------- --------        ----- ----- <!-- [CODE_FIRST] 13253 -->
  0 0x0000000010000000 512.000 GpuReserved     0x0000 iGPU Shared VRAM <!-- [CODE_FIRST] 13253 -->
  1 <hidden>            255.000 Security        0x0001 Security Processor <!-- [CODE_FIRST] 13253 -->
  2 0x000000003FF00000   1.000 SharedMemory    0x0000 MM Communication Buffer <!-- [CODE_FIRST] 13253 -->
  3 0x0000000040000000  16.000 FirmwareRuntime 0x0000 Offline Crash Dump <!-- [CODE_FIRST] 13253 -->
``` <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
If the currently booted firmware does not publish RMEM, the script terminates <!-- [CODE_FIRST] 13253 -->
with an error similar to: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
```text <!-- [CODE_FIRST] 13253 -->
The currently booted firmware does not expose an RMEM ACPI table. <!-- [CODE_FIRST] 13253 -->
``` <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Run the script from an ordinary PowerShell session after booting firmware that <!-- [CODE_FIRST] 13253 -->
publishes RMEM. If the table is absent, the script reports that the current <!-- [CODE_FIRST] 13253 -->
firmware does not expose it. Addresses, sizes, categories, and labels are <!-- [CODE_FIRST] 13253 -->
platform-specific diagnostic output. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Platform Integration <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
To evaluate RMEM on a platform: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
1. Add `MdeModulePkg/Universal/Acpi/RmemAcpiDxe/RmemAcpiDxe.inf` to the platform <!-- [CODE_FIRST] 13253 -->
   DSC and firmware-volume FDF. <!-- [CODE_FIRST] 13253 -->
2. Ensure `EFI_ACPI_TABLE_PROTOCOL` is available when the driver dispatches. <!-- [CODE_FIRST] 13253 -->
3. Create RMEM GUID HOBs for authoritative static reservations, register DXE <!-- [CODE_FIRST] 13253 -->
   reservations through the protocol, or use both paths. <!-- [CODE_FIRST] 13253 -->
4. Assign each reservation to one producer and avoid conflicting ranges. <!-- [CODE_FIRST] 13253 -->
5. Retrieve the resulting table through the operating system's supported ACPI <!-- [CODE_FIRST] 13253 -->
   table interface and validate it before decoding entries. <!-- [CODE_FIRST] 13253 -->
