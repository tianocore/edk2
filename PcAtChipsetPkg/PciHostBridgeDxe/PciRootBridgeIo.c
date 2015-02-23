/** @file
  PCI Root Bridge Io Protocol implementation

Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are
licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/ 

#include "PciHostBridge.h"
#include "IoFifo.h"

typedef struct {
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR     SpaceDesp[TypeMax];
  EFI_ACPI_END_TAG_DESCRIPTOR           EndDesp;
} RESOURCE_CONFIGURATION;

RESOURCE_CONFIGURATION Configuration = {
  {{0x8A, 0x2B, 1, 0, 0, 0, 0, 0, 0, 0},
  {0x8A, 0x2B, 0, 0, 0, 32, 0, 0, 0, 0}, 
  {0x8A, 0x2B, 0, 0, 6, 32, 0, 0, 0, 0},
  {0x8A, 0x2B, 0, 0, 0, 64, 0, 0, 0, 0},
  {0x8A, 0x2B, 0, 0, 6, 64, 0, 0, 0, 0},
  {0x8A, 0x2B, 2, 0, 0, 0, 0, 0, 0, 0}},
  {0x79, 0}
};

//
// Protocol Member Function Prototypes
//

/**
   Polls an address in memory mapped I/O space until an exit condition is met, or 
   a timeout occurs. 

   This function provides a standard way to poll a PCI memory location. A PCI memory read
   operation is performed at the PCI memory address specified by Address for the width specified
   by Width. The result of this PCI memory read operation is stored in Result. This PCI memory
   read operation is repeated until either a timeout of Delay 100 ns units has expired, or (Result &
   Mask) is equal to Value.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operations.
   @param[in]   Address   The base address of the memory operations. The caller is
                          responsible for aligning Address if required.
   @param[in]   Mask      Mask used for the polling criteria. Bytes above Width in Mask
                          are ignored. The bits in the bytes below Width which are zero in
                          Mask are ignored when polling the memory address.
   @param[in]   Value     The comparison value used for the polling exit criteria.
   @param[in]   Delay     The number of 100 ns units to poll. Note that timer available may
                          be of poorer granularity.
   @param[out]  Result    Pointer to the last value read from the memory location.
   
   @retval EFI_SUCCESS            The last data returned from the access matched the poll exit criteria.
   @retval EFI_INVALID_PARAMETER  Width is invalid.
   @retval EFI_INVALID_PARAMETER  Result is NULL.
   @retval EFI_TIMEOUT            Delay expired before a match occurred.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoPollMem ( 
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  );
  
/**
   Reads from the I/O space of a PCI Root Bridge. Returns when either the polling exit criteria is
   satisfied or after a defined duration.

   This function provides a standard way to poll a PCI I/O location. A PCI I/O read operation is
   performed at the PCI I/O address specified by Address for the width specified by Width.
   The result of this PCI I/O read operation is stored in Result. This PCI I/O read operation is
   repeated until either a timeout of Delay 100 ns units has expired, or (Result & Mask) is equal
   to Value.

   @param[in] This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in] Width     Signifies the width of the I/O operations.
   @param[in] Address   The base address of the I/O operations. The caller is responsible
                        for aligning Address if required.
   @param[in] Mask      Mask used for the polling criteria. Bytes above Width in Mask
                        are ignored. The bits in the bytes below Width which are zero in
                        Mask are ignored when polling the I/O address.
   @param[in] Value     The comparison value used for the polling exit criteria.
   @param[in] Delay     The number of 100 ns units to poll. Note that timer available may
                        be of poorer granularity.
   @param[out] Result   Pointer to the last value read from the memory location.
   
   @retval EFI_SUCCESS            The last data returned from the access matched the poll exit criteria.
   @retval EFI_INVALID_PARAMETER  Width is invalid.
   @retval EFI_INVALID_PARAMETER  Result is NULL.
   @retval EFI_TIMEOUT            Delay expired before a match occurred.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoPollIo ( 
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  );
  
/**
   Enables a PCI driver to access PCI controller registers in the PCI root bridge memory space.

   The Mem.Read(), and Mem.Write() functions enable a driver to access PCI controller
   registers in the PCI root bridge memory space.
   The memory operations are carried out exactly as requested. The caller is responsible for satisfying
   any alignment and memory width restrictions that a PCI Root Bridge on a platform might require.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operation.
   @param[in]   Address   The base address of the memory operation. The caller is
                          responsible for aligning the Address if required.
   @param[in]   Count     The number of memory operations to perform. Bytes moved is
                          Width size * Count, starting at Address.
   @param[out]  Buffer    For read operations, the destination buffer to store the results. For
                          write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoMemRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  OUT    VOID                                   *Buffer
  );

/**
   Enables a PCI driver to access PCI controller registers in the PCI root bridge memory space.

   The Mem.Read(), and Mem.Write() functions enable a driver to access PCI controller
   registers in the PCI root bridge memory space.
   The memory operations are carried out exactly as requested. The caller is responsible for satisfying
   any alignment and memory width restrictions that a PCI Root Bridge on a platform might require.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operation.
   @param[in]   Address   The base address of the memory operation. The caller is
                          responsible for aligning the Address if required.
   @param[in]   Count     The number of memory operations to perform. Bytes moved is
                          Width size * Count, starting at Address.
   @param[in]   Buffer    For read operations, the destination buffer to store the results. For
                          write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
RootBridgeIoMemWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN     VOID                                   *Buffer
  );

/**
   Enables a PCI driver to access PCI controller registers in the PCI root bridge I/O space.

   @param[in]   This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width       Signifies the width of the memory operations.
   @param[in]   UserAddress The base address of the I/O operation. The caller is responsible for
                            aligning the Address if required.
   @param[in]   Count       The number of I/O operations to perform. Bytes moved is Width
                            size * Count, starting at Address.
   @param[out]  UserBuffer  For read operations, the destination buffer to store the results. For
                            write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS              The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER    Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER    Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoIoRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  OUT    VOID                                   *UserBuffer
  );

/**
   Enables a PCI driver to access PCI controller registers in the PCI root bridge I/O space.

   @param[in]   This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width       Signifies the width of the memory operations.
   @param[in]   UserAddress The base address of the I/O operation. The caller is responsible for
                            aligning the Address if required.
   @param[in]   Count       The number of I/O operations to perform. Bytes moved is Width
                            size * Count, starting at Address.
   @param[in]   UserBuffer  For read operations, the destination buffer to store the results. For
                            write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS              The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER    Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER    Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoIoWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN     VOID                                   *UserBuffer
  );

/**
   Enables a PCI driver to copy one region of PCI root bridge memory space to another region of PCI
   root bridge memory space.

   The CopyMem() function enables a PCI driver to copy one region of PCI root bridge memory
   space to another region of PCI root bridge memory space. This is especially useful for video scroll
   operation on a memory mapped video buffer.
   The memory operations are carried out exactly as requested. The caller is responsible for satisfying
   any alignment and memory width restrictions that a PCI root bridge on a platform might require.

   @param[in] This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
   @param[in] Width       Signifies the width of the memory operations.
   @param[in] DestAddress The destination address of the memory operation. The caller is
                          responsible for aligning the DestAddress if required.
   @param[in] SrcAddress  The source address of the memory operation. The caller is
                          responsible for aligning the SrcAddress if required.
   @param[in] Count       The number of memory operations to perform. Bytes moved is
                          Width size * Count, starting at DestAddress and SrcAddress.
   
   @retval  EFI_SUCCESS             The data was copied from one memory region to another memory region.
   @retval  EFI_INVALID_PARAMETER   Width is invalid for this PCI root bridge.
   @retval  EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoCopyMem (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 DestAddress,
  IN     UINT64                                 SrcAddress,
  IN     UINTN                                  Count
  );

/**
   Enables a PCI driver to access PCI controller registers in a PCI root bridge's configuration space.

   The Pci.Read() and Pci.Write() functions enable a driver to access PCI configuration
   registers for a PCI controller.
   The PCI Configuration operations are carried out exactly as requested. The caller is responsible for
   any alignment and PCI configuration width issues that a PCI Root Bridge on a platform might
   require.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operations.
   @param[in]   Address   The address within the PCI configuration space for the PCI controller.
   @param[in]   Count     The number of PCI configuration operations to perform. Bytes
                          moved is Width size * Count, starting at Address.
   @param[out]  Buffer    For read operations, the destination buffer to store the results. For
                          write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoPciRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  OUT    VOID                                   *Buffer
  );

/**
   Enables a PCI driver to access PCI controller registers in a PCI root bridge's configuration space.

   The Pci.Read() and Pci.Write() functions enable a driver to access PCI configuration
   registers for a PCI controller.
   The PCI Configuration operations are carried out exactly as requested. The caller is responsible for
   any alignment and PCI configuration width issues that a PCI Root Bridge on a platform might
   require.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operations.
   @param[in]   Address   The address within the PCI configuration space for the PCI controller.
   @param[in]   Count     The number of PCI configuration operations to perform. Bytes
                          moved is Width size * Count, starting at Address.
   @param[in]   Buffer    For read operations, the destination buffer to store the results. For
                          write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoPciWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN     VOID                                   *Buffer
  );

/**
   Provides the PCI controller-specific addresses required to access system memory from a
   DMA bus master.

   The Map() function provides the PCI controller specific addresses needed to access system
   memory. This function is used to map system memory for PCI bus master DMA accesses.

   @param[in]       This            A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]       Operation       Indicates if the bus master is going to read or write to system memory.
   @param[in]       HostAddress     The system memory address to map to the PCI controller.
   @param[in, out]  NumberOfBytes   On input the number of bytes to map. On output the number of bytes that were mapped.
   @param[out]      DeviceAddress   The resulting map address for the bus master PCI controller to use
                                    to access the system memory's HostAddress.
   @param[out]      Mapping         The value to pass to Unmap() when the bus master DMA operation is complete.
   
   @retval EFI_SUCCESS            The range was mapped for the returned NumberOfBytes.
   @retval EFI_INVALID_PARAMETER  Operation is invalid.
   @retval EFI_INVALID_PARAMETER  HostAddress is NULL.
   @retval EFI_INVALID_PARAMETER  NumberOfBytes is NULL.
   @retval EFI_INVALID_PARAMETER  DeviceAddress is NULL.
   @retval EFI_INVALID_PARAMETER  Mapping is NULL.
   @retval EFI_UNSUPPORTED        The HostAddress cannot be mapped as a common buffer.
   @retval EFI_DEVICE_ERROR       The system hardware could not map the requested address.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoMap (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL            *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  );

/**
   Completes the Map() operation and releases any corresponding resources.

   The Unmap() function completes the Map() operation and releases any corresponding resources.
   If the operation was an EfiPciOperationBusMasterWrite or
   EfiPciOperationBusMasterWrite64, the data is committed to the target system memory.
   Any resources used for the mapping are freed.  

   @param[in] This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in] Mapping   The mapping value returned from Map().
   
   @retval EFI_SUCCESS            The range was unmapped.
   @retval EFI_INVALID_PARAMETER  Mapping is not a value that was returned by Map().
   @retval EFI_DEVICE_ERROR       The data was not committed to the target system memory.

**/
EFI_STATUS
EFIAPI
RootBridgeIoUnmap (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  VOID                             *Mapping
  );

