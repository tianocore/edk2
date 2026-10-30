# Title: Reserved-Memory Reporting (RMEM) ACPI Table

## Status: Draft

## Document: ACPI Specification Version 6.6

## License

SPDX-License-Identifier: CC-BY-4.0

## Submitter: [TianoCore Community](https://www.tianocore.org)

## Summary of the change

This change defines the Reserved-Memory Reporting (RMEM) system description
table. RMEM provides diagnostic metadata for authoritative physical-memory
reservations established by platform firmware.

Each RMEM entry identifies the reservation size, its purpose category,
attributes, and an optional diagnostic label. An entry normally identifies the
physical base address. Firmware may intentionally redact the address by setting
the Address Hidden flag and writing zero to the serialized base-address field.

The table does not reserve memory, modify the system memory map, define access
permissions, transfer ownership, or grant an operating system access to a
reported range.

## Benefits of the change

Operating systems commonly expose the difference between physically installed
memory and operating-system-available memory as a single hardware-reserved
amount. Existing interfaces do not provide a uniform explanation of the
individual firmware reservations contributing to that amount.

RMEM enables firmware and operating-system diagnostic tools to:

- Attribute reserved memory to broad purpose categories.
- Diagnose unexpectedly large reservations.
- Compare reservation behavior across firmware configurations.
- Report reservations without requiring a vendor-specific kernel-mode or
  user-mode driver.
- Redact sensitive physical addresses while retaining useful size and purpose
  information.

## Impact of the change

Firmware that implements RMEM constructs and publishes one RMEM table using an
authoritative source for each reported reservation. A gap in a UEFI memory map
or another address-space description is not, by itself, sufficient evidence
that a range is reserved physical memory.

Operating systems and diagnostic applications may retrieve and validate the
table. Consumers must treat all table contents as untrusted input and validate
the standard ACPI header, table length, revision, checksum, entry count, entry
offset, flag values, category values, strings, and address arithmetic before
using an entry.

RMEM is additive. Firmware that does not implement RMEM is unaffected.
Consumers must not require the table to be present.

RMEM Revision 1 is already implemented and supported by Windows, and tooling
exists that consumes and validates the current table structure. The Project Mu
implementation provides a corresponding firmware publisher. This change
standardizes the deployed Revision 1 wire format. Backward compatibility with
existing firmware implementations and consumers is therefore a primary
requirement. A change to the serialized format should be made only when needed
to resolve a material technical or specification issue.

