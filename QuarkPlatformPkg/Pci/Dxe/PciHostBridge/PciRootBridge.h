/** @file
The PCI Root Bridge header file.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _PCI_ROOT_BRIDGE_H_
#define _PCI_ROOT_BRIDGE_H_

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Pci.h>
#include <PciHostResource.h>

//
// Driver Consumed Protocol Prototypes
//
#include <Protocol/Metronome.h>
#include <Protocol/CpuIo2.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Runtime.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>


//
// Define the region of memory used for DMA memory
//
#define DMA_MEMORY_TOP          0x0000000001FFFFFFULL

//
// The number of PCI root bridges
//
#define ROOT_BRIDGE_COUNT  1

//
// The default latency for controllers
//
#define DEFAULT_PCI_LATENCY 0x20

//
// Define resource status constant
//
typedef struct {
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION Operation;
  UINTN                                     NumberOfBytes;
  UINTN                                     NumberOfPages;
  EFI_PHYSICAL_ADDRESS                      HostAddress;
  EFI_PHYSICAL_ADDRESS                      MappedHostAddress;
} MAP_INFO;

typedef struct {
  ACPI_HID_DEVICE_PATH      AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;

#define PCI_ROOT_BRIDGE_SIGNATURE SIGNATURE_32 ('e', '2', 'p', 'b')

typedef struct {
  UINT32                            Signature;
  EFI_LIST_ENTRY                    Link;
  EFI_HANDLE                        Handle;
  UINT64                            RootBridgeAllocAttrib;
  UINT64                            Attributes;
  UINT64                            Supports;
  PCI_RES_NODE                      ResAllocNode[6];
  PCI_ROOT_BRIDGE_RESOURCE_APERTURE Aperture;
  EFI_LOCK                          PciLock;
  UINTN                             PciAddress;
  UINTN                             PciData;
  UINT32                            HecBase;
  UINT32                            HecLen;
  UINTN                             BusScanCount;
  BOOLEAN                           BusNumberAssigned;
  VOID                              *ConfigBuffer;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   Io;
} PCI_ROOT_BRIDGE_INSTANCE;

//
// Driver Instance Data Macros
//
#define DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(a) CR (a, PCI_ROOT_BRIDGE_INSTANCE, Io, PCI_ROOT_BRIDGE_SIGNATURE)

#define DRIVER_INSTANCE_FROM_LIST_ENTRY(a)              CR (a, PCI_ROOT_BRIDGE_INSTANCE, Link, PCI_ROOT_BRIDGE_SIGNATURE)

EFI_STATUS
SimpleIioRootBridgeConstructor (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL    *Protocol,
  IN EFI_HANDLE                         HostBridgeHandle,
  IN PCI_ROOT_BRIDGE_RESOURCE_APERTURE  *ResAppeture,
  IN UINT64                             AllocAttributes
  )
/*++

Routine Description:

  Construct the Pci Root Bridge Io protocol.

Arguments:

  Protocol          -  Protocol to initialize.
  HostBridgeHandle  -  Handle to the HostBridge.
  ResAppeture       -  Resource apperture of the root bridge.
  AllocAttributes   -  Attribute of resouce allocated.

Returns:

  EFI_SUCCESS  -  Success.
  Others       -  Fail.

--*/
;

