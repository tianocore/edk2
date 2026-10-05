# Title: Reserved-Memory Reporting (RMEM) ACPI Table <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Status: Draft <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Document: ACPI Specification Version 6.6 <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## License <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
SPDX-License-Identifier: CC-BY-4.0 <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Submitter: [TianoCore Community](https://www.tianocore.org) <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Summary of the change <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
This change defines the Reserved-Memory Reporting (RMEM) system description <!-- [CODE_FIRST] 13253 -->
table. RMEM provides diagnostic metadata for authoritative physical-memory <!-- [CODE_FIRST] 13253 -->
reservations established by platform firmware. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Each RMEM entry identifies the reservation size, its purpose category, <!-- [CODE_FIRST] 13253 -->
attributes, and an optional diagnostic label. An entry normally identifies the <!-- [CODE_FIRST] 13253 -->
physical base address. Firmware may intentionally redact the address by setting <!-- [CODE_FIRST] 13253 -->
the Address Hidden flag and writing zero to the serialized base-address field. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The table does not reserve memory, modify the system memory map, define access <!-- [CODE_FIRST] 13253 -->
permissions, transfer ownership, or grant an operating system access to a <!-- [CODE_FIRST] 13253 -->
reported range. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Benefits of the change <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Operating systems commonly expose the difference between physically installed <!-- [CODE_FIRST] 13253 -->
memory and operating-system-available memory as a single hardware-reserved <!-- [CODE_FIRST] 13253 -->
amount. Existing interfaces do not provide a uniform explanation of the <!-- [CODE_FIRST] 13253 -->
individual firmware reservations contributing to that amount. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
RMEM enables firmware and operating-system diagnostic tools to: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- Attribute reserved memory to broad purpose categories. <!-- [CODE_FIRST] 13253 -->
- Diagnose unexpectedly large reservations. <!-- [CODE_FIRST] 13253 -->
- Compare reservation behavior across firmware configurations. <!-- [CODE_FIRST] 13253 -->
- Report reservations without requiring a vendor-specific kernel-mode or <!-- [CODE_FIRST] 13253 -->
  user-mode driver. <!-- [CODE_FIRST] 13253 -->
- Redact sensitive physical addresses while retaining useful size and purpose <!-- [CODE_FIRST] 13253 -->
  information. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Impact of the change <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Firmware that implements RMEM constructs and publishes one RMEM table using an <!-- [CODE_FIRST] 13253 -->
authoritative source for each reported reservation. A gap in a UEFI memory map <!-- [CODE_FIRST] 13253 -->
or another address-space description is not, by itself, sufficient evidence <!-- [CODE_FIRST] 13253 -->
that a range is reserved physical memory. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Operating systems and diagnostic applications may retrieve and validate the <!-- [CODE_FIRST] 13253 -->
table. Consumers must treat all table contents as untrusted input and validate <!-- [CODE_FIRST] 13253 -->
the standard ACPI header, table length, revision, checksum, entry count, entry <!-- [CODE_FIRST] 13253 -->
offset, flag values, category values, strings, and address arithmetic before <!-- [CODE_FIRST] 13253 -->
using an entry. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
RMEM is additive. Firmware that does not implement RMEM is unaffected. <!-- [CODE_FIRST] 13253 -->
Consumers must not require the table to be present. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
RMEM Revision 1 is already implemented and supported by Windows, and tooling <!-- [CODE_FIRST] 13253 -->
exists that consumes and validates the current table structure. The Project Mu <!-- [CODE_FIRST] 13253 -->
implementation provides a corresponding firmware publisher. This change <!-- [CODE_FIRST] 13253 -->
standardizes the deployed Revision 1 wire format. Backward compatibility with <!-- [CODE_FIRST] 13253 -->
existing firmware implementations and consumers is therefore a primary <!-- [CODE_FIRST] 13253 -->
requirement. A change to the serialized format should be made only when needed <!-- [CODE_FIRST] 13253 -->
to resolve a material technical or specification issue. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
RMEM does not replace: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- The UEFI memory map. <!-- [CODE_FIRST] 13253 -->
- ACPI resource descriptions. <!-- [CODE_FIRST] 13253 -->
- Architecture-specific reservation mechanisms. <!-- [CODE_FIRST] 13253 -->
- Platform security or memory-ownership policy. <!-- [CODE_FIRST] 13253 -->
- Existing mechanisms that make a reservation inaccessible to the operating <!-- [CODE_FIRST] 13253 -->
  system. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The table exposes reservation sizes, categories, labels, and, unless redacted, <!-- [CODE_FIRST] 13253 -->
physical addresses. Firmware must not place secrets, product codenames, unique <!-- [CODE_FIRST] 13253 -->
device information, memory contents, or other sensitive information in a <!-- [CODE_FIRST] 13253 -->
label. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Detailed description of the change [normative updates] <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
### Reserved-Memory Reporting table <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The Reserved-Memory Reporting table provides diagnostic descriptions of <!-- [CODE_FIRST] 13253 -->
physical-memory ranges reserved by platform firmware. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The table signature is `RMEM`. Its initial revision is 1. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Firmware must not publish an RMEM entry unless the corresponding range is <!-- [CODE_FIRST] 13253 -->
reserved by an authoritative platform mechanism. Publishing an entry does not <!-- [CODE_FIRST] 13253 -->
itself reserve the range. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
An RMEM table consists of the standard ACPI DESCRIPTION_HEADER, an RMEM table <!-- [CODE_FIRST] 13253 -->
header extension, and a packed array of fixed-size RMEM entries. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
#### RMEM table layout <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- **Offset 0, size 36, Header:** Standard ACPI `DESCRIPTION_HEADER`. <!-- [CODE_FIRST] 13253 -->
- **Offset 36, size 2, Entry Count:** Number of RMEM entries. <!-- [CODE_FIRST] 13253 -->
- **Offset 38, size 2, Entry Offset:** Offset in bytes from the start of the table to the first entry. <!-- [CODE_FIRST] 13253 -->
- **Entry Offset, size 48 * Entry Count, Entries:** Packed array of RMEM Revision 1 entries. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
For Revision 1: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- `Header.Signature` must contain `RMEM`. <!-- [CODE_FIRST] 13253 -->
- `Header.Revision` must contain 1. <!-- [CODE_FIRST] 13253 -->
- `Header.Length` must equal `Entry Offset + (48 * Entry Count)`. <!-- [CODE_FIRST] 13253 -->
- `Header.Checksum` must make the unsigned byte sum of the complete table zero. <!-- [CODE_FIRST] 13253 -->
- `Entry Offset` must contain 40. <!-- [CODE_FIRST] 13253 -->
- `Entry Count` must not exceed 64. <!-- [CODE_FIRST] 13253 -->
- The first entry begins at an 8-byte-aligned offset from the start of the <!-- [CODE_FIRST] 13253 -->
  table. <!-- [CODE_FIRST] 13253 -->
- Firmware should omit the RMEM table when there are no reservations to report. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The value 64 is included here to match the Revision 1 reference implementation. <!-- [CODE_FIRST] 13253 -->
The working group must determine whether this limit is normative or should be <!-- [CODE_FIRST] 13253 -->
removed from the specification before acceptance. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
#### RMEM Revision 1 entry <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- **Offset 0, size 8, Base:** Physical address of the first byte of the range, or zero when Address Hidden is set. <!-- [CODE_FIRST] 13253 -->
- **Offset 8, size 8, Size:** Length of the range in bytes. <!-- [CODE_FIRST] 13253 -->
- **Offset 16, size 2, Category:** Purpose category. <!-- [CODE_FIRST] 13253 -->
- **Offset 18, size 2, Flags:** Entry attributes. <!-- [CODE_FIRST] 13253 -->
- **Offset 20, size 28, Label:** Null-terminated, zero-padded ASCII diagnostic label. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
All multibyte integer fields use the byte ordering defined for ACPI tables. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
`Base` and `Size` must be aligned to 4096 bytes. `Size` must be nonzero. Unless <!-- [CODE_FIRST] 13253 -->
Address Hidden is set, `Base + Size - 1` must be representable in the platform <!-- [CODE_FIRST] 13253 -->
physical-address space without integer overflow. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Firmware must validate the actual physical address even when Address Hidden is <!-- [CODE_FIRST] 13253 -->
set. No two entries may describe ranges that share a physical byte. Adjacent <!-- [CODE_FIRST] 13253 -->
ranges do not overlap. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The `Label` field is optional. An absent label is represented by 28 zero bytes. <!-- [CODE_FIRST] 13253 -->
A present label must contain an ASCII null terminator within the field. <!-- [CODE_FIRST] 13253 -->
Firmware must set every byte following the first null terminator to zero. <!-- [CODE_FIRST] 13253 -->
Consumers must not interpret the label as a stable identifier. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
#### Category values <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- **0, Unknown:** Invalid sentinel; must not appear in a valid entry. <!-- [CODE_FIRST] 13253 -->
- **1, Security:** Isolated execution, security processors, or protected services. <!-- [CODE_FIRST] 13253 -->
- **2, Shared Memory:** Memory shared between firmware execution environments or components. <!-- [CODE_FIRST] 13253 -->
- **3, Display Framebuffer:** Pre-OS or persistent display framebuffer memory. <!-- [CODE_FIRST] 13253 -->
- **4, GPU Reserved:** Memory reserved for graphics processing. <!-- [CODE_FIRST] 13253 -->
- **5, AI Accelerator Reserved:** Memory reserved for AI accelerator processing. <!-- [CODE_FIRST] 13253 -->
- **6, Firmware Runtime:** Firmware runtime data, services, or crash diagnostics. <!-- [CODE_FIRST] 13253 -->
- **7, Other:** A valid reservation that does not fit another defined category. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Firmware must not publish an entry with the Unknown or an unsupported category. <!-- [CODE_FIRST] 13253 -->
A consumer that encounters an unsupported category must reject that entry or <!-- [CODE_FIRST] 13253 -->
the complete table according to its input-validation policy; it must not infer <!-- [CODE_FIRST] 13253 -->
ownership or access policy from an unknown value. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
The working group must determine whether future category allocations require a <!-- [CODE_FIRST] 13253 -->
new RMEM table revision or may be assigned through a maintained registry. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
#### Flags <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- **Bit 0, Address Hidden:** The physical address is intentionally redacted. <!-- [CODE_FIRST] 13253 -->
- **Bits 1-15:** Must be zero. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
When Address Hidden is clear, `Base` contains the physical address of the first <!-- [CODE_FIRST] 13253 -->
byte in the reservation. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
When Address Hidden is set: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
- Firmware must write zero to the serialized `Base` field. <!-- [CODE_FIRST] 13253 -->
- Firmware must use the actual physical address when validating alignment, <!-- [CODE_FIRST] 13253 -->
  address-width limits, arithmetic, and overlap. <!-- [CODE_FIRST] 13253 -->
- Consumers must treat the address as unavailable. <!-- [CODE_FIRST] 13253 -->
- Consumers may use `Size`, `Category`, and `Label` for diagnostic reporting. <!-- [CODE_FIRST] 13253 -->
- Consumers must not infer that the actual reservation begins at address zero. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
#### Producer requirements <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Firmware producing RMEM must: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
1. Obtain each entry from an authoritative reservation source. <!-- [CODE_FIRST] 13253 -->
2. Validate each actual range before serializing the table. <!-- [CODE_FIRST] 13253 -->
3. Reject duplicate and overlapping ranges. <!-- [CODE_FIRST] 13253 -->
4. Reject unsupported categories and flags. <!-- [CODE_FIRST] 13253 -->
5. Ensure labels are null-terminated and zero-padded. <!-- [CODE_FIRST] 13253 -->
6. Redact hidden addresses only after range and overlap validation. <!-- [CODE_FIRST] 13253 -->
7. Publish at most one RMEM table. <!-- [CODE_FIRST] 13253 -->
8. Complete the table checksum before publication. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Firmware should publish the table after the authoritative entry set is <!-- [CODE_FIRST] 13253 -->
complete and before control is transferred to the operating system. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
#### Consumer requirements <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
A consumer must: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
1. Validate the standard ACPI header and checksum. <!-- [CODE_FIRST] 13253 -->
2. Validate the RMEM revision, entry offset, entry count, and total length <!-- [CODE_FIRST] 13253 -->
   before accessing an entry. <!-- [CODE_FIRST] 13253 -->
3. Use checked arithmetic for all offset and range calculations. <!-- [CODE_FIRST] 13253 -->
4. Validate category and flag values. <!-- [CODE_FIRST] 13253 -->
5. Validate label termination before decoding a label. <!-- [CODE_FIRST] 13253 -->
6. Treat Address Hidden entries as having an unavailable address. <!-- [CODE_FIRST] 13253 -->
7. Treat RMEM as diagnostic metadata rather than an access-control authority. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
A consumer must not access physical memory solely because an RMEM entry <!-- [CODE_FIRST] 13253 -->
describes it. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
#### Revision compatibility <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
A consumer that supports Revision 1 must not parse an unsupported table <!-- [CODE_FIRST] 13253 -->
revision as Revision 1. Future revisions may extend the header or entry format <!-- [CODE_FIRST] 13253 -->
by changing the revision and associated length or offset rules. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Software must use `Header.Length` and `Entry Offset` when validating the table <!-- [CODE_FIRST] 13253 -->
and must not read beyond `Header.Length`. <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
## Special Instructions <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
Before this proposal is accepted: <!-- [CODE_FIRST] 13253 -->
<!-- [CODE_FIRST] 13253 -->
1. The ACPI working group must confirm or allocate the `RMEM` signature. <!-- [CODE_FIRST] 13253 -->
2. The working group must decide whether 64 entries is a normative Revision 1 <!-- [CODE_FIRST] 13253 -->
   limit or only a reference-implementation limit. <!-- [CODE_FIRST] 13253 -->
3. The working group must decide how future category values are allocated. <!-- [CODE_FIRST] 13253 -->
4. The final specification language must be reconciled with the public EDK II <!-- [CODE_FIRST] 13253 -->
   reference implementation and its tests. <!-- [CODE_FIRST] 13253 -->
5. Review must account for the deployed Windows support and tooling that <!-- [CODE_FIRST] 13253 -->
   consume the Revision 1 wire format. Any incompatible change must identify <!-- [CODE_FIRST] 13253 -->
   the material issue it resolves and define an appropriate compatibility or <!-- [CODE_FIRST] 13253 -->
   migration strategy. <!-- [CODE_FIRST] 13253 -->
