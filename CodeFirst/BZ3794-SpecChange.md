# Title: Introduction of `EFI_MM_COMMUNICATE_HEADER_V3` and `MM_COMMUNICATE3_*` interface

## Status: Draft

## Document: UEFI Platform Initialization Specification Version 1.7 Errata A

## License

SPDX-License-Identifier: CC-BY-4.0

## Submitter: [TianoCore Community](https://www.tianocore.org)

## Summary of the change

Introduce `EFI_SW_EC_MEMORY_TYPE_INFORMATION_CHANGE` and `EFI_SW_EC_RELEASE_ASSERT` into Status Codes definition.

## Benefits of the change

Current Status Codes covered various [software class error code definitions](https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Pi/PiStatusCode.h).

However, there are a few critical instances where the software could trigger system reboots while the corresponding case was not covered by the already defined status codes:

1. Memory type information change triggered system reboot;
2. Assert triggered reboot on systems that did not enable system halts;

The unexpected system reboots above could indicate decay of system health and reporting of such generic events would provide helpful information to OEMs to investigate/prevent system failures in general.

The request of this change intends to expand definitions of `EFI_SW_EC_**` under Status Codes to cover more unexpected system reboot events, which could improve Status Code futility and readability.

## Impact of the change

Occupy 2 new macro definitions of Error Codes under Software class Status Codes.

## Detailed description of the change [normative updates]

### Specification Changes

1. In PI Specification v1.7 Errata A: Vol. 3, Table 3-61: Error Code Operations: Host Software Class, add 2 new rows below `EFI_SW_EC_FV_CORRUPTED` definition:

    | Operation | Description | Extended Data |
    | --- | --- | --- |
    | EFI_SW_EC_MEMORY_TYPE_INFORMATION_CHANGE | System will reboot due to memory type information changes | None |
    | EFI_SW_EC_RELEASE_ASSERT | System software asserted  | None |

1. In PI Specification v1.7 Errata A: Vol. 3, Table 3-61: Error Code Operations: Host Software Class, replace the row of `0x0014–0x00FF` to:

    | Operation | Description | Extended Data |
    | --- | --- | --- |
    | 0x0016–0x00FF | Reserved for future use by this specification for Host Software class error codes. | None |

1. In PI Specification v1.7 Errata A: Vol. 3, Section 6.7.4.3 Error Code Definitions: Prototype, add 2 new definitions below `EFI_SW_EC_FV_CORRUPTED` definition:

    ```c
    #define EFI_SW_EC_MEMORY_TYPE_INFORMATION_CHANGE  0x00000014
    #define EFI_SW_EC_RELEASE_ASSERT                  0x00000015
    ```

### Code Changes

1. Add macro definitions in `MdePkg/Include/Pi/PiStatusCode.h` to match new specification.
