/** @file
  EFI PCI IO protocol functions declaration for PCI Bus module.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_PCI_IO_PROTOCOL_H_
#define _EFI_PCI_IO_PROTOCOL_H_

/**
  Initializes a PCI I/O Instance.

  @param PciIoDevice    Pci device instance.

**/
VOID
InitializePciIoInstance (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  Verifies access to a PCI Base Address Register (BAR).

  @param PciIoDevice  Pci device instance.
  @param BarIndex     The BAR index of the standard PCI Configuration header to use as the
                      base address for the memory or I/O operation to perform.
  @param Type         Operation type could be memory or I/O.
  @param Width        Signifies the width of the memory or I/O operations.
  @param Count        The number of memory or I/O operations to perform.
  @param Offset       The offset within the PCI configuration space for the PCI controller.

  @retval EFI_INVALID_PARAMETER Invalid Width/BarIndex or Bar type.
  @retval EFI_SUCCESS           Successfully verified.

**/
EFI_STATUS
PciIoVerifyBarAccess (
  IN PCI_IO_DEVICE                 *PciIoDevice,
  IN UINT8                         BarIndex,
  IN PCI_BAR_TYPE                  Type,
  IN IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN IN UINTN                      Count,
  IN UINT64                        *Offset
  );

/**
  Verifies access to a PCI Configuration Header.

  @param PciIoDevice  Pci device instance.
  @param Width        Signifies the width of the memory or I/O operations.
  @param Count        The number of memory or I/O operations to perform.
  @param Offset       The offset within the PCI configuration space for the PCI controller.

  @retval EFI_INVALID_PARAMETER  Invalid Width
  @retval EFI_UNSUPPORTED        Offset overflowed.
  @retval EFI_SUCCESS            Successfully verified.

**/
EFI_STATUS
PciIoVerifyConfigAccess (
  IN PCI_IO_DEVICE              *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  IN UINT64                     *Offset
  );

/**
  Reads from the memory space of a PCI controller. Returns either when the polling exit criteria is
  satisfied or after a defined duration.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory operation.
  @param  Mask                  Mask used for the polling criteria.
  @param  Value                 The comparison value used for the polling exit criteria.
  @param  Delay                 The number of 100 ns units to poll.
  @param  Result                Pointer to the last value read from the memory location.

  @retval EFI_SUCCESS           The last data returned from the access matched the poll exit criteria.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       Offset is not valid for the BarIndex of this PCI controller.
  @retval EFI_TIMEOUT           Delay expired before a match occurred.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciIoPollMem (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  );

/**
  Reads from the memory space of a PCI controller. Returns either when the polling exit criteria is
  satisfied or after a defined duration.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory operation.
  @param  Mask                  Mask used for the polling criteria.
  @param  Value                 The comparison value used for the polling exit criteria.
  @param  Delay                 The number of 100 ns units to poll.
  @param  Result                Pointer to the last value read from the memory location.

  @retval EFI_SUCCESS           The last data returned from the access matched the poll exit criteria.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       Offset is not valid for the BarIndex of this PCI controller.
  @retval EFI_TIMEOUT           Delay expired before a match occurred.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciIoPollIo (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  );

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciIoMemRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  );

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciIoMemWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  );

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciIoIoRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  );

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciIoIoWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  );

/**
  Enable a PCI driver to access PCI controller registers in PCI configuration space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  Offset                The offset within the PCI configuration space for the PCI controller.
  @param  Count                 The number of PCI configuration operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.


  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI configuration header of the PCI controller.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Buffer is NULL or Width is invalid.

**/
EFI_STATUS
EFIAPI
PciIoConfigRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  );

/**
  Enable a PCI driver to access PCI controller registers in PCI configuration space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  Offset                The offset within the PCI configuration space for the PCI controller.
  @param  Count                 The number of PCI configuration operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.


  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI configuration header of the PCI controller.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Buffer is NULL or Width is invalid.

**/
EFI_STATUS
EFIAPI
PciIoConfigWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  );