/**
   Allocates pages that are suitable for an EfiPciOperationBusMasterCommonBuffer or
   EfiPciOperationBusMasterCommonBuffer64 mapping.
  
   @param This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param Type        This parameter is not used and must be ignored.
   @param MemoryType  The type of memory to allocate, EfiBootServicesData or EfiRuntimeServicesData.
   @param Pages       The number of pages to allocate.
   @param HostAddress A pointer to store the base system memory address of the allocated range.
   @param Attributes  The requested bit mask of attributes for the allocated range. Only
                      the attributes EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE, EFI_PCI_ATTRIBUTE_MEMORY_CACHED, 
                      and EFI_PCI_ATTRIBUTE_DUAL_ADDRESS_CYCLE may be used with this function.
   
   @retval EFI_SUCCESS            The requested memory pages were allocated.
   @retval EFI_INVALID_PARAMETER  MemoryType is invalid.
   @retval EFI_INVALID_PARAMETER  HostAddress is NULL.
   @retval EFI_UNSUPPORTED        Attributes is unsupported. The only legal attribute bits are
                                  MEMORY_WRITE_COMBINE, MEMORY_CACHED, and DUAL_ADDRESS_CYCLE.
   @retval EFI_OUT_OF_RESOURCES   The memory pages could not be allocated.

**/
EFI_STATUS
EFIAPI
RootBridgeIoAllocateBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE                Type,
  IN  EFI_MEMORY_TYPE                  MemoryType,
  IN  UINTN                            Pages,
  OUT VOID                             **HostAddress,
  IN  UINT64                           Attributes
  );

/**
   Frees memory that was allocated with AllocateBuffer().

   The FreeBuffer() function frees memory that was allocated with AllocateBuffer().

   @param This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param Pages       The number of pages to free.
   @param HostAddress The base system memory address of the allocated range.
   
   @retval EFI_SUCCESS            The requested memory pages were freed.
   @retval EFI_INVALID_PARAMETER  The memory range specified by HostAddress and Pages
                                  was not allocated with AllocateBuffer().

**/
EFI_STATUS
EFIAPI
RootBridgeIoFreeBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  UINTN                            Pages,
  OUT VOID                             *HostAddress
  );

/**
   Flushes all PCI posted write transactions from a PCI host bridge to system memory.

   The Flush() function flushes any PCI posted write transactions from a PCI host bridge to system
   memory. Posted write transactions are generated by PCI bus masters when they perform write
   transactions to target addresses in system memory.
   This function does not flush posted write transactions from any PCI bridges. A PCI controller
   specific action must be taken to guarantee that the posted write transactions have been flushed from
   the PCI controller and from all the PCI bridges into the PCI host bridge. This is typically done with
   a PCI read transaction from the PCI controller prior to calling Flush().

   @param This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   
   @retval EFI_SUCCESS        The PCI posted write transactions were flushed from the PCI host
                              bridge to system memory.
   @retval EFI_DEVICE_ERROR   The PCI posted write transactions were not flushed from the PCI
                              host bridge due to a hardware error.

**/
EFI_STATUS
EFIAPI
RootBridgeIoFlush (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This
  );

/**
   Gets the attributes that a PCI root bridge supports setting with SetAttributes(), and the
   attributes that a PCI root bridge is currently using.  

   The GetAttributes() function returns the mask of attributes that this PCI root bridge supports
   and the mask of attributes that the PCI root bridge is currently using.

   @param This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param Supported   A pointer to the mask of attributes that this PCI root bridge
                      supports setting with SetAttributes().
   @param Attributes  A pointer to the mask of attributes that this PCI root bridge is
                      currently using.
   
   @retval  EFI_SUCCESS           If Supports is not NULL, then the attributes that the PCI root
                                  bridge supports is returned in Supports. If Attributes is
                                  not NULL, then the attributes that the PCI root bridge is currently
                                  using is returned in Attributes.
   @retval  EFI_INVALID_PARAMETER Both Supports and Attributes are NULL.

**/
EFI_STATUS
EFIAPI
RootBridgeIoGetAttributes (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT UINT64                           *Supported,
  OUT UINT64                           *Attributes
  );

/**
   Sets attributes for a resource range on a PCI root bridge.

   The SetAttributes() function sets the attributes specified in Attributes for the PCI root
   bridge on the resource range specified by ResourceBase and ResourceLength. Since the
   granularity of setting these attributes may vary from resource type to resource type, and from
   platform to platform, the actual resource range and the one passed in by the caller may differ. As a
   result, this function may set the attributes specified by Attributes on a larger resource range
   than the caller requested. The actual range is returned in ResourceBase and
   ResourceLength. The caller is responsible for verifying that the actual range for which the
   attributes were set is acceptable.

   @param[in]       This            A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]       Attributes      The mask of attributes to set. If the attribute bit
                                    MEMORY_WRITE_COMBINE, MEMORY_CACHED, or
                                    MEMORY_DISABLE is set, then the resource range is specified by
                                    ResourceBase and ResourceLength. If
                                    MEMORY_WRITE_COMBINE, MEMORY_CACHED, and
                                    MEMORY_DISABLE are not set, then ResourceBase and
                                    ResourceLength are ignored, and may be NULL.
   @param[in, out]  ResourceBase    A pointer to the base address of the resource range to be modified
                                    by the attributes specified by Attributes.
   @param[in, out]  ResourceLength  A pointer to the length of the resource range to be modified by the
                                    attributes specified by Attributes.
   
   @retval  EFI_SUCCESS     The current configuration of this PCI root bridge was returned in Resources.
   @retval  EFI_UNSUPPORTED The current configuration of this PCI root bridge could not be retrieved.
   @retval  EFI_INVALID_PARAMETER Invalid pointer of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL

**/
EFI_STATUS
EFIAPI
RootBridgeIoSetAttributes (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN     UINT64                           Attributes,
  IN OUT UINT64                           *ResourceBase,
  IN OUT UINT64                           *ResourceLength 
  ); 

