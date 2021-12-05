/** @file
  ISA I/O Protocol is used by ISA device drivers to perform I/O, MMIO and DMA
  operations on the ISA controllers they manage.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_ISA_IO_H_
#define _EFI_ISA_IO_H_

#include <Protocol/IsaAcpi.h>

///
/// Global ID for the EFI_ISA_IO_PROTOCOL
///
#define EFI_ISA_IO_PROTOCOL_GUID \
  { \
    0x7ee2bd44, 0x3da0, 0x11d4, { 0x9a, 0x38, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

///
/// Forward declaration for the EFI_ISA_IO_PROTOCOL.
///
typedef struct _EFI_ISA_IO_PROTOCOL EFI_ISA_IO_PROTOCOL;

///
/// Width of EFI_ISA_IO_PROTOCOL I/O Port and MMIO operations.
///
typedef enum {
  EfiIsaIoWidthUint8 = 0,      ///< 8-bit operation.
  EfiIsaIoWidthUint16,         ///< 16-bit operation.
  EfiIsaIoWidthUint32,         ///< 32-bit operation
  EfiIsaIoWidthReserved,
  EfiIsaIoWidthFifoUint8,      ///< 8-bit FIFO operation.
  EfiIsaIoWidthFifoUint16,     ///< 16-bit FIFO operation.
  EfiIsaIoWidthFifoUint32,     ///< 32-bit FIFO operation.
  EfiIsaIoWidthFifoReserved,
  EfiIsaIoWidthFillUint8,      ///< 8-bit Fill operation.
  EfiIsaIoWidthFillUint16,     ///< 16-bit Fill operation.
  EfiIsaIoWidthFillUint32,     ///< 32-bit Fill operation.
  EfiIsaIoWidthFillReserved,
  EfiIsaIoWidthMaximum
} EFI_ISA_IO_PROTOCOL_WIDTH;

///
/// Attributes for the EFI_ISA_IO_PROTOCOL common DMA buffer allocations.
///
#define EFI_ISA_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE  0x080    ///< Map a memory range so write are combined.
#define EFI_ISA_IO_ATTRIBUTE_MEMORY_CACHED         0x800    ///< Map a memory range so all read and write accesses are cached.
#define EFI_ISA_IO_ATTRIBUTE_MEMORY_DISABLE        0x1000   ///< Disable a memory range.

///
/// Channel attribute for EFI_ISA_IO_PROTOCOL slave DMA requests
///
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_COMPATIBLE  0x001   ///< Set the speed of the DMA transfer in compatible mode.
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_A           0x002   ///< Not supported.
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_B           0x004   ///< Not supported.
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_C           0x008   ///< Not supported.
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_8           0x010   ///< Request 8-bit DMA transfers.  Only available on channels 0..3.
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_16          0x020   ///< Request 16-bit DMA transfers.  Only available on channels 4..7.
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SINGLE_MODE       0x040   ///< Request a single DMA transfer.
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_DEMAND_MODE       0x080   ///< Request multiple DMA transfers until TC (Terminal Count) or EOP (End of Process).
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_AUTO_INITIALIZE   0x100   ///< Automatically reload base and count at the end of the DMA transfer.

///
/// The DMA opreration type for EFI_ISA_IO_PROTOCOL DMA requests.
///
typedef enum {
  ///
  /// A read operation from system memory by a bus master.
  ///
  EfiIsaIoOperationBusMasterRead,
  ///
  /// A write operation to system memory by a bus master.
  ///
  EfiIsaIoOperationBusMasterWrite,
  ///
  /// Provides both read and write access to system memory by both the processor
  /// and a bus master. The buffer is coherent from both the processor's and the
  /// bus master's point of view.
  ///
  EfiIsaIoOperationBusMasterCommonBuffer,
  ///
  /// A read operation from system memory by a slave device.
  ///
  EfiIsaIoOperationSlaveRead,
  ///
  /// A write operation to system memory by a slave master.
  ///
  EfiIsaIoOperationSlaveWrite,
  EfiIsaIoOperationMaximum
} EFI_ISA_IO_PROTOCOL_OPERATION;

/**
  Performs ISA I/O and MMIO Read/Write Cycles

  @param[in]      This     A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]      Width    Specifies the width of the I/O or MMIO operation.
  @param[in]      Offset   The offset into the ISA I/O or MMIO space to start the
                           operation.
  @param[in]      Count    The number of I/O or MMIO operations to perform.
  @param[in, out] Buffer   For read operations, the destination buffer to store
                           the results. For write operations, the source buffer to
                           write data from.

  @retval EFI_SUCCESS             The data was successfully read from or written to the device.
  @retval EFI_UNSUPPORTED         The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER   Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_IO_MEM)(
  IN     EFI_ISA_IO_PROTOCOL        *This,
  IN     EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  );

///
/// Structure of functions for accessing ISA I/O and MMIO space.
///
typedef struct {
  ///
  /// Read from ISA I/O or MMIO space.
  ///
  EFI_ISA_IO_PROTOCOL_IO_MEM    Read;
  ///
  /// Write to ISA I/O or MMIO space.
  ///
  EFI_ISA_IO_PROTOCOL_IO_MEM    Write;
} EFI_ISA_IO_PROTOCOL_ACCESS;

/**
  Copies data from one region of ISA MMIO space to another region of ISA
  MMIO space.

  @param[in] This         A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Width        Specifies the width of the MMIO copy operation.
  @param[in] DestOffset   The offset of the destination in ISA MMIO space.
  @param[in] SrcOffset    The offset of the source in ISA MMIO space.
  @param[in] Count        The number tranfers to perform for this copy operation.

  @retval EFI_SUCCESS             The data was copied successfully.
  @retval EFI_UNSUPPORTED         The DestOffset or SrcOffset is not valid for this device.
  @retval EFI_INVALID_PARAMETER   Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_COPY_MEM)(
  IN EFI_ISA_IO_PROTOCOL         *This,
  IN EFI_ISA_IO_PROTOCOL_WIDTH   Width,
  IN UINT32                      DestOffset,
  IN UINT32                      SrcOffset,
  IN UINTN                       Count
  );

/**
  Maps a memory region for DMA.

  This function returns the device-specific addresses required to access system memory.
  This function is used to map system memory for ISA DMA operations.  All ISA DMA
  operations must be performed through their mapped addresses, and such mappings must
  be freed with EFI_ISA_IO_PROTOCOL.Unmap() after the DMA operation is completed.

  If the DMA operation is a single read or write data transfer through an ISA bus
  master, then EfiIsaIoOperationBusMasterRead or EfiIsaIoOperationBusMasterWrite
  is used and the range is unmapped to complete the operation. If the DMA operation
  is a single read or write data transfer through an ISA slave controller, then
  EfiIsaIoOperationSlaveRead or EfiIsaIoOperationSlaveWrite is used and the range
  is unmapped to complete the operation.

  If performing a DMA read operation, all the data must be present in system memory before the Map() is performed.  Similarly,
  if performing a DMA write operation, the data must not be accessed in system
  memory until EFI_ISA_IO_PROTOCOL.Unmap() is performed.  Bus master operations that
  require both read and write access or require multiple host device interactions
  within the same mapped region must use EfiIsaIoOperationBusMasterCommonBuffer.
  However, only memory allocated via the EFI_ISA_IO_PROTOCOL.AllocateBuffer() interface
  is guaranteed to be able to be mapped for this operation type.  In all mapping
  requests the NumberOfBytes returned may be less than originally requested.  It is
  the caller's responsibility to make additional requests to complete the entire
  transfer.

  @param[in]      This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]      Operation           Indicates the type of DMA (slave or bus master),
                                      and if the DMA operation is going to read or
                                      write to system memory.
  @param[in]      ChannelNumber       The slave channel number to use for this DMA
                                      operation.  If Operation and ChannelAttributes
                                      shows that this device performs bus mastering
                                      DMA, then this field is ignored.  The legal
                                      range for this field is 0..7.
  @param[in]      ChannelAttributes   A bitmask of the attributes used to configure
                                      the slave DMA channel for this DMA operation.
                                      See EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_* for the
                                      legal bit combinations.
  @param[in]      HostAddress         The system memory address to map to the device.
  @param[in, out] NumberOfBytes       On input the number of bytes to map.  On
                                      output the number of bytes that were mapped.
  @param[out]     DeviceAddress       The resulting map address for the bus master
                                      device to use to access the hosts HostAddress.
  @param[out]     Mapping             A returned value that must be passed to into
                                      EFI_ISA_IO_PROTOCOL.Unmap() to free all the the
                                      resources associated with this map request.

  @retval EFI_SUCCESS             The range was mapped for the returned NumberOfBytes.
  @retval EFI_INVALID_PARAMETER   The Operation is undefined.
  @retval EFI_INVALID_PARAMETER   The HostAddress is undefined.
  @retval EFI_UNSUPPORTED         The HostAddress can not be mapped as a common buffer.
  @retval EFI_DEVICE_ERROR        The system hardware could not map the requested address.
  @retval EFI_OUT_OF_RESOURCES    The memory pages could not be allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_MAP)(
  IN     EFI_ISA_IO_PROTOCOL            *This,
  IN     EFI_ISA_IO_PROTOCOL_OPERATION  Operation,
  IN     UINT8                          ChannelNumber      OPTIONAL,
  IN     UINT32                         ChannelAttributes,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  );

/**
  Unmaps a memory region that was previously mapped with EFI_ISA_IO_PROTOCOL.Map().

  The EFI_ISA_IO_PROTOCOL.Map() operation is completed and any corresponding
  resources are released.  If the operation was EfiIsaIoOperationSlaveWrite
  or EfiIsaIoOperationBusMasterWrite, the data is committed to system memory.
  Any resources used for the mapping are freed.

  @param[in] This           A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Mapping        The mapping value returned from EFI_ISA_IO_PROTOCOL.Map().

  @retval EFI_SUCCESS       The memory region was unmapped.
  @retval EFI_DEVICE_ERROR  The data was not committed to the target system memory.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_UNMAP)(
  IN  EFI_ISA_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
  );

/**
  Allocates pages that are suitable for an EfiIsaIoOperationBusMasterCommonBuffer
  mapping.

  @param[in]  This          A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Type          The type allocation to perform.
  @param[in]  MemoryType    The type of memory to allocate.
  @param[in]  Pages         The number of pages to allocate.
  @param[out] HostAddress   A pointer to store the base address of the allocated range.
  @param[in]  Attributes    The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS             The requested memory pages were allocated.
  @retval EFI_INVALID_PARAMETER   Type is invalid.
  @retval EFI_INVALID_PARAMETER   MemoryType is invalid.
  @retval EFI_INVALID_PARAMETER   HostAddress is NULL.
  @retval EFI_UNSUPPORTED         Attributes is unsupported.
  @retval EFI_UNSUPPORTED         The memory range specified by HostAddress, Pages,
                                  and Type is not available for common buffer use.
  @retval EFI_OUT_OF_RESOURCES    The memory pages could not be allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_ALLOCATE_BUFFER)(
  IN  EFI_ISA_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE    Type,
  IN  EFI_MEMORY_TYPE      MemoryType,
  IN  UINTN                Pages,
  OUT VOID                 **HostAddress,
  IN  UINT64               Attributes
  );

/**
  Frees a common buffer that was allocated with EFI_ISA_IO_PROTOCOL.AllocateBuffer().

  @param[in] This          A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Pages         The number of pages to free from the previously allocated common buffer.
  @param[in] HostAddress   The base address of the previously allocated common buffer.


  @retval EFI_SUCCESS             The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER   The memory was not allocated with EFI_ISA_IO.AllocateBufer().

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_FREE_BUFFER)(
  IN  EFI_ISA_IO_PROTOCOL  *This,
  IN  UINTN                Pages,
  IN  VOID                 *HostAddress
  );

/**
  Flushes a DMA buffer, which forces all DMA posted write transactions to complete.

  @param[in] This   A pointer to the EFI_ISA_IO_PROTOCOL instance.

  @retval  EFI_SUCCESS        The DMA buffers were flushed.
  @retval  EFI_DEVICE_ERROR   The buffers were not flushed due to a hardware error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_FLUSH)(
  IN EFI_ISA_IO_PROTOCOL  *This
  );

///
/// The EFI_ISA_IO_PROTOCOL provides the basic Memory, I/O, and DMA interfaces
/// used to abstract accesses to ISA controllers.  There is one EFI_ISA_IO_PROTOCOL
/// instance for each ISA controller on a ISA bus. A device driver that wishes
/// to manage an ISA controller in a system will have to retrieve the
/// ISA_PCI_IO_PROTOCOL instance associated with the ISA controller.
///
struct _EFI_ISA_IO_PROTOCOL {
  EFI_ISA_IO_PROTOCOL_ACCESS             Mem;
  EFI_ISA_IO_PROTOCOL_ACCESS             Io;
  EFI_ISA_IO_PROTOCOL_COPY_MEM           CopyMem;
  EFI_ISA_IO_PROTOCOL_MAP                Map;
  EFI_ISA_IO_PROTOCOL_UNMAP              Unmap;
  EFI_ISA_IO_PROTOCOL_ALLOCATE_BUFFER    AllocateBuffer;
  EFI_ISA_IO_PROTOCOL_FREE_BUFFER        FreeBuffer;
  EFI_ISA_IO_PROTOCOL_FLUSH              Flush;
  ///
  /// The list of I/O , MMIO, DMA, and Interrupt resources associated with the
  /// ISA controller abstracted by this instance of the EFI_ISA_IO_PROTOCOL.
  ///
  EFI_ISA_ACPI_RESOURCE_LIST             *ResourceList;
  ///
  /// The size, in bytes, of the ROM image.
  ///
  UINT32                                 RomSize;
  ///
  /// A pointer to the in memory copy of the ROM image. The ISA Bus Driver is responsible
  /// for allocating memory for the ROM image, and copying the contents of the ROM to memory
  /// during ISA Bus initialization.
  ///
  VOID                                   *RomImage;
};

extern EFI_GUID  gEfiIsaIoProtocolGuid;

#endif