/**
  Enables a PCI driver to copy one region of PCI memory space to another region of PCI
  memory space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  DestBarIndex          The BAR index in the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  DestOffset            The destination offset within the BAR specified by DestBarIndex to
                                start the memory writes for the copy operation.
  @param  SrcBarIndex           The BAR index in the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  SrcOffset             The source offset within the BAR specified by SrcBarIndex to start
                                the memory reads for the copy operation.
  @param  Count                 The number of memory operations to perform. Bytes moved is Width
                                size * Count, starting at DestOffset and SrcOffset.

  @retval EFI_SUCCESS           The data was copied from one memory region to another memory region.
  @retval EFI_UNSUPPORTED       DestBarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       SrcBarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by DestOffset, Width, and Count
                                is not valid for the PCI BAR specified by DestBarIndex.
  @retval EFI_UNSUPPORTED       The address range specified by SrcOffset, Width, and Count is
                                not valid for the PCI BAR specified by SrcBarIndex.
  @retval EFI_INVALID_PARAMETER Width is invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
PciIoCopyMem (
  IN EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      DestBarIndex,
  IN     UINT64                     DestOffset,
  IN     UINT8                      SrcBarIndex,
  IN     UINT64                     SrcOffset,
  IN     UINTN                      Count
  );

/**
  Provides the PCI controller-specific addresses needed to access system memory.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
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
EFI_STATUS
EFIAPI
PciIoMap (
  IN     EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.

**/
EFI_STATUS
EFIAPI
PciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
  );

/**
  Allocates pages that are suitable for an EfiPciIoOperationBusMasterCommonBuffer
  or EfiPciOperationBusMasterCommonBuffer64 mapping.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
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
EFI_STATUS
EFIAPI
PciIoAllocateBuffer (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE    Type,
  IN  EFI_MEMORY_TYPE      MemoryType,
  IN  UINTN                Pages,
  OUT VOID                 **HostAddress,
  IN  UINT64               Attributes
  );

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
EFI_STATUS
EFIAPI
PciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  UINTN                Pages,
  IN  VOID                 *HostAddress
  );

/**
  Flushes all PCI posted write transactions from a PCI host bridge to system memory.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.

  @retval EFI_SUCCESS           The PCI posted write transactions were flushed from the PCI host
                                bridge to system memory.
  @retval EFI_DEVICE_ERROR      The PCI posted write transactions were not flushed from the PCI
                                host bridge due to a hardware error.

**/
EFI_STATUS
EFIAPI
PciIoFlush (
  IN  EFI_PCI_IO_PROTOCOL  *This
  );

/**
  Retrieves this PCI controller's current PCI bus number, device number, and function number.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  SegmentNumber         The PCI controller's current PCI segment number.
  @param  BusNumber             The PCI controller's current PCI bus number.
  @param  DeviceNumber          The PCI controller's current PCI device number.
  @param  FunctionNumber        The PCI controller's current PCI function number.

  @retval EFI_SUCCESS           The PCI controller location was returned.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciIoGetLocation (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  OUT UINTN                *Segment,
  OUT UINTN                *Bus,
  OUT UINTN                *Device,
  OUT UINTN                *Function
  );

/**
  Check BAR type for PCI resource.

  @param PciIoDevice   PCI device instance.
  @param BarIndex      The BAR index of the standard PCI Configuration header to use as the
                       base address for the memory or I/O operation to perform.
  @param BarType       Memory or I/O.

  @retval TRUE         Pci device's bar type is same with input BarType.
  @retval TRUE         Pci device's bar type is not same with input BarType.

**/
BOOLEAN
CheckBarType (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINT8          BarIndex,
  IN PCI_BAR_TYPE   BarType
  );

/**
  Set/Disable new attributes to a Root Bridge.

  @param  PciIoDevice  Pci device instance.
  @param  Attributes   New attribute want to be set.
  @param  Operation    Set or Disable.

  @retval  EFI_UNSUPPORTED  If root bridge does not support change attribute.
  @retval  EFI_SUCCESS      Successfully set new attributes.

**/
EFI_STATUS
ModifyRootBridgeAttributes (
  IN  PCI_IO_DEVICE                            *PciIoDevice,
  IN  UINT64                                   Attributes,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation
  );