//
// Protocol Member Function Prototypes
//
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
/*++

Routine Description:

  Poll an address in memory mapped space until an exit condition is met
  or a timeout occurs.

Arguments:

  This     -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Width    -  Width of the memory operation.
  Address  -  The base address of the memory operation.
  Mask     -  Mask used for polling criteria.
  Value    -  Comparison value used for polling exit criteria.
  Delay    -  Number of 100ns units to poll.
  Result   -  Pointer to the last value read from memory location.

Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_TIMEOUT            -  Delay expired before a match occurred.
  EFI_OUT_OF_RESOURCES   -  Fail due to lack of resources.

--*/
;

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
/*++

Routine Description:

  Poll an address in I/O space until an exit condition is met
  or a timeout occurs.

Arguments:

  This     -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Width    -  Width of I/O operation.
  Address  -  The base address of the I/O operation.
  Mask     -  Mask used for polling criteria.
  Value    -  Comparison value used for polling exit criteria.
  Delay    -  Number of 100ns units to poll.
  Result   -  Pointer to the last value read from memory location.

Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_TIMEOUT            -  Delay expired before a match occurred.
  EFI_OUT_OF_RESOURCES   -  Fail due to lack of resources.

--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoMemRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
/*++

Routine Description:

  Allow read from memory mapped I/O space.

Arguments:

  This     -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Width    -  The width of memory operation.
  Address  -  Base address of the memory operation.
  Count    -  Number of memory opeartion to perform.
  Buffer   -  The destination buffer to store data.

Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_OUT_OF_RESOURCES   -  Fail due to lack of resources.

--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoMemWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
/*++

Routine Description:

  Allow write to memory mapped I/O space.

Arguments:

  This     -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Width    -  The width of memory operation.
  Address  -  Base address of the memory operation.
  Count    -  Number of memory opeartion to perform.
  Buffer   -  The source buffer to write data from.

Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_OUT_OF_RESOURCES   -  Fail due to lack of resources.

--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoIoRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *UserBuffer
  )
/*++

Routine Description:

  Enable a PCI driver to read PCI controller registers in the
  PCI root bridge I/O space.

Arguments:

  This         -  A pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  Width        -  Signifies the width of the memory operation.
  UserAddress  -  The base address of the I/O operation.
  Count        -  The number of I/O operations to perform.
  UserBuffer   -  The destination buffer to store the results.

Returns:

  EFI_SUCCESS            -  The data was read from the PCI root bridge.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of
                            resources.
--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoIoWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *UserBuffer
  )
/*++

Routine Description:

  Enable a PCI driver to write to PCI controller registers in the
  PCI root bridge I/O space.

Arguments:

  This         -  A pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  Width        -  Signifies the width of the memory operation.
  UserAddress  -  The base address of the I/O operation.
  Count        -  The number of I/O operations to perform.
  UserBuffer   -  The source buffer to write data from.

Returns:

  EFI_SUCCESS            -  The data was written to the PCI root bridge.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of
                            resources.
--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoCopyMem (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL          *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN UINT64                                   DestAddress,
  IN UINT64                                   SrcAddress,
  IN UINTN                                    Count
  )
/*++

Routine Description:

  Copy one region of PCI root bridge memory space to be copied to
  another region of PCI root bridge memory space.

Arguments:

  This         -  A pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Width        -  Signifies the width of the memory operation.
  DestAddress  -  Destination address of the memory operation.
  SrcAddress   -  Source address of the memory operation.
  Count        -  Number of memory operations to perform.

Returns:

  EFI_SUCCESS            -  The data was copied successfully.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of
                            resources.
--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoPciRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
/*++

Routine Description:

  Allows read from PCI configuration space.

Arguments:

  This     -  A pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  Width    -  Signifies the width of the memory operation.
  Address  -  The address within the PCI configuration space
              for the PCI controller.
  Count    -  The number of PCI configuration operations
              to perform.
  Buffer   -  The destination buffer to store the results.

Returns:

  EFI_SUCCESS            -  The data was read from the PCI root bridge.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of
                            resources.
--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoPciWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
/*++

Routine Description:

  Allows write to PCI configuration space.

Arguments:

  This     -  A pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  Width    -  Signifies the width of the memory operation.
  Address  -  The address within the PCI configuration space
              for the PCI controller.
  Count    -  The number of PCI configuration operations
              to perform.
  Buffer   -  The source buffer to get the results.

Returns:

  EFI_SUCCESS            -  The data was written to the PCI root bridge.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of
                            resources.
--*/
;

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
/*++

Routine Description:

  Provides the PCI controller-specific address needed to access
  system memory for DMA.

Arguments:

  This           -  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  Operation      -  Indicate if the bus master is going to read or write
                    to system memory.
  HostAddress    -  The system memory address to map on the PCI controller.
  NumberOfBytes  -  On input the number of bytes to map.
                    On output the number of bytes that were mapped.
  DeviceAddress  -  The resulting map address for the bus master PCI
                    controller to use to access the system memory's HostAddress.
  Mapping        -  The value to pass to Unmap() when the bus master DMA
                    operation is complete.

Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_UNSUPPORTED        -  The HostAddress cannot be mapped as a common
                            buffer.
  EFI_DEVICE_ERROR       -  The System hardware could not map the requested
                            address.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to
                            lack of resources.

--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoUnmap (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  VOID                             *Mapping
  )
/*++

Routine Description:

  Completes the Map() operation and releases any corresponding resources.

Arguments:

  This     -  Pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Mapping  -  The value returned from Map() operation.

Returns:

  EFI_SUCCESS            -  The range was unmapped successfully.
  EFI_INVALID_PARAMETER  -  Mapping is not a value that was returned
                            by Map operation.
  EFI_DEVICE_ERROR       -  The data was not committed to the target
                            system memory.

--*/
;

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
/*++

Routine Description:

  Allocates pages that are suitable for a common buffer mapping.

Arguments:

  This         -  Pointer to EFI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Type         -  Not used and can be ignored.
  MemoryType   -  Type of memory to allocate.
  Pages        -  Number of pages to allocate.
  HostAddress  -  Pointer to store the base system memory address
                  of the allocated range.
  Attributes   -  Requested bit mask of attributes of the allocated
                  range.

Returns:

  EFI_SUCCESS            -  The requested memory range were allocated.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_UNSUPPORTED        -  Attributes is unsupported.

--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoFreeBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  UINTN                            Pages,
  OUT VOID                             *HostAddress
  )
/*++

Routine Description:

  Free memory allocated in AllocateBuffer.

Arguments:

  This         -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
                  instance.
  Pages        -  Number of pages to free.
  HostAddress  -  The base system memory address of the
                  allocated range.

Returns:

  EFI_SUCCESS            -  Requested memory pages were freed.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.

--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoFlush (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This
  )
/*++

Routine Description:

  Flushes all PCI posted write transactions from a PCI host
  bridge to system memory.

Arguments:

  This  - Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.

Returns:

  EFI_SUCCESS       -  PCI posted write transactions were flushed
                       from PCI host bridge to system memory.
  EFI_DEVICE_ERROR  -  Fail due to hardware error.

--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoGetAttributes (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT UINT64                           *Supported,
  OUT UINT64                           *Attributes
  )
/*++

Routine Description:

  Get the attributes that a PCI root bridge supports and
  the attributes the PCI root bridge is currently using.

Arguments:

  This        -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
                 instance.
  Supports    -  A pointer to the mask of attributes that
                 this PCI root bridge supports.
  Attributes  -  A pointer to the mask of attributes that
                 this PCI root bridge is currently using.
Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.

--*/