/**
   Retrieves the current resource settings of this PCI root bridge in the form of a set of ACPI 2.0
   resource descriptors.

   There are only two resource descriptor types from the ACPI Specification that may be used to
   describe the current resources allocated to a PCI root bridge. These are the QWORD Address
   Space Descriptor (ACPI 2.0 Section 6.4.3.5.1), and the End Tag (ACPI 2.0 Section 6.4.2.8). The
   QWORD Address Space Descriptor can describe memory, I/O, and bus number ranges for dynamic
   or fixed resources. The configuration of a PCI root bridge is described with one or more QWORD
   Address Space Descriptors followed by an End Tag.

   @param[in]   This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[out]  Resources   A pointer to the ACPI 2.0 resource descriptors that describe the
                            current configuration of this PCI root bridge. The storage for the
                            ACPI 2.0 resource descriptors is allocated by this function. The
                            caller must treat the return buffer as read-only data, and the buffer
                            must not be freed by the caller.
   
   @retval  EFI_SUCCESS     The current configuration of this PCI root bridge was returned in Resources.
   @retval  EFI_UNSUPPORTED The current configuration of this PCI root bridge could not be retrieved.
   @retval  EFI_INVALID_PARAMETER Invalid pointer of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL

**/
EFI_STATUS
EFIAPI
RootBridgeIoConfiguration (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT    VOID                             **Resources
  );

//
// Memory Controller Pci Root Bridge Io Module Variables
//
EFI_METRONOME_ARCH_PROTOCOL *mMetronome;

//
// Lookup table for increment values based on transfer widths
//
UINT8 mInStride[] = {
  1, // EfiPciWidthUint8
  2, // EfiPciWidthUint16
  4, // EfiPciWidthUint32
  8, // EfiPciWidthUint64
  0, // EfiPciWidthFifoUint8
  0, // EfiPciWidthFifoUint16
  0, // EfiPciWidthFifoUint32
  0, // EfiPciWidthFifoUint64
  1, // EfiPciWidthFillUint8
  2, // EfiPciWidthFillUint16
  4, // EfiPciWidthFillUint32
  8  // EfiPciWidthFillUint64
};

//
// Lookup table for increment values based on transfer widths
//
UINT8 mOutStride[] = {
  1, // EfiPciWidthUint8
  2, // EfiPciWidthUint16
  4, // EfiPciWidthUint32
  8, // EfiPciWidthUint64
  1, // EfiPciWidthFifoUint8
  2, // EfiPciWidthFifoUint16
  4, // EfiPciWidthFifoUint32
  8, // EfiPciWidthFifoUint64
  0, // EfiPciWidthFillUint8
  0, // EfiPciWidthFillUint16
  0, // EfiPciWidthFillUint32
  0  // EfiPciWidthFillUint64
};