/**
  Check whether this device can be enable/disable to snoop.

  @param PciIoDevice  Pci device instance.
  @param Operation    Enable/Disable.

  @retval EFI_UNSUPPORTED  Pci device is not GFX device or not support snoop.
  @retval EFI_SUCCESS      Snoop can be supported.

**/
EFI_STATUS
SupportPaletteSnoopAttributes (
  IN PCI_IO_DEVICE                            *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation
  );

/**
  Performs an operation on the attributes that this PCI controller supports. The operations include
  getting the set of supported attributes, retrieving the current attributes, setting the current
  attributes, enabling attributes, and disabling attributes.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Operation             The operation to perform on the attributes for this PCI controller.
  @param  Attributes            The mask of attributes that are used for Set, Enable, and Disable
                                operations.
  @param  Result                A pointer to the result mask of attributes that are returned for the Get
                                and Supported operations.

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       one or more of the bits set in
                                Attributes are not supported by this PCI controller or one of
                                its parent bridges when Operation is Set, Enable or Disable.

**/
EFI_STATUS
EFIAPI
PciIoAttributes (
  IN EFI_PCI_IO_PROTOCOL                       *This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  );

/**
  Gets the attributes that this PCI controller supports setting on a BAR using
  SetBarAttributes(), and retrieves the list of resource descriptors for a BAR.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for resource range. The legal range for this field is 0..5.
  @param  Supports              A pointer to the mask of attributes that this PCI controller supports
                                setting for this BAR with SetBarAttributes().
  @param  Resources             A pointer to the resource descriptors that describe the current
                                configuration of this BAR of the PCI controller.

  @retval EFI_SUCCESS           If Supports is not NULL, then the attributes that the PCI
                                controller supports are returned in Supports. If Resources
                                is not NULL, then the resource descriptors that the PCI
                                controller is currently using are returned in Resources.
  @retval EFI_INVALID_PARAMETER Both Supports and Attributes are NULL.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to allocate
                                Resources.

**/
EFI_STATUS
EFIAPI
PciIoGetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL  *This,
  IN  UINT8               BarIndex,
  OUT UINT64              *Supports  OPTIONAL,
  OUT VOID                **Resources OPTIONAL
  );

/**
  Sets the attributes for a range of a BAR on a PCI controller.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Attributes            The mask of attributes to set for the resource range specified by
                                BarIndex, Offset, and Length.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for resource range. The legal range for this field is 0..5.
  @param  Offset                A pointer to the BAR relative base address of the resource range to be
                                modified by the attributes specified by Attributes.
  @param  Length                A pointer to the length of the resource range to be modified by the
                                attributes specified by Attributes.

  @retval EFI_SUCCESS           The set of attributes specified by Attributes for the resource
                                range specified by BarIndex, Offset, and Length were
                                set on the PCI controller, and the actual resource range is returned
                                in Offset and Length.
  @retval EFI_INVALID_PARAMETER Offset or Length is NULL.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources to set the attributes on the
                                resource range specified by BarIndex, Offset, and
                                Length.

**/
EFI_STATUS
EFIAPI
PciIoSetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL  *This,
  IN     UINT64           Attributes,
  IN     UINT8            BarIndex,
  IN OUT UINT64           *Offset,
  IN OUT UINT64           *Length
  );

/**
  Test whether two Pci devices has same parent bridge.

  @param PciDevice1  The first pci device for testing.
  @param PciDevice2  The second pci device for testing.

  @retval TRUE       Two Pci device has the same parent bridge.
  @retval FALSE      Two Pci device has not the same parent bridge.

**/
BOOLEAN
PciDevicesOnTheSamePath (
  IN PCI_IO_DEVICE  *PciDevice1,
  IN PCI_IO_DEVICE  *PciDevice2
  );

#endif
