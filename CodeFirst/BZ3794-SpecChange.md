# Title: Introduction of `EFI_SW_EC_FRAGMENTED_MEMORY_MAP` Status Code

## Status: Draft

## Document: UEFI Platform Initialization Specification Version 1.7 Errata A

## License

SPDX-License-Identifier: CC-BY-4.0

## Submitter: [TianoCore Community](https://www.tianocore.org)

## Summary of the change

Add `EFI_SW_EC_FRAGMENTED_MEMORY_MAP` into Status Codes definition.

## Benefits of the change

Current Status Codes covered various [software class error code definitions](https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Pi/PiStatusCode.h).

However, fragmented memory map from boot to boot would fail to meet certain OS ACPI requirements (i.e. S4 resume boot requires consistent memory maps) and trigger system reboots. Yet the corresponding case was not covered by the already defined status codes.

The unexpected system reboots above could indicate decay of system health and reporting of such generic events would provide helpful information to OEMs to investigate/prevent system failures in general.

The request of this change intends to expand definitions of `EFI_SW_EC_**` under Status Codes to cover more unexpected system reboot events, which could improve Status Code futility and readability.

## Impact of the change

Occupy a new macro definitions of Error Codes under Software class Status Codes.

## Detailed description of the change [normative updates]

### Specification Changes

1. In PI Specification v1.7 Errata A: Vol. 3, Table 3-61: Error Code Operations: Host Software Class, add one new rows below `EFI_SW_EC_FV_CORRUPTED` definition:

    | Operation | Description | Extended Data |
    | --- | --- | --- |
    | EFI_SW_EC_FRAGMENTED_MEMORY_MAP | System will reboot due to fragmented memory maps | None |

1. In PI Specification v1.7 Errata A: Vol. 3, Table 3-61: Error Code Operations: Host Software Class, replace the row of `0x0014-0x00FF` to:

    | Operation | Description | Extended Data |
    | --- | --- | --- |
    | 0x0015-0x00FF | Reserved for future use by this specification for Host Software class error codes. | None |

1. In PI Specification v1.7 Errata A: Vol. 3, Section 6.7.4.3 Error Code Definitions: Prototype, add one new definitions below `EFI_SW_EC_FV_CORRUPTED` definition:

    ```c
    #define EFI_SW_EC_FRAGMENTED_MEMORY_MAP  0x00000014
    ```

### Code Changes

1. Add macro definitions in `MdePkg/Include/Pi/PiStatusCode.h` to match new specification.
