/** @file
  The header file for EFI_ISA_IO protocol implementation.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ISA_IO_H_
#define _ISA_IO_H_

#include "IsaDriver.h"

//
// Bits definition of PcdIsaBusSupportedFeatures
//
#define PCD_ISA_BUS_SUPPORT_DMA                  BIT0
#define PCD_ISA_BUS_ONLY_SUPPORT_SLAVE_DMA       BIT1
#define PCD_ISA_BUS_SUPPORT_ISA_MEMORY           BIT2

//
// ISA I/O Support Function Prototypes
//

/**
  Verifies access to an ISA device

  @param[in] IsaIoDevice         The ISA device to be verified.
  @param[in] Type                The Access type. The input must be either IsaAccessTypeMem or IsaAccessTypeIo.
  @param[in] Width               The width of the memory operation.
  @param[in] Count               The number of memory operations to perform.
  @param[in] Offset              The offset in ISA memory space to start the memory operation.

  @retval EFI_SUCCESS            Verify success.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED        The device ont support the access type.
**/
EFI_STATUS
IsaIoVerifyAccess (
  IN ISA_IO_DEVICE              *IsaIoDevice,
  IN ISA_ACCESS_TYPE            Type,
  IN EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  IN UINT32                     Offset
  );

/**
  Performs an ISA I/O Read Cycle

  @param[in]  This              A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Width             Specifies the width of the I/O operation.
  @param[in]  Offset            The offset in ISA I/O space to start the I/O operation.
  @param[in]  Count             The number of I/O operations to perform.
  @param[out] Buffer            The destination buffer to store the results

  @retval EFI_SUCCESS           The data was read from the device sucessfully.
  @retval EFI_UNSUPPORTED       The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoIoRead (
  IN  EFI_ISA_IO_PROTOCOL        *This,
  IN  EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN  UINT32                     Offset,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  );

/**
  Performs an ISA I/O Write Cycle

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Width               Specifies the width of the I/O operation.
  @param[in] Offset              The offset in ISA I/O space to start the I/O operation.
  @param[in] Count               The number of I/O operations to perform.
  @param[in] Buffer              The source buffer to write data from

  @retval EFI_SUCCESS            The data was writen to the device sucessfully.
  @retval EFI_UNSUPPORTED        The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoIoWrite (
  IN EFI_ISA_IO_PROTOCOL        *This,
  IN EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN UINT32                     Offset,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  );

/**
  Maps a memory region for DMA

  @param This                    A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param Operation               Indicates the type of DMA (slave or bus master), and if
                                 the DMA operation is going to read or write to system memory.
  @param ChannelNumber           The slave channel number to use for this DMA operation.
                                 If Operation and ChannelAttributes shows that this device
                                 performs bus mastering DMA, then this field is ignored.
                                 The legal range for this field is 0..7.
  @param ChannelAttributes       The attributes of the DMA channel to use for this DMA operation
  @param HostAddress             The system memory address to map to the device.
  @param NumberOfBytes           On input the number of bytes to map.  On output the number
                                 of bytes that were mapped.
  @param DeviceAddress           The resulting map address for the bus master device to use
                                 to access the hosts HostAddress.
  @param Mapping                 A resulting value to pass to EFI_ISA_IO.Unmap().

  @retval EFI_SUCCESS            The range was mapped for the returned NumberOfBytes.
  @retval EFI_INVALID_PARAMETER  The Operation or HostAddress is undefined.
  @retval EFI_UNSUPPORTED        The HostAddress can not be mapped as a common buffer.
  @retval EFI_DEVICE_ERROR       The system hardware could not map the requested address.
  @retval EFI_OUT_OF_RESOURCES   The memory pages could not be allocated.
**/
EFI_STATUS
EFIAPI
IsaIoMap (
  IN     EFI_ISA_IO_PROTOCOL            *This,
  IN     EFI_ISA_IO_PROTOCOL_OPERATION  Operation,
  IN     UINT8                          ChannelNumber  OPTIONAL,
  IN     UINT32                         ChannelAttributes,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  );

/**
  Unmaps a memory region for DMA

  @param[in] This           A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Mapping        The mapping value returned from EFI_ISA_IO.Map().

  @retval EFI_SUCCESS       The range was unmapped.
  @retval EFI_DEVICE_ERROR  The data was not committed to the target system memory.
**/
EFI_STATUS
EFIAPI
IsaIoUnmap (
  IN EFI_ISA_IO_PROTOCOL  *This,
  IN VOID                 *Mapping
  );

/**
  Flushes any posted write data to the system memory.

  @param[in] This             A pointer to the EFI_ISA_IO_PROTOCOL instance.

  @retval  EFI_SUCCESS        The buffers were flushed.
  @retval  EFI_DEVICE_ERROR   The buffers were not flushed due to a hardware error.
**/
EFI_STATUS
EFIAPI
IsaIoFlush (
  IN EFI_ISA_IO_PROTOCOL  *This
  );

