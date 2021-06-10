# Title: Change MessageLength Field of EFI_MM_COMMUNICATE_HEADER to UINT64

## Status: Draft

## Document: UEFI Platform Initialization Specification Version 1.7 Errata A

## License

SPDX-License-Identifier: CC-BY-4.0

## Submitter: [TianoCore Community](https://www.tianocore.org)

## Summary of the change

Replace `EFI_MM_COMMUNICATE_HEADER` with `EFI_MM_COMMUNICATE_HEADER_NEW` to provide better portability between different architectures (IA32 & X64) and adapt to flexible array supported by modern compilers:

```c
typedef struct {
  UINT32    Signature;
  UINT32    Version;
  EFI_GUID  MessageGuid;
  UINT64    MessageSize;
  UINT8     MessageData[ANYSIZE_ARRAY];
} EFI_MM_COMMUNICATE_HEADER_NEW;
```

## Benefits of the change

In PI Spec v1.7 Errata A, Vol.4, Sec 5.7 MM Communication Protocol, the MessageLength field of `EFI_MM_COMMUNICATE_HEADER` (also defined as `EFI_SMM_COMMUNICATE_HEADER`) is defined as type UINTN.

But this structure, as a generic definition, could be used for both PEI and DXE MM communication. Thus for a system that supports PEI MM launch, but operates PEI in 32bit mode and MM foundation in 64bit, the current `EFI_MM_COMMUNICATE_HEADER` definition will cause structure parse error due to UINTN used.

Furthermore, the addition of signature could help identifying whether the data received is compiliant with this new data structure, which will help for binary release modules to identify usage of legacy `EFI_MM_COMMUNICATE_HEADER`.

Version field is also added to indicate the current version of header in case there is need for minor modification in the future.

Lastly, the data field of MM communicate message is replaced with flexible array to allow users not having to consume extra data during communicate and author code more intrinsically.

## Impact of the change

This change will impact the known structure consumers including:

```bash
MdeModulePkg/Core/PiSmmCore/PiSmmIpl
MdeModulePkg/Application/SmiHandlerProfileInfo
MdeModulePkg/Application/MemoryProfileInfo
MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib
```

For consumers that are not using `OFFSET_OF(EFI_MM_COMMUNICATE_HEADER, Data)`, but performing explicit addition such as the existing MdeModulePkg/Application/SmiHandlerProfileInfo/SmiHandlerProfileInfo.c, one will need to change code implementation to match new structure definition. Otherwise, the code compiled on IA32 architecture will experience structure field dereference error.

User who currently uses UINTN local variables as place holder of MessageLength will need to use caution to make cast from UINTN to UINT64 and vice versa. It is recommended to use `SafeUint64ToUintn` for such operations when the value is indeterministic.

Note: MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib is also consuming this structure, but it handled this size discrepancy internally. If this potential spec change is not applied, all applicable PEI MM communicate callers will need to use the same routine as that of SmmLockBoxPeiLib to invoke a properly populated EFI_MM_COMMUNICATE_HEADER to be used in X64 MM foundation.

## Detailed description of the change [normative updates]

### Specification Changes

1. In PI Specification v1.7 Errata A: Vol. 4 Page-91, the definition of `EFI_MM_COMMUNICATE_HEADER` should be changed from current:

```c
typedef struct {
  EFI_GUID  HeaderGuid;
  UINTN     MessageLength;
  UINT8     Data[ANYSIZE_ARRAY];
} EFI_MM_COMMUNICATE_HEADER;
```

to:

```c
typedef struct {
  UINT32    Signature;
  UINT32    Version;
  EFI_GUID  MessageGuid;
  UINT64    MessageSize;
  UINT8     MessageData[ANYSIZE_ARRAY];
} EFI_MM_COMMUNICATE_HEADER_NEW;

#define EFI_MM_COMMUNICATE_HEADER_NEW_SIGNATURE 0x4D434832 // "MCH2"
#define EFI_MM_COMMUNICATE_HEADER_NEW_VERSION   1
```

### Code Changes

1. Update data structure in `MdePkg/Include/Protocol/MmCommunication.h` to match new definition that includes the field type and name changes.

1. Replace all references to `EFI_MM_COMMUNICATE_HEADER` with `EFI_MM_COMMUNICATE_HEADER_NEW` in:

```bash
MdeModulePkg/Application/SmiHandlerProfileInfo/SmiHandlerProfileInfo.c
MdeModulePkg/Application/MemoryProfileInfo/MemoryProfileInfo.c
MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.c
```

1. In registered `EFI_MM_ENTRY_POINT`s, two check should be added when it is a synchronous MMI for MM Core or request from Communicate protocol:
    * Verify non-MM environment is compliant with new data structure by inspecting `Signature` field.
    * Verify non-MM environment is supplying data structure format that is supported by inspecting the `Version` field.

This change would cover:

```bash
StandaloneMmPkg/Core/StandaloneMmCore.c
MdeModulePkg/Core/PiSmmCore/PiSmmCore.c
```

1. Remove the explicit calculation of the offset of `Data` in `EFI_MM_COMMUNICATE_HEADER`. Thus applicable calculations of `sizeof(EFI_GUID) + sizeof(UINTN)` should be replaced with `OFFSET_OF(EFI_MM_COMMUNICATE_HEADER, Data)` or `sizeof(EFI_MM_COMMUNICATE_HEADER_NEW)`. These calculations are identified in:

```bash
MdeModulePkg/Application/SmiHandlerProfileInfo/SmiHandlerProfileInfo.c
MdeModulePkg/Application/MemoryProfileInfo/MemoryProfileInfo.c
MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.c
```

1. Resolve potentially mismatched type between `UINTN` and `UINT64`. This would occur when `MessageLength` or its derivitive are used for local calculation with existing `UINTN` typed variables. Code change regarding this perspective is per case evaluation: if the variables involved are all deterministic values, and there is no overflow or underflow risk, a cast operation (from `UINTN` to `UINT64`) can be safely used. Otherwise, the calculation will be performed in `UINT64` bitwidth and then convert to `UINTN` using `SafeUint64*` and `SafeUint64ToUintn`, respectively. These operations are identified in:

```bash
MdeModulePkg/Core/PiSmmCore/PiSmmIpl.c
MdeModulePkg/Application/SmiHandlerProfileInfo/SmiHandlerProfileInfo.c
MdeModulePkg/Application/MemoryProfileInfo/MemoryProfileInfo.c
```