/**

  Construct the Pci Root Bridge Io protocol

  @param Protocol         Point to protocol instance
  @param HostBridgeHandle Handle of host bridge
  @param Attri            Attribute of host bridge
  @param ResAperture      ResourceAperture for host bridge

  @retval EFI_SUCCESS Success to initialize the Pci Root Bridge.

**/
EFI_STATUS
RootBridgeConstructor (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL    *Protocol,
  IN EFI_HANDLE                         HostBridgeHandle,
  IN UINT64                             Attri,
  IN PCI_ROOT_BRIDGE_RESOURCE_APERTURE  *ResAperture
  )
{
  EFI_STATUS                        Status;
  PCI_ROOT_BRIDGE_INSTANCE          *PrivateData;
  PCI_RESOURCE_TYPE                 Index;

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (Protocol);

  //
  // The host to pci bridge, the host memory and io addresses are
  // direct mapped to pci addresses, so no need translate, set bases to 0.
  //
  PrivateData->MemBase = ResAperture->MemBase;
  PrivateData->IoBase  = ResAperture->IoBase;

  //
  // The host bridge only supports 32bit addressing for memory
  // and standard IA32 16bit io
  //
  PrivateData->MemLimit = ResAperture->MemLimit;
  PrivateData->IoLimit  = ResAperture->IoLimit;

  //
  // Bus Aperture for this Root Bridge (Possible Range)
  //
  PrivateData->BusBase  = ResAperture->BusBase;
  PrivateData->BusLimit = ResAperture->BusLimit;
  
  //
  // Specific for this chipset
  //
  for (Index = TypeIo; Index < TypeMax; Index++) {
    PrivateData->ResAllocNode[Index].Type      = Index;
    PrivateData->ResAllocNode[Index].Base      = 0;
    PrivateData->ResAllocNode[Index].Length    = 0;
    PrivateData->ResAllocNode[Index].Status    = ResNone;
  }
  
  PrivateData->RootBridgeAttrib = Attri;
  
  PrivateData->Supports    = EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO | EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO | \
                             EFI_PCI_ATTRIBUTE_ISA_IO_16 | EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO | \
                             EFI_PCI_ATTRIBUTE_VGA_MEMORY | \
                             EFI_PCI_ATTRIBUTE_VGA_IO_16  | EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;
  PrivateData->Attributes  = PrivateData->Supports;

  Protocol->ParentHandle   = HostBridgeHandle;
  
  Protocol->PollMem        = RootBridgeIoPollMem;
  Protocol->PollIo         = RootBridgeIoPollIo;

  Protocol->Mem.Read       = RootBridgeIoMemRead;
  Protocol->Mem.Write      = RootBridgeIoMemWrite;

  Protocol->Io.Read        = RootBridgeIoIoRead;
  Protocol->Io.Write       = RootBridgeIoIoWrite;

  Protocol->CopyMem        = RootBridgeIoCopyMem;

  Protocol->Pci.Read       = RootBridgeIoPciRead;
  Protocol->Pci.Write      = RootBridgeIoPciWrite;

  Protocol->Map            = RootBridgeIoMap;
  Protocol->Unmap          = RootBridgeIoUnmap;

  Protocol->AllocateBuffer = RootBridgeIoAllocateBuffer;
  Protocol->FreeBuffer     = RootBridgeIoFreeBuffer;

  Protocol->Flush          = RootBridgeIoFlush;

  Protocol->GetAttributes  = RootBridgeIoGetAttributes;
  Protocol->SetAttributes  = RootBridgeIoSetAttributes;

  Protocol->Configuration  = RootBridgeIoConfiguration;

  Protocol->SegmentNumber  = 0;

  Status = gBS->LocateProtocol (&gEfiMetronomeArchProtocolGuid, NULL, (VOID **)&mMetronome);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Check parameters for IO,MMIO,PCI read/write services of PCI Root Bridge IO.

  The I/O operations are carried out exactly as requested. The caller is responsible 
  for satisfying any alignment and I/O width restrictions that a PI System on a 
  platform might require. For example on some platforms, width requests of 
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will 
  be handled by the driver.
  
  @param[in] This           A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param[in] OperationType  I/O operation type: IO/MMIO/PCI.
  @param[in] Width          Signifies the width of the I/O or Memory operation.
  @param[in] Address        The base address of the I/O operation. 
  @param[in] Count          The number of I/O operations to perform. The number of  
                            bytes moved is Width size * Count, starting at Address.
  @param[in] Buffer         For read operations, the destination buffer to store the results.
                            For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The parameters for this request pass the checks.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width, 
                                 and Count is not valid for this PI system.

**/
EFI_STATUS
RootBridgeIoCheckParameter (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN OPERATION_TYPE                         OperationType,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 Address,
  IN UINTN                                  Count,
  IN VOID                                   *Buffer
  )
{
  PCI_ROOT_BRIDGE_INSTANCE                     *PrivateData;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS  *PciRbAddr;
  UINT64                                       MaxCount;
  UINT64                                       Base;
  UINT64                                       Limit;

  //
  // Check to see if Buffer is NULL
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if Width is in the valid range
  //
  if ((UINT32)Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // For FIFO type, the target address won't increase during the access,
  // so treat Count as 1
  //
  if (Width >= EfiPciWidthFifoUint8 && Width <= EfiPciWidthFifoUint64) {
    Count = 1;
  }

  //
  // Check to see if Width is in the valid range for I/O Port operations
  //
  Width = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (Width & 0x03);
  if ((OperationType != MemOperation) && (Width == EfiPciWidthUint64)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if Address is aligned
  //
  if ((Address & (UINT64)(mInStride[Width] - 1)) != 0) {
    return EFI_UNSUPPORTED;
  }

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  //
  // Check to see if any address associated with this transfer exceeds the maximum 
  // allowed address.  The maximum address implied by the parameters passed in is
  // Address + Size * Count.  If the following condition is met, then the transfer
  // is not supported.
  //
  //    Address + Size * Count > Limit + 1
  //
  // Since Limit can be the maximum integer value supported by the CPU and Count 
  // can also be the maximum integer value supported by the CPU, this range
  // check must be adjusted to avoid all oveflow conditions.
  //   
  // The following form of the range check is equivalent but assumes that 
  // Limit is of the form (2^n - 1).
  //
  if (OperationType == IoOperation) {
    Base = PrivateData->IoBase;
    Limit = PrivateData->IoLimit;
  } else if (OperationType == MemOperation) {
    Base = PrivateData->MemBase;
    Limit = PrivateData->MemLimit;
  } else {
    PciRbAddr = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS*) &Address;
    if (PciRbAddr->Bus < PrivateData->BusBase || PciRbAddr->Bus > PrivateData->BusLimit) {
      return EFI_INVALID_PARAMETER;
    }

    if (PciRbAddr->Device > MAX_PCI_DEVICE_NUMBER || PciRbAddr->Function > MAX_PCI_FUNCTION_NUMBER) {
      return EFI_INVALID_PARAMETER;
    }

    if (PciRbAddr->ExtendedRegister != 0) {
      Address = PciRbAddr->ExtendedRegister;
    } else {
      Address = PciRbAddr->Register;
    }
    Base = 0;
    Limit = MAX_PCI_REG_ADDRESS;
  }

  if (Address < Base) {
      return EFI_INVALID_PARAMETER;
  }

  if (Count == 0) {
    if (Address > Limit) {
      return EFI_UNSUPPORTED;
    }
  } else {  
    MaxCount = RShiftU64 (Limit, Width);
    if (MaxCount < (Count - 1)) {
      return EFI_UNSUPPORTED;
    }
    if (Address > LShiftU64 (MaxCount - Count + 1, Width)) {
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}

/**
   Internal help function for read and write memory space.

   @param[in]   This          A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Write         Switch value for Read or Write.
   @param[in]   Width         Signifies the width of the memory operations.
   @param[in]   UserAddress   The address within the PCI configuration space for the PCI controller.
   @param[in]   Count         The number of PCI configuration operations to perform. Bytes
                              moved is Width size * Count, starting at Address.
   @param[in, out] UserBuffer For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
RootBridgeIoMemRW (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     BOOLEAN                                Write,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
{
  EFI_STATUS                             Status;
  UINT8                                  InStride;
  UINT8                                  OutStride;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  OperationWidth;
  UINT8                                  *Uint8Buffer;

  Status = RootBridgeIoCheckParameter (This, MemOperation, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  InStride = mInStride[Width];
  OutStride = mOutStride[Width];
  OperationWidth = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (Width & 0x03);
  for (Uint8Buffer = Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (Write) {
      switch (OperationWidth) {
        case EfiPciWidthUint8:
          MmioWrite8 ((UINTN)Address, *Uint8Buffer);
          break;
        case EfiPciWidthUint16:
          MmioWrite16 ((UINTN)Address, *((UINT16 *)Uint8Buffer));
          break;
        case EfiPciWidthUint32:
          MmioWrite32 ((UINTN)Address, *((UINT32 *)Uint8Buffer));
          break;
        case EfiPciWidthUint64:
          MmioWrite64 ((UINTN)Address, *((UINT64 *)Uint8Buffer));
          break;
        default:
          //
          // The RootBridgeIoCheckParameter call above will ensure that this
          // path is not taken.
          //
          ASSERT (FALSE);
          break;
      }
    } else {
      switch (OperationWidth) {
        case EfiPciWidthUint8:
          *Uint8Buffer = MmioRead8 ((UINTN)Address);
          break;
        case EfiPciWidthUint16:
          *((UINT16 *)Uint8Buffer) = MmioRead16 ((UINTN)Address);
          break;
        case EfiPciWidthUint32:
          *((UINT32 *)Uint8Buffer) = MmioRead32 ((UINTN)Address);
          break;
        case EfiPciWidthUint64:
          *((UINT64 *)Uint8Buffer) = MmioRead64 ((UINTN)Address);
          break;
        default:
          //
          // The RootBridgeIoCheckParameter call above will ensure that this
          // path is not taken.
          //
          ASSERT (FALSE);
          break;
      }
    }
  }
  return EFI_SUCCESS;  
}

/**
   Internal help function for read and write IO space.

   @param[in]   This          A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Write         Switch value for Read or Write.
   @param[in]   Width         Signifies the width of the memory operations.
   @param[in]   UserAddress   The address within the PCI configuration space for the PCI controller.
   @param[in]   Count         The number of PCI configuration operations to perform. Bytes
                              moved is Width size * Count, starting at Address.
   @param[in, out] UserBuffer For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
RootBridgeIoIoRW (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     BOOLEAN                                Write,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
{
  EFI_STATUS                             Status;
  UINT8                                  InStride;
  UINT8                                  OutStride;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  OperationWidth;
  UINT8                                  *Uint8Buffer;

  Status = RootBridgeIoCheckParameter (This, IoOperation, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  InStride = mInStride[Width];
  OutStride = mOutStride[Width];
  OperationWidth = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (Width & 0x03);

#if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)
  if (InStride == 0) {
    if (Write) {
      switch (OperationWidth) {
        case EfiPciWidthUint8:
          IoWriteFifo8 ((UINTN) Address, Count, Buffer);
          return EFI_SUCCESS;
        case EfiPciWidthUint16:
          IoWriteFifo16 ((UINTN) Address, Count, Buffer);
          return EFI_SUCCESS;
        case EfiPciWidthUint32:
          IoWriteFifo32 ((UINTN) Address, Count, Buffer);
          return EFI_SUCCESS;
        default:
          //
          // The RootBridgeIoCheckParameter call above will ensure that this
          // path is not taken.
          //
          ASSERT (FALSE);
          break;
      }
    } else {
      switch (OperationWidth) {
        case EfiPciWidthUint8:
          IoReadFifo8 ((UINTN) Address, Count, Buffer);
          return EFI_SUCCESS;
        case EfiPciWidthUint16:
          IoReadFifo16 ((UINTN) Address, Count, Buffer);
          return EFI_SUCCESS;
        case EfiPciWidthUint32:
          IoReadFifo32 ((UINTN) Address, Count, Buffer);
          return EFI_SUCCESS;
        default:
          //
          // The RootBridgeIoCheckParameter call above will ensure that this
          // path is not taken.
          //
          ASSERT (FALSE);
          break;
      }
    }
  }
#endif

  for (Uint8Buffer = Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (Write) {
      switch (OperationWidth) {
        case EfiPciWidthUint8:
          IoWrite8 ((UINTN)Address, *Uint8Buffer);
          break;
        case EfiPciWidthUint16:
          IoWrite16 ((UINTN)Address, *((UINT16 *)Uint8Buffer));
          break;
        case EfiPciWidthUint32:
          IoWrite32 ((UINTN)Address, *((UINT32 *)Uint8Buffer));
          break;
        default:
          //
          // The RootBridgeIoCheckParameter call above will ensure that this
          // path is not taken.
          //
          ASSERT (FALSE);
          break;
      }
    } else {
      switch (OperationWidth) {
        case EfiPciWidthUint8:
          *Uint8Buffer = IoRead8 ((UINTN)Address);
          break;
        case EfiPciWidthUint16:
          *((UINT16 *)Uint8Buffer) = IoRead16 ((UINTN)Address);
          break;
        case EfiPciWidthUint32:
          *((UINT32 *)Uint8Buffer) = IoRead32 ((UINTN)Address);
          break;
        default:
          //
          // The RootBridgeIoCheckParameter call above will ensure that this
          // path is not taken.
          //
          ASSERT (FALSE);
          break;
      }
    }
  }
  return EFI_SUCCESS;
}

/**
   Internal help function for read and write PCI configuration space.

   @param[in]   This          A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Write         Switch value for Read or Write.
   @param[in]   Width         Signifies the width of the memory operations.
   @param[in]   UserAddress   The address within the PCI configuration space for the PCI controller.
   @param[in]   Count         The number of PCI configuration operations to perform. Bytes
                              moved is Width size * Count, starting at Address.
   @param[in, out] UserBuffer For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
RootBridgeIoPciRW (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN BOOLEAN                                Write,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 Address,
  IN UINTN                                  Count,
  IN OUT VOID                               *Buffer
  )
{
  EFI_STATUS                                   Status;
  UINT8                                        InStride;
  UINT8                                        OutStride;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH        OperationWidth;
  UINT8                                        *Uint8Buffer;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS  *PciRbAddr;
  UINTN                                        PcieRegAddr;

  Status = RootBridgeIoCheckParameter (This, PciOperation, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PciRbAddr = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS*) &Address;

  PcieRegAddr = (UINTN) PCI_LIB_ADDRESS (
                          PciRbAddr->Bus,
                          PciRbAddr->Device,
                          PciRbAddr->Function,
                          (PciRbAddr->ExtendedRegister != 0) ? \
                            PciRbAddr->ExtendedRegister :
                            PciRbAddr->Register
                          );

  InStride = mInStride[Width];
  OutStride = mOutStride[Width];
  OperationWidth = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (Width & 0x03);
  for (Uint8Buffer = Buffer; Count > 0; PcieRegAddr += InStride, Uint8Buffer += OutStride, Count--) {
    if (Write) {
      switch (OperationWidth) {
        case EfiPciWidthUint8:
          PciWrite8 (PcieRegAddr, *Uint8Buffer);
          break;
        case EfiPciWidthUint16:
          PciWrite16 (PcieRegAddr, *((UINT16 *)Uint8Buffer));
          break;
        case EfiPciWidthUint32:
          PciWrite32 (PcieRegAddr, *((UINT32 *)Uint8Buffer));
          break;
        default:
          //
          // The RootBridgeIoCheckParameter call above will ensure that this
          // path is not taken.
          //
          ASSERT (FALSE);
          break;
      }
    } else {
      switch (OperationWidth) {
        case EfiPciWidthUint8:
          *Uint8Buffer = PciRead8 (PcieRegAddr);
          break;
        case EfiPciWidthUint16:
          *((UINT16 *)Uint8Buffer) = PciRead16 (PcieRegAddr);
          break;
        case EfiPciWidthUint32:
          *((UINT32 *)Uint8Buffer) = PciRead32 (PcieRegAddr);
          break;
        default:
          //
          // The RootBridgeIoCheckParameter call above will ensure that this
          // path is not taken.
          //
          ASSERT (FALSE);
          break;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
   Polls an address in memory mapped I/O space until an exit condition is met, or 
   a timeout occurs. 

   This function provides a standard way to poll a PCI memory location. A PCI memory read
   operation is performed at the PCI memory address specified by Address for the width specified
   by Width. The result of this PCI memory read operation is stored in Result. This PCI memory
   read operation is repeated until either a timeout of Delay 100 ns units has expired, or (Result &
   Mask) is equal to Value.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operations.
   @param[in]   Address   The base address of the memory operations. The caller is
                          responsible for aligning Address if required.
   @param[in]   Mask      Mask used for the polling criteria. Bytes above Width in Mask
                          are ignored. The bits in the bytes below Width which are zero in
                          Mask are ignored when polling the memory address.
   @param[in]   Value     The comparison value used for the polling exit criteria.
   @param[in]   Delay     The number of 100 ns units to poll. Note that timer available may
                          be of poorer granularity.
   @param[out]  Result    Pointer to the last value read from the memory location.
   
   @retval EFI_SUCCESS            The last data returned from the access matched the poll exit criteria.
   @retval EFI_INVALID_PARAMETER  Width is invalid.
   @retval EFI_INVALID_PARAMETER  Result is NULL.
   @retval EFI_TIMEOUT            Delay expired before a match occurred.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoPollMem ( 
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  )
{
  EFI_STATUS  Status;
  UINT64      NumberOfTicks;
  UINT32      Remainder;

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32)Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // No matter what, always do a single poll.
  //
  Status = This->Mem.Read (This, Width, Address, 1, Result);
  if (EFI_ERROR (Status)) {
    return Status;
  }    
  if ((*Result & Mask) == Value) {
    return EFI_SUCCESS;
  }

  if (Delay == 0) {
    return EFI_SUCCESS;
  
  } else {

    //
    // Determine the proper # of metronome ticks to wait for polling the
    // location.  The nuber of ticks is Roundup (Delay / mMetronome->TickPeriod)+1
    // The "+1" to account for the possibility of the first tick being short
    // because we started in the middle of a tick.
    //
    // BugBug: overriding mMetronome->TickPeriod with UINT32 until Metronome
    // protocol definition is updated.
    //
    NumberOfTicks = DivU64x32Remainder (Delay, (UINT32) mMetronome->TickPeriod, &Remainder);
    if (Remainder != 0) {
      NumberOfTicks += 1;
    }
    NumberOfTicks += 1;
  
    while (NumberOfTicks != 0) {

      mMetronome->WaitForTick (mMetronome, 1);
    
      Status = This->Mem.Read (This, Width, Address, 1, Result);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    
      if ((*Result & Mask) == Value) {
        return EFI_SUCCESS;
      }

      NumberOfTicks -= 1;
    }
  }
  return EFI_TIMEOUT;
}
  
/**
   Reads from the I/O space of a PCI Root Bridge. Returns when either the polling exit criteria is
   satisfied or after a defined duration.

   This function provides a standard way to poll a PCI I/O location. A PCI I/O read operation is
   performed at the PCI I/O address specified by Address for the width specified by Width.
   The result of this PCI I/O read operation is stored in Result. This PCI I/O read operation is
   repeated until either a timeout of Delay 100 ns units has expired, or (Result & Mask) is equal
   to Value.

   @param[in] This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in] Width     Signifies the width of the I/O operations.
   @param[in] Address   The base address of the I/O operations. The caller is responsible
                        for aligning Address if required.
   @param[in] Mask      Mask used for the polling criteria. Bytes above Width in Mask
                        are ignored. The bits in the bytes below Width which are zero in
                        Mask are ignored when polling the I/O address.
   @param[in] Value     The comparison value used for the polling exit criteria.
   @param[in] Delay     The number of 100 ns units to poll. Note that timer available may
                        be of poorer granularity.
   @param[out] Result   Pointer to the last value read from the memory location.
   
   @retval EFI_SUCCESS            The last data returned from the access matched the poll exit criteria.
   @retval EFI_INVALID_PARAMETER  Width is invalid.
   @retval EFI_INVALID_PARAMETER  Result is NULL.
   @retval EFI_TIMEOUT            Delay expired before a match occurred.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoPollIo ( 
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  )
{
  EFI_STATUS  Status;
  UINT64      NumberOfTicks;
  UINT32      Remainder;

  //
  // No matter what, always do a single poll.
  //

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32)Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }
  
  Status = This->Io.Read (This, Width, Address, 1, Result);
  if (EFI_ERROR (Status)) {
    return Status;
  }    
  if ((*Result & Mask) == Value) {
    return EFI_SUCCESS;
  }

  if (Delay == 0) {
    return EFI_SUCCESS;
  
  } else {

    //
    // Determine the proper # of metronome ticks to wait for polling the
    // location.  The number of ticks is Roundup (Delay / mMetronome->TickPeriod)+1
    // The "+1" to account for the possibility of the first tick being short
    // because we started in the middle of a tick.
    //
    NumberOfTicks = DivU64x32Remainder (Delay, (UINT32)mMetronome->TickPeriod, &Remainder);
    if (Remainder != 0) {
      NumberOfTicks += 1;
    }
    NumberOfTicks += 1;
  
    while (NumberOfTicks != 0) {

      mMetronome->WaitForTick (mMetronome, 1);
    
      Status = This->Io.Read (This, Width, Address, 1, Result);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    
      if ((*Result & Mask) == Value) {
        return EFI_SUCCESS;
      }

      NumberOfTicks -= 1;
    }
  }
  return EFI_TIMEOUT;
}

/**
   Enables a PCI driver to access PCI controller registers in the PCI root bridge memory space.

   The Mem.Read(), and Mem.Write() functions enable a driver to access PCI controller
   registers in the PCI root bridge memory space.
   The memory operations are carried out exactly as requested. The caller is responsible for satisfying
   any alignment and memory width restrictions that a PCI Root Bridge on a platform might require.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operation.
   @param[in]   Address   The base address of the memory operation. The caller is
                          responsible for aligning the Address if required.
   @param[in]   Count     The number of memory operations to perform. Bytes moved is
                          Width size * Count, starting at Address.
   @param[out]  Buffer    For read operations, the destination buffer to store the results. For
                          write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoMemRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  OUT    VOID                                   *Buffer
  )
{
  return RootBridgeIoMemRW (This, FALSE, Width, Address, Count, Buffer);
}

/**
   Enables a PCI driver to access PCI controller registers in the PCI root bridge memory space.

   The Mem.Read(), and Mem.Write() functions enable a driver to access PCI controller
   registers in the PCI root bridge memory space.
   The memory operations are carried out exactly as requested. The caller is responsible for satisfying
   any alignment and memory width restrictions that a PCI Root Bridge on a platform might require.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operation.
   @param[in]   Address   The base address of the memory operation. The caller is
                          responsible for aligning the Address if required.
   @param[in]   Count     The number of memory operations to perform. Bytes moved is
                          Width size * Count, starting at Address.
   @param[in]   Buffer    For read operations, the destination buffer to store the results. For
                          write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
RootBridgeIoMemWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN     VOID                                   *Buffer
  )
{
  return RootBridgeIoMemRW (This, TRUE, Width, Address, Count, Buffer);  
}

/**
   Enables a PCI driver to access PCI controller registers in the PCI root bridge I/O space.

   @param[in]   This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width       Signifies the width of the memory operations.
   @param[in]   Address     The base address of the I/O operation. The caller is responsible for
                            aligning the Address if required.
   @param[in]   Count       The number of I/O operations to perform. Bytes moved is Width
                            size * Count, starting at Address.
   @param[out]  Buffer      For read operations, the destination buffer to store the results. For
                            write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS              The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER    Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER    Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoIoRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  OUT    VOID                                   *Buffer
  )
{
  return RootBridgeIoIoRW (This, FALSE, Width, Address, Count, Buffer);  
}

/**
   Enables a PCI driver to access PCI controller registers in the PCI root bridge I/O space.

   @param[in]   This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width       Signifies the width of the memory operations.
   @param[in]   Address     The base address of the I/O operation. The caller is responsible for
                            aligning the Address if required.
   @param[in]   Count       The number of I/O operations to perform. Bytes moved is Width
                            size * Count, starting at Address.
   @param[in]   Buffer       For read operations, the destination buffer to store the results. For
                            write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS              The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER    Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER    Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoIoWrite (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL         *This,
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH   Width,
  IN       UINT64                                  Address,
  IN       UINTN                                   Count,
  IN       VOID                                    *Buffer
  )
{
  return RootBridgeIoIoRW (This, TRUE, Width, Address, Count, Buffer);  
}

/**
   Enables a PCI driver to copy one region of PCI root bridge memory space to another region of PCI
   root bridge memory space.

   The CopyMem() function enables a PCI driver to copy one region of PCI root bridge memory
   space to another region of PCI root bridge memory space. This is especially useful for video scroll
   operation on a memory mapped video buffer.
   The memory operations are carried out exactly as requested. The caller is responsible for satisfying
   any alignment and memory width restrictions that a PCI root bridge on a platform might require.

   @param[in] This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
   @param[in] Width       Signifies the width of the memory operations.
   @param[in] DestAddress The destination address of the memory operation. The caller is
                          responsible for aligning the DestAddress if required.
   @param[in] SrcAddress  The source address of the memory operation. The caller is
                          responsible for aligning the SrcAddress if required.
   @param[in] Count       The number of memory operations to perform. Bytes moved is
                          Width size * Count, starting at DestAddress and SrcAddress.
   
   @retval  EFI_SUCCESS             The data was copied from one memory region to another memory region.
   @retval  EFI_INVALID_PARAMETER   Width is invalid for this PCI root bridge.
   @retval  EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoCopyMem (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH        Width,
  IN UINT64                                       DestAddress,
  IN UINT64                                       SrcAddress,
  IN UINTN                                        Count
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Direction;
  UINTN       Stride;
  UINTN       Index;
  UINT64      Result;

  if ((UINT32)Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }    

  if (DestAddress == SrcAddress) {
    return EFI_SUCCESS;
  }

  Stride = (UINTN)(1 << Width);

  Direction = TRUE;
  if ((DestAddress > SrcAddress) && (DestAddress < (SrcAddress + Count * Stride))) {
    Direction   = FALSE;
    SrcAddress  = SrcAddress  + (Count-1) * Stride;
    DestAddress = DestAddress + (Count-1) * Stride;
  }

  for (Index = 0;Index < Count;Index++) {
    Status = RootBridgeIoMemRead (
               This,
               Width,
               SrcAddress,
               1,
               &Result
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = RootBridgeIoMemWrite (
               This,
               Width,
               DestAddress,
               1,
               &Result
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    if (Direction) {
      SrcAddress  += Stride;
      DestAddress += Stride;
    } else {
      SrcAddress  -= Stride;
      DestAddress -= Stride;
    }
  }
  return EFI_SUCCESS;
}

/**
   Enables a PCI driver to access PCI controller registers in a PCI root bridge's configuration space.

   The Pci.Read() and Pci.Write() functions enable a driver to access PCI configuration
   registers for a PCI controller.
   The PCI Configuration operations are carried out exactly as requested. The caller is responsible for
   any alignment and PCI configuration width issues that a PCI Root Bridge on a platform might
   require.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operations.
   @param[in]   Address   The address within the PCI configuration space for the PCI controller.
   @param[in]   Count     The number of PCI configuration operations to perform. Bytes
                          moved is Width size * Count, starting at Address.
   @param[out]  Buffer    For read operations, the destination buffer to store the results. For
                          write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoPciRead (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN       UINT64                                 Address,
  IN       UINTN                                  Count,
  OUT      VOID                                   *Buffer
  )
{
  return RootBridgeIoPciRW (This, FALSE, Width, Address, Count, Buffer);
}

/**
   Enables a PCI driver to access PCI controller registers in a PCI root bridge's configuration space.

   The Pci.Read() and Pci.Write() functions enable a driver to access PCI configuration
   registers for a PCI controller.
   The PCI Configuration operations are carried out exactly as requested. The caller is responsible for
   any alignment and PCI configuration width issues that a PCI Root Bridge on a platform might
   require.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operations.
   @param[in]   Address   The address within the PCI configuration space for the PCI controller.
   @param[in]   Count     The number of PCI configuration operations to perform. Bytes
                          moved is Width size * Count, starting at Address.
   @param[in]   Buffer    For read operations, the destination buffer to store the results. For
                          write operations, the source buffer to write data from.
   
   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoPciWrite (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN       UINT64                                 Address,
  IN       UINTN                                  Count,
  IN       VOID                                   *Buffer
  )
{
  return RootBridgeIoPciRW (This, TRUE, Width, Address, Count, Buffer);
}

/**
   Provides the PCI controller-specific addresses required to access system memory from a
   DMA bus master.

   The Map() function provides the PCI controller specific addresses needed to access system
   memory. This function is used to map system memory for PCI bus master DMA accesses.

   @param[in]       This            A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]       Operation       Indicates if the bus master is going to read or write to system memory.
   @param[in]       HostAddress     The system memory address to map to the PCI controller.
   @param[in, out]  NumberOfBytes   On input the number of bytes to map. On output the number of bytes that were mapped.
   @param[out]      DeviceAddress   The resulting map address for the bus master PCI controller to use
                                    to access the system memory's HostAddress.
   @param[out]      Mapping         The value to pass to Unmap() when the bus master DMA operation is complete.
   
   @retval EFI_SUCCESS            The range was mapped for the returned NumberOfBytes.
   @retval EFI_INVALID_PARAMETER  Operation is invalid.
   @retval EFI_INVALID_PARAMETER  HostAddress is NULL.
   @retval EFI_INVALID_PARAMETER  NumberOfBytes is NULL.
   @retval EFI_INVALID_PARAMETER  DeviceAddress is NULL.
   @retval EFI_INVALID_PARAMETER  Mapping is NULL.
   @retval EFI_UNSUPPORTED        The HostAddress cannot be mapped as a common buffer.
   @retval EFI_DEVICE_ERROR       The system hardware could not map the requested address.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoMap (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL            *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  MAP_INFO              *MapInfo;

  if (HostAddress == NULL || NumberOfBytes == NULL || DeviceAddress == NULL || Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Initialize the return values to their defaults
  //
  *Mapping = NULL;

  //
  // Make sure that Operation is valid
  //
  if ((UINT32)Operation >= EfiPciOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Most PCAT like chipsets can not handle performing DMA above 4GB.
  // If any part of the DMA transfer being mapped is above 4GB, then
  // map the DMA transfer to a buffer below 4GB.
  //
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress;
  if ((PhysicalAddress + *NumberOfBytes) > 0x100000000ULL) {

    //
    // Common Buffer operations can not be remapped.  If the common buffer
    // if above 4GB, then it is not possible to generate a mapping, so return 
    // an error.
    //
    if (Operation == EfiPciOperationBusMasterCommonBuffer || Operation == EfiPciOperationBusMasterCommonBuffer64) {
      return EFI_UNSUPPORTED;
    }

    //
    // Allocate a MAP_INFO structure to remember the mapping when Unmap() is
    // called later.
    //
    Status = gBS->AllocatePool (
                    EfiBootServicesData, 
                    sizeof(MAP_INFO), 
                    (VOID **)&MapInfo
                    );
    if (EFI_ERROR (Status)) {
      *NumberOfBytes = 0;
      return Status;
    }

    //
    // Return a pointer to the MAP_INFO structure in Mapping
    //
    *Mapping = MapInfo;

    //
    // Initialize the MAP_INFO structure
    //
    MapInfo->Operation         = Operation;
    MapInfo->NumberOfBytes     = *NumberOfBytes;
    MapInfo->NumberOfPages     = EFI_SIZE_TO_PAGES(*NumberOfBytes);
    MapInfo->HostAddress       = PhysicalAddress;
    MapInfo->MappedHostAddress = 0x00000000ffffffff;

    //
    // Allocate a buffer below 4GB to map the transfer to.
    //
    Status = gBS->AllocatePages (
                    AllocateMaxAddress, 
                    EfiBootServicesData, 
                    MapInfo->NumberOfPages,
                    &MapInfo->MappedHostAddress
                    );
    if (EFI_ERROR (Status)) {
      gBS->FreePool (MapInfo);
      *NumberOfBytes = 0;
      return Status;
    }

    //
    // If this is a read operation from the Bus Master's point of view,
    // then copy the contents of the real buffer into the mapped buffer
    // so the Bus Master can read the contents of the real buffer.
    //
    if (Operation == EfiPciOperationBusMasterRead || Operation == EfiPciOperationBusMasterRead64) {
      CopyMem (
        (VOID *)(UINTN)MapInfo->MappedHostAddress, 
        (VOID *)(UINTN)MapInfo->HostAddress,
        MapInfo->NumberOfBytes
        );
    }

    //
    // The DeviceAddress is the address of the maped buffer below 4GB
    //
    *DeviceAddress = MapInfo->MappedHostAddress;
  } else {
    //
    // The transfer is below 4GB, so the DeviceAddress is simply the HostAddress
    //
    *DeviceAddress = PhysicalAddress;
  }

  return EFI_SUCCESS;
}

/**
   Completes the Map() operation and releases any corresponding resources.

   The Unmap() function completes the Map() operation and releases any corresponding resources.
   If the operation was an EfiPciOperationBusMasterWrite or
   EfiPciOperationBusMasterWrite64, the data is committed to the target system memory.
   Any resources used for the mapping are freed.  

   @param[in] This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in] Mapping   The mapping value returned from Map().
   
   @retval EFI_SUCCESS            The range was unmapped.
   @retval EFI_INVALID_PARAMETER  Mapping is not a value that was returned by Map().
   @retval EFI_DEVICE_ERROR       The data was not committed to the target system memory.

**/
EFI_STATUS
EFIAPI
RootBridgeIoUnmap (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN VOID                             *Mapping
  )
{
  MAP_INFO    *MapInfo;

  //
  // See if the Map() operation associated with this Unmap() required a mapping buffer.
  // If a mapping buffer was not required, then this function simply returns EFI_SUCCESS.
  //
  if (Mapping != NULL) {
    //
    // Get the MAP_INFO structure from Mapping
    //
    MapInfo = (MAP_INFO *)Mapping;

    //
    // If this is a write operation from the Bus Master's point of view,
    // then copy the contents of the mapped buffer into the real buffer
    // so the processor can read the contents of the real buffer.
    //
    if (MapInfo->Operation == EfiPciOperationBusMasterWrite || MapInfo->Operation == EfiPciOperationBusMasterWrite64) {
      CopyMem (
        (VOID *)(UINTN)MapInfo->HostAddress, 
        (VOID *)(UINTN)MapInfo->MappedHostAddress,
        MapInfo->NumberOfBytes
        );
    }

    //
    // Free the mapped buffer and the MAP_INFO structure.
    //
    gBS->FreePages (MapInfo->MappedHostAddress, MapInfo->NumberOfPages);
    gBS->FreePool (Mapping);
  }
  return EFI_SUCCESS;
}

/**
   Allocates pages that are suitable for an EfiPciOperationBusMasterCommonBuffer or
   EfiPciOperationBusMasterCommonBuffer64 mapping.
  
   @param This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param Type        This parameter is not used and must be ignored.
   @param MemoryType  The type of memory to allocate, EfiBootServicesData or EfiRuntimeServicesData.
   @param Pages       The number of pages to allocate.
   @param HostAddress A pointer to store the base system memory address of the allocated range.
   @param Attributes  The requested bit mask of attributes for the allocated range. Only
                      the attributes EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE, EFI_PCI_ATTRIBUTE_MEMORY_CACHED, 
                      and EFI_PCI_ATTRIBUTE_DUAL_ADDRESS_CYCLE may be used with this function.
   
   @retval EFI_SUCCESS            The requested memory pages were allocated.
   @retval EFI_INVALID_PARAMETER  MemoryType is invalid.
   @retval EFI_INVALID_PARAMETER  HostAddress is NULL.
   @retval EFI_UNSUPPORTED        Attributes is unsupported. The only legal attribute bits are
                                  MEMORY_WRITE_COMBINE, MEMORY_CACHED, and DUAL_ADDRESS_CYCLE.
   @retval EFI_OUT_OF_RESOURCES   The memory pages could not be allocated.

**/
EFI_STATUS
EFIAPI
RootBridgeIoAllocateBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE                Type,
  IN  EFI_MEMORY_TYPE                  MemoryType,
  IN  UINTN                            Pages,
  OUT VOID                             **HostAddress,
  IN  UINT64                           Attributes
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;

  //
  // Validate Attributes
  //
  if ((Attributes & EFI_PCI_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check for invalid inputs
  //
  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
 
  //
  // The only valid memory types are EfiBootServicesData and EfiRuntimeServicesData
  //
  if (MemoryType != EfiBootServicesData && MemoryType != EfiRuntimeServicesData) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Limit allocations to memory below 4GB
  //
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS)(0xffffffff);

  Status = gBS->AllocatePages (AllocateMaxAddress, MemoryType, Pages, &PhysicalAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *HostAddress = (VOID *)(UINTN)PhysicalAddress;

  return EFI_SUCCESS;
}

/**
   Frees memory that was allocated with AllocateBuffer().

   The FreeBuffer() function frees memory that was allocated with AllocateBuffer().

   @param This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param Pages       The number of pages to free.
   @param HostAddress The base system memory address of the allocated range.
   
   @retval EFI_SUCCESS            The requested memory pages were freed.
   @retval EFI_INVALID_PARAMETER  The memory range specified by HostAddress and Pages
                                  was not allocated with AllocateBuffer().

**/
EFI_STATUS
EFIAPI
RootBridgeIoFreeBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  UINTN                            Pages,
  OUT VOID                             *HostAddress
  )
{
  return gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress, Pages);
}

/**
   Flushes all PCI posted write transactions from a PCI host bridge to system memory.

   The Flush() function flushes any PCI posted write transactions from a PCI host bridge to system
   memory. Posted write transactions are generated by PCI bus masters when they perform write
   transactions to target addresses in system memory.
   This function does not flush posted write transactions from any PCI bridges. A PCI controller
   specific action must be taken to guarantee that the posted write transactions have been flushed from
   the PCI controller and from all the PCI bridges into the PCI host bridge. This is typically done with
   a PCI read transaction from the PCI controller prior to calling Flush().

   @param This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   
   @retval EFI_SUCCESS        The PCI posted write transactions were flushed from the PCI host
                              bridge to system memory.
   @retval EFI_DEVICE_ERROR   The PCI posted write transactions were not flushed from the PCI
                              host bridge due to a hardware error.

**/
EFI_STATUS
EFIAPI
RootBridgeIoFlush (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This
  )
{
  //
  // not supported yet
  //
  return EFI_SUCCESS;
}

/**
   Gets the attributes that a PCI root bridge supports setting with SetAttributes(), and the
   attributes that a PCI root bridge is currently using.  

   The GetAttributes() function returns the mask of attributes that this PCI root bridge supports
   and the mask of attributes that the PCI root bridge is currently using.

   @param This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param Supported   A pointer to the mask of attributes that this PCI root bridge
                      supports setting with SetAttributes().
   @param Attributes  A pointer to the mask of attributes that this PCI root bridge is
                      currently using.
   
   @retval  EFI_SUCCESS           If Supports is not NULL, then the attributes that the PCI root
                                  bridge supports is returned in Supports. If Attributes is
                                  not NULL, then the attributes that the PCI root bridge is currently
                                  using is returned in Attributes.
   @retval  EFI_INVALID_PARAMETER Both Supports and Attributes are NULL.

**/
EFI_STATUS
EFIAPI
RootBridgeIoGetAttributes (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT UINT64                           *Supported,
  OUT UINT64                           *Attributes
  )
{
  PCI_ROOT_BRIDGE_INSTANCE *PrivateData;

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  if (Attributes == NULL && Supported == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set the return value for Supported and Attributes
  //
  if (Supported != NULL) {
    *Supported  = PrivateData->Supports; 
  }

  if (Attributes != NULL) {
    *Attributes = PrivateData->Attributes;
  }
  
  return EFI_SUCCESS;
}

/**
   Sets attributes for a resource range on a PCI root bridge.

   The SetAttributes() function sets the attributes specified in Attributes for the PCI root
   bridge on the resource range specified by ResourceBase and ResourceLength. Since the
   granularity of setting these attributes may vary from resource type to resource type, and from
   platform to platform, the actual resource range and the one passed in by the caller may differ. As a
   result, this function may set the attributes specified by Attributes on a larger resource range
   than the caller requested. The actual range is returned in ResourceBase and
   ResourceLength. The caller is responsible for verifying that the actual range for which the
   attributes were set is acceptable.

   @param[in]       This            A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]       Attributes      The mask of attributes to set. If the attribute bit
                                    MEMORY_WRITE_COMBINE, MEMORY_CACHED, or
                                    MEMORY_DISABLE is set, then the resource range is specified by
                                    ResourceBase and ResourceLength. If
                                    MEMORY_WRITE_COMBINE, MEMORY_CACHED, and
                                    MEMORY_DISABLE are not set, then ResourceBase and
                                    ResourceLength are ignored, and may be NULL.
   @param[in, out]  ResourceBase    A pointer to the base address of the resource range to be modified
                                    by the attributes specified by Attributes.
   @param[in, out]  ResourceLength  A pointer to the length of the resource range to be modified by the
                                    attributes specified by Attributes.
   
   @retval  EFI_SUCCESS     The current configuration of this PCI root bridge was returned in Resources.
   @retval  EFI_UNSUPPORTED The current configuration of this PCI root bridge could not be retrieved.
   @retval  EFI_INVALID_PARAMETER Invalid pointer of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL

**/
EFI_STATUS
EFIAPI
RootBridgeIoSetAttributes (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN     UINT64                           Attributes,
  IN OUT UINT64                           *ResourceBase,
  IN OUT UINT64                           *ResourceLength 
  )
{
  PCI_ROOT_BRIDGE_INSTANCE            *PrivateData;
  
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);
  
  if (Attributes != 0) {
    if ((Attributes & (~(PrivateData->Supports))) != 0) {
      return EFI_UNSUPPORTED;
    }
  }
  
  //
  // This is a generic driver for a PC-AT class system.  It does not have any
  // chipset specific knowlegde, so none of the attributes can be set or 
  // cleared.  Any attempt to set attribute that are already set will succeed, 
  // and any attempt to set an attribute that is not supported will fail.
  //
  if (Attributes & (~PrivateData->Attributes)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
   Retrieves the current resource settings of this PCI root bridge in the form of a set of ACPI 2.0
   resource descriptors.

   There are only two resource descriptor types from the ACPI Specification that may be used to
   describe the current resources allocated to a PCI root bridge. These are the QWORD Address
   Space Descriptor (ACPI 2.0 Section 6.4.3.5.1), and the End Tag (ACPI 2.0 Section 6.4.2.8). The
   QWORD Address Space Descriptor can describe memory, I/O, and bus number ranges for dynamic
   or fixed resources. The configuration of a PCI root bridge is described with one or more QWORD
   Address Space Descriptors followed by an End Tag.

   @param[in]   This        A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[out]  Resources   A pointer to the ACPI 2.0 resource descriptors that describe the
                            current configuration of this PCI root bridge. The storage for the
                            ACPI 2.0 resource descriptors is allocated by this function. The
                            caller must treat the return buffer as read-only data, and the buffer
                            must not be freed by the caller.
   
   @retval  EFI_SUCCESS     The current configuration of this PCI root bridge was returned in Resources.
   @retval  EFI_UNSUPPORTED The current configuration of this PCI root bridge could not be retrieved.
   @retval  EFI_INVALID_PARAMETER Invalid pointer of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL

**/
EFI_STATUS
EFIAPI
RootBridgeIoConfiguration (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL     *This,
  OUT VOID                                **Resources
  )
{
  PCI_ROOT_BRIDGE_INSTANCE              *PrivateData;
  UINTN                                 Index;

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);
  
  for (Index = 0; Index < TypeMax; Index++) {
    if (PrivateData->ResAllocNode[Index].Status == ResAllocated) {
      Configuration.SpaceDesp[Index].AddrRangeMin = PrivateData->ResAllocNode[Index].Base;
      Configuration.SpaceDesp[Index].AddrRangeMax = PrivateData->ResAllocNode[Index].Base + PrivateData->ResAllocNode[Index].Length - 1;
      Configuration.SpaceDesp[Index].AddrLen      = PrivateData->ResAllocNode[Index].Length;
    }  
  }  
    
  *Resources = &Configuration;      
  return EFI_SUCCESS;
}

