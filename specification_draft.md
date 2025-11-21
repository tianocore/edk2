# Title: Add DXE_SERVICES.ADD_MEMORY_SPACEV2

## Status: Draft

## Document: UEFI Platform Initialization Specification Version 1.9

## License

SPDX-License-Identifier: CC-BY-4.0

## Submitter: [TianoCore Community](https://www.tianocore.org)

## Summary of the change

This change adds the AddMemorySpaceV2 function to DXE Services. This function behaves exactly the same as
DXE_SERVICES.ADD_MEMORY_SPACE except that it takes an additional parameter, Attributes, which are the memory
attributes to be set on that range. This is equivalent to calling DXE_SERVICES.ADD_MEMORY_SPACE and then
DXE_SERVICES.SET_MEMORY_SPACE_ATTRIBUTES. It follows in the spirit of other proposed PI spec changes that are
merging adding memory ranges with setting attributes on them.

## Benefits of the change

Callers of DXE_SERVICES.ADD_MEMORY_SPACEV2 can call a single API to add a memory region to the Global Coherency
Database (GDC) with the needed attributes. Today, callers must call two APIs, leaving the possibility for the attribute
setting to fail and leaving the memory in an invalid state, e.g. if MMIO is being added, it must be marked
EFI_MEMORY_UC. Today it must temporarily be in the invalid state where EFI_MEMORY_UC is not set until a separate call
is made. By having this one API, the system can ensure memory is always in valid states. If the attribute setting was
to fail on DXE_SERVICES.ADD_MEMORY_SPACEV2, this API can remove the memory space and leave the system in a well known
state. This otherwise falls on the caller to ensure with the original API.

## Impact of the change

The impact of this change is small. A new function is added to DXE_SERVICES. Callers may use this function or may use
the existing DXE_SERVICES.ADD_MEMORY_SPACE function in combination with DXE_SERVICES.SET_MEMORY_SPACE_ATTRIBUTES.
Existing callers do not need to change.

## Detailed description of the change [normative updates]

In PI spec v1.9 section 7.2.4 a new subsection would be added below:

7.2.4.x AddMemorySpaceV2()

Summary

This service adds reserved memory, system memory, or memory-mapped I/O resources to the global coherency domain of the processor
and sets the specified memory attributes on the range.

Prototype

typedef
EFI_STATUS
(EFIAPI *EFI_ADD_MEMORY_SPACEV2) (
  IN EFI_GCD_MEMORY_TYPE    GcdMemoryType,
  IN EFI_PHYSICAL_ADDRESS   BaseAddress,
  IN UINT64                 Length,
  IN UINT64                 Capabilities,
  IN UINT64                 Attributes
  );

Parameters

GcdMemoryType
The type of memory resource being added. Type EFI_GCD_MEMORY_TYPE is defined in the AddMemorySpace() function description. The only types allowed are EfiGcdMemoryTypeReserved , EfiGcdMemoryTypeSystemMemory , EfiGcdMemoryTypePersistent , EfiGcdMemoryTypeMoreReliable , EfiGcdMemoryTypeUnaccepted, and EfiGcdMemoryTypeMemoryMappedIo .

BaseAddress
The physical address that is the start address of the memory resource being added. Type EFI_PHYSICAL_ADDRESS is defined in the AllocatePages() function description in the UEFI 2.0 specification.

Length
The size, in bytes, of the memory resource that is being added.

Capabilities
The bit mask of attributes that the memory resource region supports. The bit mask of available attributes is defined in the GetMemoryMap() function description in the UEFI 2.0 specification.

Description

The AddMemorySpaceV2() function converts unallocated non-existent memory ranges to a range of reserved memory, a range of system memory, or a range of memory mapped I/O. BaseAddress and Length specify the memory range, and GcdMemoryType specifies the memory type. The bit mask of all supported attributes for the memory range being added is specified by Capabilities . If the memory range is successfully added, then EFI_SUCCESS is returned.

If the memory range specified by BaseAddress and Length is of type EfiGcdMemoryTypeSystemMemory or EfiGcdMemoryTypeMoreReliable , then the memory range may be automatically allocated for use by the UEFI memory services. If the addition of the memory range specified by BaseAddress and Length results in a GCD memory space map containing one or more 4 KiB regions of unallocated EfiGcdMemoryTypeSystemMemory or EfiGcdMemoryTypeMoreReliable aligned on 4 KiB boundaries, then those regions will always be converted to ranges of allocated EfiGcdMemoryTypeSystemMemory or EfiGcdMemoryTypeMoreReliable respectively. This extra conversion will never be performed for fragments of memory that do not meet the above criteria.

If the GCD memory space map contains adjacent memory regions that only differ in their base address and length fields, then those adjacent memory regions must be merged into a single memory descriptor.

If Length is zero, then EFI_INVALID_PARAMETER is returned.

If GcdMemoryType is not EfiGcdMemoryTypeReserved , EfiGcdMemoryTypeSystemMemory , EfiGcdMemoryTypeMemoryMappedIo , EfiGcdMemoryPersistent , EfiGcdMemoryTypeMoreReliable , or EfiGcdMemoryTypeUnaccepted then EFI_INVALID_PARAMETER is returned.

If the processor does not support one or more bytes of the memory range specified by BaseAddress and Length , then EFI_UNSUPPORTED is returned.

If any portion of the memory range specified by BaseAddress and Length is not of type EfiGcdMemoryTypeNonExistent , then EFI_ACCESS_DENIED is returned.

If any portion of the memory range specified by BaseAddress and Length was allocated in a prior call to AllocateMemorySpace() , then EFI_ACCESS_DENIED is returned.

If there are not enough system resources available to add the memory resource to the global coherency domain of the processor, then EFI_OUT_OF_RESOURCES is returned.

If the bit mask of attributes is not supported for the memory resource range specified by BaseAddress and Length, then EFI_UNSUPPORTED
is returned.

If the attributes for the memory resource range specified by BaseAddress and Length cannot be modified, then EFI_ACCESS_DENIED is returned.