// GC_TODO:    Supported - add argument and description to function comment
//
// GC_TODO:    Supported - add argument and description to function comment
//
// GC_TODO:    Supported - add argument and description to function comment
//
;

EFI_STATUS
EFIAPI
RootBridgeIoSetAttributes (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN     UINT64                           Attributes,
  IN OUT UINT64                           *ResourceBase,
  IN OUT UINT64                           *ResourceLength
  )
/*++

Routine Description:

  Sets the attributes for a resource range on a PCI root bridge.

Arguments:

  This            -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Attributes      -  The mask of attributes to set.
  ResourceBase    -  Pointer to the base address of the resource range
                     to be modified by the attributes specified by Attributes.
  ResourceLength  -  Pointer to the length of the resource range to be modified.

Returns:
  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_OUT_OF_RESOURCES   -  Not enough resources to set the attributes upon.

--*/
;

EFI_STATUS
EFIAPI
RootBridgeIoConfiguration (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT VOID                             **Resources
  )
/*++

Routine Description:

  Retrieves the current resource settings of this PCI root bridge
  in the form of a set of ACPI 2.0 resource descriptor.

Arguments:

  This       -  Pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Resources  -  Pointer to the ACPI 2.0 resource descriptor that
                describe the current configuration of this PCI root
                bridge.

Returns:

  EFI_SUCCESS      -  Success.
  EFI_UNSUPPORTED  -  Current configuration of the PCI root bridge
                      could not be retrieved.

--*/
;

#endif
