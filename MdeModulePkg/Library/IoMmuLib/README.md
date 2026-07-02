# IoMmuLib

IoMmuLib is a library that provides a simplified interface for IOMMU (Input/Output Memory Management Unit) operations.
This library abstracts the complexity of directly interfacing with the IOMMU protocol.
It eliminates the need for PCD (Platform Configuration Database) dependencies and removes the requirement for manual
protocol location within driver code.

## Overview

The library wraps the EDK II IOMMU protocol dependency (depex) and provides a clean,
consistent API for IOMMU operations across different platforms. By using IoMmuLib,
drivers no longer need to:

- Manually locate the gEdkiiIoMmuProtocolGuid IOMMU protocol using `gBS->LocateProtocol()`
- Handle PCD configurations for IOMMU settings
- Manage protocol availability checks

## Architecture

IoMmuLib consists of two implementation files:

- **IoMmuLib.c**: The functional implementation that interfaces with the actual IOMMU protocol. IoMmuIsPresent == TRUE.
- **IoMmuLibNull.c**: A null implementation that returns `EFI_SUCCESS` for all operations. IoMmuIsPresent == FALSE.

The appropriate implementation is selected during the build process based on platform requirements.
Callers of the IoMmuLib library functions can handle for if the IoMmu is present with the IoMmuisPresent() function.

## Functions

### IoMmuIsPresent

```c
BOOLEAN
EFIAPI
IoMmuIsPresent (
  VOID
  );
```

Returns True if IoMmu is found, False otherwise.

### IoMmuMap

```c
EFI_STATUS
EFIAPI
IoMmuMap (
  IN     EDKII_IOMMU_OPERATION  Operation,
  IN     VOID                   *HostAddress,
  IN OUT UINTN                  *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS   *DeviceAddress,
  OUT    VOID                   **MappingInfo
  );
```

Maps a host address to a device address using the IOMMU page table. Currently supports identity mapping operations.

**Parameters:**

- `Operation`: The type of IOMMU operation to perform
- `HostAddress`: The host memory address to map
- `NumberOfBytes`: Input - bytes to map; Output - bytes actually mapped
- `DeviceAddress`: Returns the resulting device-accessible address
- `MappingInfo`: Returns a pointer for the mapping (used by unmap and set attribute operations)

**Returns:**

- `EFI_SUCCESS`: Mapping completed successfully
- `EFI_NOT_READY`: The IoMmu protocol is not ready.
- Other status codes as defined by the underlying IOMMU protocol

### IoMmuUnmap

```c
EFI_STATUS
EFIAPI
IoMmuUnmap (
  IN  VOID  *MappingInfo
  );
```

Unmaps a previously mapped device address and invalidates the corresponding TLB entries.

**Parameters:**

- `MappingInfo`: The mapping info returned by `IoMmuMap()`

**Returns:**

- `EFI_SUCCESS`: Unmapping completed successfully
- `EFI_NOT_READY`: The IoMmu protocol is not ready.
- Other status codes as defined by the underlying IOMMU protocol

### IoMmuAllocateBuffer

```c
EFI_STATUS
EFIAPI
IoMmuAllocateBuffer (
  IN     EFI_ALLOCATE_TYPE  Type,
  IN     EFI_MEMORY_TYPE    MemoryType,
  IN     UINTN              Pages,
  IN OUT VOID               **HostAddress,
  IN     UINT64             Attributes
  );
```

Allocates a buffer suitable for IOMMU operations with specified memory attributes.

**Parameters:**

- `Type`: The allocation type (AllocateAnyPages, AllocateMaxAddress, AllocateAddress)
- `MemoryType`: The type of memory to allocate
- `Pages`: Number of 4KB pages to allocate
- `HostAddress`: Input - desired address (if Type is AllocateAddress); Output - allocated address
- `Attributes`: Memory attributes for the allocation

**Returns:**

- `EFI_SUCCESS`: Buffer allocated successfully
- `EFI_NOT_READY`: The IoMmu protocol is not ready.
- Other status codes as defined by the underlying IOMMU protocol

### IoMmuFreeBuffer

```c
EFI_STATUS
EFIAPI
IoMmuFreeBuffer (
  IN  UINTN  Pages,
  IN  VOID   *HostAddress
  );
```

Frees a buffer that was previously allocated by `IoMmuAllocateBuffer()`.

**Parameters:**

- `Pages`: Number of pages to free (must match the original allocation)
- `HostAddress`: The host address to free

**Returns:**

- `EFI_SUCCESS`: Buffer freed successfully
- `EFI_NOT_READY`: The IoMmu protocol is not ready.
- Other status codes as defined by the underlying IOMMU protocol

### IoMmuSetAttribute

```c
EFI_STATUS
EFIAPI
IoMmuSetAttribute (
  IN EFI_HANDLE  DeviceHandle,
  IN VOID        *MappingInfo,
  IN UINT64      IoMmuAccess
  );
```

Sets read/write access attributes for a mapped region in the IOMMU page table.

**Parameters:**

- `DeviceHandle`: The device handle associated with the mapping
- `MappingInfo`: The mapping handle returned by `IoMmuMap()`
- `IoMmuAccess`: The desired IOMMU access attributes