/**
  Writes I/O operation base address and count number to a 8 bit I/O Port.

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] AddrOffset          The address' offset.
  @param[in] PageOffset          The page's offest.
  @param[in] CountOffset         The count's offset.
  @param[in] BaseAddress         The base address.
  @param[in] Count               The number of I/O operations to perform.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Parameter is invalid.
  @retval EFI_UNSUPPORTED        The address range specified by these Offsets and Count is not valid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
WriteDmaPort (
  IN EFI_ISA_IO_PROTOCOL  *This,
  IN UINT32               AddrOffset,
  IN UINT32               PageOffset,
  IN UINT32               CountOffset,
  IN UINT32               BaseAddress,
  IN UINT16               Count
  );

/**
  Writes an 8-bit I/O Port

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Offset              The offset in ISA IO space to start the IO operation.
  @param[in] Value               The data to write port.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Parameter is invalid.
  @retval EFI_UNSUPPORTED        The address range specified by Offset is not valid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
WritePort (
  IN EFI_ISA_IO_PROTOCOL  *This,
  IN UINT32               Offset,
  IN UINT8                Value
  );

/**
  Performs an ISA Memory Read Cycle

  @param[in]  This               A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Width              Specifies the width of the memory operation.
  @param[in]  Offset             The offset in ISA memory space to start the memory operation.
  @param[in]  Count              The number of memory operations to perform.
  @param[out] Buffer             The destination buffer to store the results

  @retval EFI_SUCCESS            The data was read from the device successfully.
  @retval EFI_UNSUPPORTED        The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoMemRead (
  IN  EFI_ISA_IO_PROTOCOL        *This,
  IN  EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN  UINT32                     Offset,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  );


/**
  Performs an ISA Memory Write Cycle

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Width               Specifies the width of the memory operation.
  @param[in] Offset              The offset in ISA memory space to start the memory operation.
  @param[in] Count               The number of memory operations to perform.
  @param[in] Buffer              The source buffer to write data from

  @retval EFI_SUCCESS            The data was written to the device sucessfully.
  @retval EFI_UNSUPPORTED        The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoMemWrite (
  IN EFI_ISA_IO_PROTOCOL        *This,
  IN EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN UINT32                     Offset,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  );

/**
  Copy one region of ISA memory space to another region of ISA memory space on the ISA controller.

  @param[in]  This               A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Width              Specifies the width of the memory copy operation.
  @param[in]  DestOffset         The offset of the destination
  @param[in]  SrcOffset          The offset of the source
  @param[in]  Count              The number of memory copy  operations to perform

  @retval EFI_SUCCESS            The data was copied sucessfully.
  @retval EFI_UNSUPPORTED        The DestOffset or SrcOffset is not valid for this device.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoCopyMem (
  IN EFI_ISA_IO_PROTOCOL        *This,
  IN EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN UINT32                     DestOffset,
  IN UINT32                     SrcOffset,
  IN UINTN                      Count
  );

/**
  Allocates pages that are suitable for an EfiIsaIoOperationBusMasterCommonBuffer mapping.

  @param[in]  This               A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Type               The type allocation to perform.
  @param[in]  MemoryType         The type of memory to allocate.
  @param[in]  Pages              The number of pages to allocate.
  @param[out] HostAddress        A pointer to store the base address of the allocated range.
  @param[in]  Attributes         The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS            The requested memory pages were allocated.
  @retval EFI_INVALID_PARAMETER  Type is invalid or MemoryType is invalid or HostAddress is NULL
  @retval EFI_UNSUPPORTED        Attributes is unsupported or the memory range specified
                                 by HostAddress, Pages, and Type is not available for common buffer use.
  @retval EFI_OUT_OF_RESOURCES   The memory pages could not be allocated.
**/
EFI_STATUS
EFIAPI
IsaIoAllocateBuffer (
  IN  EFI_ISA_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE    Type,
  IN  EFI_MEMORY_TYPE      MemoryType,
  IN  UINTN                Pages,
  OUT VOID                 **HostAddress,
  IN  UINT64               Attributes
  );

/**
  Frees memory that was allocated with EFI_ISA_IO.AllocateBuffer().

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Pages               The number of pages to free.
  @param[in] HostAddress         The base address of the allocated range.

  @retval EFI_SUCCESS            The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER  The memory was not allocated with EFI_ISA_IO.AllocateBufer().
**/
EFI_STATUS
EFIAPI
IsaIoFreeBuffer (
  IN EFI_ISA_IO_PROTOCOL  *This,
  IN UINTN                Pages,
  IN VOID                 *HostAddress
  );

#endif

