# Title: Introduction of `EFI_PERIPHERAL_TPM` Peripheral Subclass Definition

## Status: Draft

## Document: UEFI Platform Initialization Specification Version 1.7 Errata A

## License

SPDX-License-Identifier: CC-BY-4.0

## Submitter: [TianoCore Community](https://www.tianocore.org)

## Summary of the change

Add `EFI_PERIPHERAL_TPM` into Peripheral Subclass definition.

## Benefits of the change

Current status code covered various [peripheral subclass definitions](https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Pi/PiStatusCode.h).

As Trusted Platform Module (TPM) becomes more available on the modern systems, status reports from such peripheral are playing more important roles in anaylzing the secruity state and healthiness of a system. However, peripheral subclass definitions do not cover TPM as of today.

Standardizing the TPM peripheral subclass definition could facilitate the parsing of peripheral reported errors and avoid potential definition collisions from implementation based subclass usages.

The request of this change intends to expand definitions of `EFI_PERIPHERAL_**` under Periperhal Subclass definitions to cover the TPM subclass.

## Impact of the change

Occupy a new macro definitions of subclass under `Defined Subclasses: User-Accessible Peripheral Class`.

## Detailed description of the change [normative updates]

### Specification Changes

1. In PI Specification v1.7 Errata A: Vol. 3, Table 3-30: Defined Subclasses: User-Accessible Peripheral Class, add one new rows below `EFI_PERIPHERAL_DOCKING` definition and adjust the rest of reserved definitions accordingly:

    | Subclass | Code Name | Description |
    | --- | --- | --- |
    | Trusted Platform Module | EFI_PERIPHERAL_TPM | The peripheral referred to is a Trusted Platform Module |
    | 0x0F–0x7F | Reserved for future use by this specification |  |

1. In PI Specification v1.7 Errata A: Vol. 3, Table 3-84: Defined Subclasses: User-Accessible Peripheral Class, add one new rows below `EFI_PERIPHERAL_DOCKING` definition and adjust the rest of reserved definitions accordingly:

    | Subclass | Code Name |
    | --- | --- |
    | Trusted Platform Module | EFI_PERIPHERAL_TPM |
    | 0x0F–0x7F | Reserved for future use by this specification. |

1. In PI Specification v1.7 Errata A: Vol. 3, Section 6.7.2.1 Subclass Definitions: Prototype, add one new definitions below `EFI_PERIPHERAL_DOCKING` definition:

    ```c
    #define EFI_PERIPHERAL_TPM \
      (EFI_PERIPHERAL | 0x000E0000)
    ```

### Code Changes

1. Add macro definitions in `MdePkg/Include/Pi/PiStatusCode.h` to match new specification.
1. Replace existing references of `gEfiSecurityPkgTokenSpaceGuid.PcdStatusCodeSubClassTpmDevice` from SecurityPkg with new definition.
1. Updated the default value of `gEfiSecurityPkgTokenSpaceGuid.PcdStatusCodeSubClassTpmDevice` to `(EFI_PERIPHERAL | 0x000E0000)` for consistency and backwards compatibility outside of SecurityPkg.
