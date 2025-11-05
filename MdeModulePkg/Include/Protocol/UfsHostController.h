/** @file

  EDKII Universal Flash Storage Host Controller Protocol.

Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_UFS_HC_PROTOCOL_H__
#define __EDKII_UFS_HC_PROTOCOL_H__

//
// UFS Host Controller Protocol GUID value
//
#define EDKII_UFS_HOST_CONTROLLER_PROTOCOL_GUID \
    { \
      0xebc01af5, 0x7a9, 0x489e, { 0xb7, 0xce, 0xdc, 0x8, 0x9e, 0x45, 0x9b, 0x2f } \
    }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EDKII_UFS_HOST_CONTROLLER_PROTOCOL EDKII_UFS_HOST_CONTROLLER_PROTOCOL;

/**
  Get the MMIO base address of UFS host controller.

  @param  This          The protocol instance pointer.
  @param  MmioBar       Pointer to the UFS host controller MMIO base address.

  @retval EFI_SUCCESS            The operation succeeds.
  @retval EFI_INVALID_PARAMETER  The parameters are invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_GET_MMIO_BAR)(
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL     *This,
  OUT UINTN                                  *MmioBar
  );

///
/// *******************************************************
/// EFI_UFS_HOST_CONTROLLER_OPERATION
/// *******************************************************
///
typedef enum {
  ///
  /// A read operation from system memory by a bus master.
  ///
  EdkiiUfsHcOperationBusMasterRead,
  ///
  /// A write operation from system memory by a bus master.
  ///
  EdkiiUfsHcOperationBusMasterWrite,
  ///
  /// Provides both read and write access to system memory by both the processor and a
  /// bus master. The buffer is coherent from both the processor's and the bus master's point of view.
  ///
  EdkiiUfsHcOperationBusMasterCommonBuffer,
  EdkiiUfsHcOperationMaximum
} EDKII_UFS_HOST_CONTROLLER_OPERATION;

/**
  Provides the UFS controller-specific addresses needed to access system memory.

  @param  This                  A pointer to the EFI_UFS_HOST_CONTROLLER_PROTOCOL instance.
  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the UFS controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master UFS controller to use to
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
(EFIAPI *EDKII_UFS_HC_MAP)(
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL   *This,
  IN     EDKII_UFS_HOST_CONTROLLER_OPERATION  Operation,
  IN     VOID                                 *HostAddress,
  IN OUT UINTN                                *NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS                 *DeviceAddress,
  OUT VOID                                 **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  A pointer to the EFI_UFS_HOST_CONTROLLER_PROTOCOL instance.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_UNMAP)(
  IN  EDKII_UFS_HOST_CONTROLLER_PROTOCOL     *This,
  IN  VOID                                   *Mapping
  );

/**
  Allocates pages that are suitable for an EfiUfsHcOperationBusMasterCommonBuffer
  mapping.

  @param  This                  A pointer to the EFI_UFS_HOST_CONTROLLER_PROTOCOL instance.
  @param  Type                  This parameter is not used and must be ignored.
  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  Attributes            The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_ALLOCATE_BUFFER)(
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL   *This,
  IN     EFI_ALLOCATE_TYPE                    Type,
  IN     EFI_MEMORY_TYPE                      MemoryType,
  IN     UINTN                                Pages,
  OUT VOID                                 **HostAddress,
  IN     UINT64                               Attributes
  );

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  This                  A pointer to the EFI_UFS_HOST_CONTROLLER_PROTOCOL instance.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_FREE_BUFFER)(
  IN  EDKII_UFS_HOST_CONTROLLER_PROTOCOL   *This,
  IN  UINTN                                Pages,
  IN  VOID                                 *HostAddress
  );

/**
  Flushes all posted write transactions from the UFS bus to attached UFS device.

  @param  This                  A pointer to the EFI_UFS_HOST_CONTROLLER_PROTOCOL instance.

  @retval EFI_SUCCESS           The posted write transactions were flushed from the UFS bus
                                to attached UFS device.
  @retval EFI_DEVICE_ERROR      The posted write transactions were not flushed from the UFS
                                bus to attached UFS device due to a hardware error.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_FLUSH)(
  IN  EDKII_UFS_HOST_CONTROLLER_PROTOCOL   *This
  );

typedef enum {
  EfiUfsHcWidthUint8 = 0,
  EfiUfsHcWidthUint16,
  EfiUfsHcWidthUint32,
  EfiUfsHcWidthUint64,
  EfiUfsHcWidthMaximum
} EDKII_UFS_HOST_CONTROLLER_PROTOCOL_WIDTH;

/**
  Enable a UFS bus driver to access UFS MMIO registers in the UFS Host Controller memory space.

  @param  This                  A pointer to the EDKII_UFS_HOST_CONTROLLER_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  Offset                The offset within the UFS Host Controller MMIO space to start the
                                memory operation.
  @param  Count                 The number of memory operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results.
                                For write operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the UFS host controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the UFS Host Controller memory space.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_MMIO_READ_WRITE)(
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL        *This,
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL_WIDTH  Width,
  IN     UINT64                                    Offset,
  IN     UINTN                                     Count,
  IN OUT VOID                                      *Buffer
  );

///
///  UFS Host Controller Protocol structure.
///
struct _EDKII_UFS_HOST_CONTROLLER_PROTOCOL {
  EDKII_UFS_HC_GET_MMIO_BAR       GetUfsHcMmioBar;
  EDKII_UFS_HC_ALLOCATE_BUFFER    AllocateBuffer;
  EDKII_UFS_HC_FREE_BUFFER        FreeBuffer;
  EDKII_UFS_HC_MAP                Map;
  EDKII_UFS_HC_UNMAP              Unmap;
  EDKII_UFS_HC_FLUSH              Flush;
  EDKII_UFS_HC_MMIO_READ_WRITE    Read;
  EDKII_UFS_HC_MMIO_READ_WRITE    Write;
};

///
///  UFS Host Controller Protocol GUID variable.
///
extern EFI_GUID  gEdkiiUfsHostControllerProtocolGuid;

#endif