**Returns:**

- `EFI_SUCCESS`: Attributes set successfully
- `EFI_NOT_READY`: The IoMmu protocol is not ready.
- Other status codes as defined by the underlying IOMMU protocol

### IoMmuSetAttributeById

```c
EFI_STATUS
EFIAPI
IoMmuSetAttributeById (
  IN UINT64  IommuBase,
  IN UINT32  DmaId,
  IN VOID    *MappingInfo,
  IN UINT64  IoMmuAccess
  );
```

Sets read/write access attributes for a mapped region in the IOMMU page table by explicitly
specifying the `(IommuBase, DmaId)` pair instead of an `EFI_HANDLE`.

This function is intended for firmware-internal DMA agents that have no UEFI device handle and
therefore cannot be resolved through the IORT (I/O Remapping Table). In such cases the caller is
responsible for providing the correct `IommuBase` and `DmaId`.

**Parameters:**

- `IommuBase`: Base MMIO address of the IOMMU that owns `DmaId`
- `DmaId`: The DMA identifier emitted by the calling DMA agent (e.g. StreamID on Arm SMMU, RequesterID on VT-d)
- `MappingInfo`: The mapping handle returned by `IoMmuMap()`
- `IoMmuAccess`: The desired IOMMU access attributes

**Returns:**

- `EFI_SUCCESS`: Attributes set successfully
- `EFI_NOT_READY`: The IoMmu protocol is not ready.
- `EFI_UNSUPPORTED`: The underlying IOMMU protocol does not implement `SetAttributeById`. This occurs when
  the producer's `EDKII_IOMMU_PROTOCOL` revision is older than `EDKII_IOMMU_PROTOCOL_REVISION` or does not
  provide a `SetAttributeById` function. The rest of the library remains fully functional.
- Other status codes as defined by the underlying IOMMU protocol

#### IoMmuSetAttribute vs IoMmuSetAttributeById

Use `IoMmuSetAttribute` when the DMA agent has a UEFI device handle (`EFI_HANDLE`) that the IOMMU
producer can resolve through the IORT to determine the owning IOMMU and DMA identifier. This is the
common case for PCIe devices and other handle-backed devices.

Use `IoMmuSetAttributeById` instead of `IoMmuSetAttribute` when the calling DMA agent has **no** UEFI
device handle and therefore cannot be resolved via the IORT. Because the caller supplies the
`IommuBase` and `DmaId` directly, the IOMMU producer does not need to look up the device handle. The
caller is responsible for providing the correct values.

> Note: `IoMmuSetAttributeById` requires an IOMMU producer whose protocol revision is at least
> `EDKII_IOMMU_PROTOCOL_REVISION` and implements `SetAttributeById`. If this is not the case, the
> function returns `EFI_UNSUPPORTED`; callers should fall back to `IoMmuSetAttribute` where possible.

## Usage Guidelines

### Example Usage Pattern

Many other usage cases could occur and the caller must handle the return status appropriatley.
Caller must handle for cases where IOMMU is available/not available with proper error handling.

```c
// Initialize all inputs to the IoMmuLib functions appropriatley. The below is just an example.
EFI_STATUS Status;
VOID *Mapping;
EFI_PHYSICAL_ADDRESS DeviceAddress;
UINTN NumberOfBytes;
UINT64 IoMmuAccess;

// Map host buffer for device access
Status = IoMmuMap (
           EdkiiIoMmuOperationBusMasterRead,
           HostBuffer,
           &NumberOfBytes,
           &DeviceAddress,
           &Mapping
           );

if (EFI_ERROR (Status)) {
  DEBUG ((DEBUG_ERROR, "%a - IoMmuMap failed.\n", __func__));
  ASSERT (FALSE);
}

Status = IoMmuSetAttribute (
            NULL,
            Mapping,
            IoMmuAccess
            );

// Perform device operation using DeviceAddress

// Unmap when done
Status = IoMmuUnmap (Mapping);

```

## Platform Integration

The library automatically adapts to platform capabilities. On platforms without IOMMU support, use IoMmuLibNull.
On platforms with IOMMU support, use IoMmuLib.

For integrating with a platform, in the top level DSC, you can do the following for example:

```text
  # Enable IoMmu/Smmu support
  DEFINE REQUIRE_IOMMU             = TRUE

!if $(REQUIRE_IOMMU) == TRUE
  IoMmuLib|MdeModulePkg/Library/IoMmuLib/IoMmuLib.inf
!else
  IoMmuLib|MdeModulePkg/Library/IoMmuLibNull/IoMmuLibNull.inf
!endif
```

### Dispatch ordering (Depex)

- AARCH64: `IoMmuLib.inf` has an AARCH64-only depex on `gEdkiiIoMmuProtocolGuid` to force
the IOMMU protocol to be published before any IoMmuLib consumer dispatches.

- x86: Cannot use the same depex. `IntelVTdDxe` itself depends on the PciIo
protocol, so a depex from IoMmuLib would create a circular dispatch
dependency. X86 instead relies on `RegisterProtocolNotify` and
`IoMmuIsPresent()` to discover the producer at runtime.
