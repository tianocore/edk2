/** @file
  ISA I/O Protocol is used to perform ISA device Io/Mem operations.

Copyright (c) 2006 - 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _EFI_ISA_IO_H_
#define _EFI_ISA_IO_H_


#include <Protocol/IsaAcpi.h>

//
// Global GUID for the ISA I/O Protocol
//
#define EFI_ISA_IO_PROTOCOL_GUID \
  { 0x7ee2bd44, 0x3da0, 0x11d4, { 0x9a, 0x38, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

typedef struct _EFI_ISA_IO_PROTOCOL EFI_ISA_IO_PROTOCOL;

//
// Width of ISA Io/Mem operation
//
typedef enum {
  EfiIsaIoWidthUint8,
  EfiIsaIoWidthUint16,
  EfiIsaIoWidthUint32,
  EfiIsaIoWidthReserved,
  EfiIsaIoWidthFifoUint8,
  EfiIsaIoWidthFifoUint16,
  EfiIsaIoWidthFifoUint32,
  EfiIsaIoWidthFifoReserved,
  EfiIsaIoWidthFillUint8,
  EfiIsaIoWidthFillUint16,
  EfiIsaIoWidthFillUint32,
  EfiIsaIoWidthFillReserved,
  EfiIsaIoWidthMaximum
} EFI_ISA_IO_PROTOCOL_WIDTH;

//
// Attributes for common buffer allocations
//
#define EFI_ISA_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE  0x080    ///< Map a memory range so write are combined
#define EFI_ISA_IO_ATTRIBUTE_MEMORY_CACHED         0x800    ///< Map a memory range so all r/w accesses are cached
#define EFI_ISA_IO_ATTRIBUTE_MEMORY_DISABLE        0x1000   ///< Disable a memory range 

//
// Channel attribute for DMA operations
//
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_COMPATIBLE  0x001
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_A           0x002
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_B           0x004
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_C           0x008
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_8           0x010
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_16          0x020
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SINGLE_MODE       0x040
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_DEMAND_MODE       0x080
#define EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_AUTO_INITIALIZE   0x100

typedef enum {
  EfiIsaIoOperationBusMasterRead,
  EfiIsaIoOperationBusMasterWrite,
  EfiIsaIoOperationBusMasterCommonBuffer,
  EfiIsaIoOperationSlaveRead,
  EfiIsaIoOperationSlaveWrite,
  EfiIsaIoOperationMaximum
} EFI_ISA_IO_PROTOCOL_OPERATION;

/**
  Performs an ISA Io/Memory Read/Write Cycle

  @param This                    A pointer to the EFI_ISA_IO_PROTOCOL instance.  
  @param Width                   Signifies the width of the Io/Memory operation.
  @param Offset                  The offset in ISA Io/Memory space to start the Io/Memory operation.  
  @param Count                   The number of Io/Memory operations to perform.
  @param Buffer                  [OUT] The destination buffer to store the results.
                                 [IN] The source buffer to write data to the device.

  @retval EFI_SUCCESS             The data was read from / written to the device sucessfully.
  @retval EFI_UNSUPPORTED         The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER   Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_IO_MEM) (
  IN     EFI_ISA_IO_PROTOCOL          *This,
  IN     EFI_ISA_IO_PROTOCOL_WIDTH    Width,
  IN     UINT32                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  );

typedef struct {
  EFI_ISA_IO_PROTOCOL_IO_MEM  Read;
  EFI_ISA_IO_PROTOCOL_IO_MEM  Write;
} EFI_ISA_IO_PROTOCOL_ACCESS;

/**
  Performs an ISA I/O Copy Memory 

  @param This                    A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param Width                   Signifies the width of the memory copy operation.
  @param DestOffset              The offset of the destination 
  @param SrcOffset               The offset of the source
  @param Count                   The number of memory copy  operations to perform

  @retval EFI_SUCCESS             The data was copied sucessfully.
  @retval EFI_UNSUPPORTED         The DestOffset or SrcOffset is not valid for this device.
  @retval EFI_INVALID_PARAMETER   Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_COPY_MEM) (
  IN     EFI_ISA_IO_PROTOCOL          *This,
  IN     EFI_ISA_IO_PROTOCOL_WIDTH    Width,
  IN     UINT32                       DestOffset,
  IN     UINT32                       SrcOffset,
  IN     UINTN                        Count
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


  @retval EFI_SUCCESS             The range was mapped for the returned NumberOfBytes.
  @retval EFI_INVALID_PARAMETER   The Operation or HostAddress is undefined.
  @retval EFI_UNSUPPORTED         The HostAddress can not be mapped as a common buffer.
  @retval EFI_DEVICE_ERROR        The system hardware could not map the requested address.
  @retval EFI_OUT_OF_RESOURCES    The memory pages could not be allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_MAP) (
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
  Unmaps a memory region for DMA

  @param This               A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param Mapping            The mapping value returned from EFI_ISA_IO.Map().

  @retval EFI_SUCCESS        The range was unmapped.
  @retval EFI_DEVICE_ERROR   The data was not committed to the target system memory.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_UNMAP) (
  IN  EFI_ISA_IO_PROTOCOL          *This,
  IN  VOID                         *Mapping
  );

/**
  Allocates a common buffer for DMA

  @param This                    A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param Type                    The type allocation to perform.
  @param MemoryType              The type of memory to allocate.
  @param Pages                   The number of pages to allocate.
  @param HostAddress             A pointer to store the base address of the allocated range.
  @param Attributes              The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS             The requested memory pages were allocated.
  @retval EFI_INVALID_PARAMETER   Type is invalid or MemoryType is invalid or HostAddress is NULL
  @retval EFI_UNSUPPORTED         Attributes is unsupported or the memory range specified 
                                  by HostAddress, Pages, and Type is not available for common buffer use.
  @retval EFI_OUT_OF_RESOURCES    The memory pages could not be allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_ALLOCATE_BUFFER) (
  IN  EFI_ISA_IO_PROTOCOL          *This,
  IN  EFI_ALLOCATE_TYPE            Type,
  IN  EFI_MEMORY_TYPE              MemoryType,
  IN  UINTN                        Pages,
  OUT VOID                         **HostAddress,
  IN  UINT64                       Attributes
  );

/**
  Frees a common buffer 

  @param This                    A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param Pages                   The number of pages to free.
  @param HostAddress             The base address of the allocated range.


  @retval EFI_SUCCESS             The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER   The memory was not allocated with EFI_ISA_IO.AllocateBufer().

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_FREE_BUFFER) (
  IN  EFI_ISA_IO_PROTOCOL          *This,
  IN  UINTN                        Pages,
  IN  VOID                         *HostAddress
  );

/**
  Flushes a DMA buffer

  @param This                 A pointer to the EFI_ISA_IO_PROTOCOL instance.

  @retval  EFI_SUCCESS        The buffers were flushed.
  @retval  EFI_DEVICE_ERROR   The buffers were not flushed due to a hardware error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ISA_IO_PROTOCOL_FLUSH) (
  IN EFI_ISA_IO_PROTOCOL                  *This
  );

//
// Interface structure for the ISA I/O Protocol
//
struct _EFI_ISA_IO_PROTOCOL {
  EFI_ISA_IO_PROTOCOL_ACCESS           Mem;
  EFI_ISA_IO_PROTOCOL_ACCESS           Io;
  EFI_ISA_IO_PROTOCOL_COPY_MEM         CopyMem;
  EFI_ISA_IO_PROTOCOL_MAP              Map;
  EFI_ISA_IO_PROTOCOL_UNMAP            Unmap;
  EFI_ISA_IO_PROTOCOL_ALLOCATE_BUFFER  AllocateBuffer;
  EFI_ISA_IO_PROTOCOL_FREE_BUFFER      FreeBuffer;
  EFI_ISA_IO_PROTOCOL_FLUSH            Flush;
  EFI_ISA_ACPI_RESOURCE_LIST           *ResourceList;
  UINT32                               RomSize;
  VOID                                 *RomImage;
};

extern EFI_GUID gEfiIsaIoProtocolGuid;

#endif
