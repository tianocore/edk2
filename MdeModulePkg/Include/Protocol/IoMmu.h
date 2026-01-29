/** @file
  EFI IOMMU Protocol.

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __IOMMU_H__
#define __IOMMU_H__

//
// IOMMU Protocol GUID value
//
#define EDKII_IOMMU_PROTOCOL_GUID \
    { \
      0x4e939de9, 0xd948, 0x4b0f, { 0x88, 0xed, 0xe6, 0xe1, 0xce, 0x51, 0x7c, 0x1e } \
    }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EDKII_IOMMU_PROTOCOL EDKII_IOMMU_PROTOCOL;

//
// Revision The revision to which the IOMMU interface adheres.
//          All future revisions must be backwards compatible.
//          If a future version is not back wards compatible it is not the same GUID.
//
#define EDKII_IOMMU_PROTOCOL_REVISION  0x00010000

//
// IOMMU Access for SetAttribute
//
// These types can be "ORed" together as needed.
// Any undefined bits are reserved and must be zero.
//
#define EDKII_IOMMU_ACCESS_READ   0x1
#define EDKII_IOMMU_ACCESS_WRITE  0x2

//
// IOMMU Operation for Map
//
typedef enum {
  ///
  /// A read operation from system memory by a bus master that is not capable of producing
  /// PCI dual address cycles.
  ///
  EdkiiIoMmuOperationBusMasterRead,
  ///
  /// A write operation from system memory by a bus master that is not capable of producing
  /// PCI dual address cycles.
  ///
  EdkiiIoMmuOperationBusMasterWrite,
  ///
  /// Provides both read and write access to system memory by both the processor and a bus
  /// master that is not capable of producing PCI dual address cycles.
  ///
  EdkiiIoMmuOperationBusMasterCommonBuffer,
  ///
  /// A read operation from system memory by a bus master that is capable of producing PCI
  /// dual address cycles.
  ///
  EdkiiIoMmuOperationBusMasterRead64,
  ///
  /// A write operation to system memory by a bus master that is capable of producing PCI
  /// dual address cycles.
  ///
  EdkiiIoMmuOperationBusMasterWrite64,
  ///
  /// Provides both read and write access to system memory by both the processor and a bus
  /// master that is capable of producing PCI dual address cycles.
  ///
  EdkiiIoMmuOperationBusMasterCommonBuffer64,
  EdkiiIoMmuOperationMaximum
} EDKII_IOMMU_OPERATION;

//
// IOMMU attribute for AllocateBuffer
// Any undefined bits are reserved and must be zero.
//
#define EDKII_IOMMU_ATTRIBUTE_MEMORY_WRITE_COMBINE  0x0080
#define EDKII_IOMMU_ATTRIBUTE_MEMORY_CACHED         0x0800
#define EDKII_IOMMU_ATTRIBUTE_DUAL_ADDRESS_CYCLE    0x8000

#define EDKII_IOMMU_ATTRIBUTE_VALID_FOR_ALLOCATE_BUFFER  (EDKII_IOMMU_ATTRIBUTE_MEMORY_WRITE_COMBINE | EDKII_IOMMU_ATTRIBUTE_MEMORY_CACHED | EDKII_IOMMU_ATTRIBUTE_DUAL_ADDRESS_CYCLE)

#define EDKII_IOMMU_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER  (~EDKII_IOMMU_ATTRIBUTE_VALID_FOR_ALLOCATE_BUFFER)

/**
  Set IOMMU attribute for a system memory.

  If the IOMMU protocol exists, the system memory cannot be used
  for DMA by default.

  When a device requests a DMA access for a system memory,
  the device driver need use SetAttribute() to update the IOMMU
  attribute to request DMA access (read and/or write).

  The DeviceHandle is used to identify which device submits the request.
  The IOMMU implementation need translate the device path to an IOMMU device ID,
  and set IOMMU hardware register accordingly.
  1) DeviceHandle can be a standard PCI device.
     The memory for BusMasterRead need set EDKII_IOMMU_ACCESS_READ.
     The memory for BusMasterWrite need set EDKII_IOMMU_ACCESS_WRITE.
     The memory for BusMasterCommonBuffer need set EDKII_IOMMU_ACCESS_READ|EDKII_IOMMU_ACCESS_WRITE.
     After the memory is used, the memory need set 0 to keep it being protected.
  2) DeviceHandle can be an ACPI device (ISA, I2C, SPI, etc).
     The memory for DMA access need set EDKII_IOMMU_ACCESS_READ and/or EDKII_IOMMU_ACCESS_WRITE.

  @param[in]  This              The protocol instance pointer.
  @param[in]  DeviceHandle      The device who initiates the DMA access request.
  @param[in]  Mapping           The mapping value returned from Map().
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_SUCCESS            The IoMmuAccess is set for the memory range specified by DeviceAddress and Length.
  @retval EFI_INVALID_PARAMETER  DeviceHandle is an invalid handle.
  @retval EFI_INVALID_PARAMETER  Mapping is not a value that was returned by Map().
  @retval EFI_INVALID_PARAMETER  IoMmuAccess specified an illegal combination of access.
  @retval EFI_UNSUPPORTED        DeviceHandle is unknown by the IOMMU.
  @retval EFI_UNSUPPORTED        The bit mask of IoMmuAccess is not supported by the IOMMU.
  @retval EFI_UNSUPPORTED        The IOMMU does not support the memory range specified by Mapping.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to modify the IOMMU access.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error while attempting the operation.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IOMMU_SET_ATTRIBUTE)(
  IN EDKII_IOMMU_PROTOCOL  *This,
  IN EFI_HANDLE            DeviceHandle,
  IN VOID                  *Mapping,
  IN UINT64                IoMmuAccess
  );

/**
  Provides the controller-specific addresses required to access system memory from a
  DMA bus master.

  @param  This                  The protocol instance pointer.
  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IOMMU_MAP)(
  IN     EDKII_IOMMU_PROTOCOL                       *This,
  IN     EDKII_IOMMU_OPERATION                      Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  The protocol instance pointer.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IOMMU_UNMAP)(
  IN  EDKII_IOMMU_PROTOCOL                     *This,
  IN  VOID                                     *Mapping
  );

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param  This                  The protocol instance pointer.
  @param  Type                  This parameter is not used and must be ignored.
  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  Attributes            The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE, MEMORY_CACHED and DUAL_ADDRESS_CYCLE.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IOMMU_ALLOCATE_BUFFER)(
  IN     EDKII_IOMMU_PROTOCOL                     *This,
  IN     EFI_ALLOCATE_TYPE                        Type,
  IN     EFI_MEMORY_TYPE                          MemoryType,
  IN     UINTN                                    Pages,
  IN OUT VOID                                     **HostAddress,
  IN     UINT64                                   Attributes
  );

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  This                  The protocol instance pointer.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IOMMU_FREE_BUFFER)(
  IN  EDKII_IOMMU_PROTOCOL                     *This,
  IN  UINTN                                    Pages,
  IN  VOID                                     *HostAddress
  );

///
/// IOMMU Protocol structure.
///
struct _EDKII_IOMMU_PROTOCOL {
  UINT64                         Revision;
  EDKII_IOMMU_SET_ATTRIBUTE      SetAttribute;
  EDKII_IOMMU_MAP                Map;
  EDKII_IOMMU_UNMAP              Unmap;
  EDKII_IOMMU_ALLOCATE_BUFFER    AllocateBuffer;
  EDKII_IOMMU_FREE_BUFFER        FreeBuffer;
};

///
/// IOMMU Protocol GUID variable.
///
extern EFI_GUID  gEdkiiIoMmuProtocolGuid;

#endif