The existing Project Mu
[Reserved-Memory Reporting through ACPI](https://github.com/microsoft/mu_basecore/blob/587366f707e8a15d4b5bec9aaea759b558486705/MdeModulePkg/Universal/Acpi/RmemAcpiDxe/ReadMe.md)
documentation describes the deployed Revision 1 behavior, validation policy,
Windows retrieval tooling, and reference firmware integration. It is the
compatibility reference for this proposal; the final ACPI specification is
normative if the documents conflict.

RMEM does not replace:

- The UEFI memory map.
- ACPI resource descriptions.
- Architecture-specific reservation mechanisms.
- Platform security or memory-ownership policy.
- Existing mechanisms that make a reservation inaccessible to the operating
  system.

The table exposes reservation sizes, categories, labels, and, unless redacted,
physical addresses. Firmware must not place secrets, product codenames, unique
device information, memory contents, or other sensitive information in a
label.

## Detailed description of the change [normative updates]

### Reserved-Memory Reporting table

The Reserved-Memory Reporting table provides diagnostic descriptions of
physical-memory ranges reserved by platform firmware.

The table signature is `RMEM`. Its initial revision is 1.

Firmware must not publish an RMEM entry unless the corresponding range is
reserved by an authoritative platform mechanism. Publishing an entry does not
itself reserve the range.

An RMEM table consists of the standard ACPI DESCRIPTION_HEADER, an RMEM table
header extension, and a packed array of fixed-size RMEM entries.

#### RMEM table layout

| Offset | Size | Field | Description |
| ---: | ---: | --- | --- |
| 0 | 36 | Header | Standard ACPI `DESCRIPTION_HEADER` |
| 36 | 2 | Entry Count | Number of RMEM entries |
| 38 | 2 | Entry Offset | Offset in bytes from the start of the table to the first entry |
| Entry Offset | 48 * Entry Count | Entries | Packed array of RMEM Revision 1 entries |

For Revision 1:

- `Header.Signature` must contain `RMEM`.
- `Header.Revision` must contain 1.
- `Header.Length` must equal `Entry Offset + (48 * Entry Count)`.
- `Header.Checksum` must make the unsigned byte sum of the complete table zero.
- `Entry Offset` must contain 40.
- `Entry Count` must not exceed 64.
- The first entry begins at an 8-byte-aligned offset from the start of the
  table.
- Firmware should omit the RMEM table when there are no reservations to report.

The value 64 is included here to match the Revision 1 reference implementation.
The working group must determine whether this limit is normative or should be
removed from the specification before acceptance.

#### RMEM Revision 1 entry

| Offset | Size | Field | Description |
| ---: | ---: | --- | --- |
| 0 | 8 | Base | Physical address of the first byte of the range, or zero when Address Hidden is set |
| 8 | 8 | Size | Length of the range in bytes |
| 16 | 2 | Category | Purpose category |
| 18 | 2 | Flags | Entry attributes |
| 20 | 28 | Label | Null-terminated, zero-padded ASCII diagnostic label |

All multibyte integer fields use the byte ordering defined for ACPI tables.

`Base` and `Size` must be aligned to 4096 bytes. `Size` must be nonzero. Unless
Address Hidden is set, `Base + Size - 1` must be representable in the platform
physical-address space without integer overflow.

Firmware must validate the actual physical address even when Address Hidden is
set. No two entries may describe ranges that share a physical byte. Adjacent
ranges do not overlap.

The `Label` field is optional. An absent label is represented by 28 zero bytes.
A present label must contain an ASCII null terminator within the field.
Firmware must set every byte following the first null terminator to zero.
Consumers must not interpret the label as a stable identifier.

#### Category values

| Value | Category | Intended use |
| ---: | --- | --- |
| 0 | Unknown | Invalid sentinel for missing or uninitialized values; must not appear in a valid entry |
| 1 | Security | Isolated execution, security processors, or protected services |
| 2 | Shared Memory | Memory shared across firmware execution environments |
| 3 | Display Framebuffer | Pre-OS or persistent display framebuffer memory |
| 4 | GPU Reserved | Memory reserved for graphics use |
| 5 | AI Accelerator Reserved | Memory reserved for AI acceleration |
| 6 | Firmware Runtime | Runtime data, services, or crash diagnostics |
| 7 | Other | A reservation that does not fit another category |

Firmware must not publish an entry with the Unknown or an unsupported category.
A consumer that encounters an unsupported category must reject that entry or
the complete table according to its input-validation policy; it must not infer
ownership or access policy from an unknown value.

The working group must determine whether future category allocations require a
new RMEM table revision or may be assigned through a maintained registry.

#### Flags

| Bits | Attribute | Description |
| ---: | --- | --- |
| 0 | Address Hidden | The physical address is intentionally redacted |
| 1-15 | Not defined | Must be zero |

When Address Hidden is clear, `Base` contains the physical address of the first
byte in the reservation.

When Address Hidden is set:

- Firmware must write zero to the serialized `Base` field.
- Firmware must use the actual physical address when validating alignment,
  address-width limits, arithmetic, and overlap.
- Consumers must treat the address as unavailable.
- Consumers may use `Size`, `Category`, and `Label` for diagnostic reporting.
- Consumers must not infer that the actual reservation begins at address zero.

#### Producer requirements

Firmware producing RMEM must:

1. Obtain each entry from an authoritative reservation source.
2. Validate each actual range before serializing the table.
3. Reject duplicate and overlapping ranges.
4. Reject unsupported categories and flags.
5. Ensure labels are null-terminated and zero-padded.
6. Redact hidden addresses only after range and overlap validation.
7. Publish at most one RMEM table.
8. Complete the table checksum before publication.

Firmware should publish the table after the authoritative entry set is
complete and before control is transferred to the operating system.

#### Consumer requirements

A consumer must:

1. Validate the standard ACPI header and checksum.
2. Validate the RMEM revision, entry offset, entry count, and total length
   before accessing an entry.
3. Use checked arithmetic for all offset and range calculations.
4. Validate category and flag values.
5. Validate label termination before decoding a label.
6. Treat Address Hidden entries as having an unavailable address.
7. Treat RMEM as diagnostic metadata rather than an access-control authority.

A consumer must not access physical memory solely because an RMEM entry
describes it.

#### Revision compatibility

A consumer that supports Revision 1 must not parse an unsupported table
revision as Revision 1. Future revisions may extend the header or entry format
by changing the revision and associated length or offset rules.

Software must use `Header.Length` and `Entry Offset` when validating the table
and must not read beyond `Header.Length`.
