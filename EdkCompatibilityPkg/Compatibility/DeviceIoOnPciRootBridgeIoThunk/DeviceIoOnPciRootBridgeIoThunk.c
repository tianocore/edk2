/** @file
Module produces Device I/O on top of PCI Root Bridge I/O for Segment 0 only.
This is a valid assumption because many of the EFI 1.02/EFI 1.10 systems that may have provided 
Device I/O were single segment platforms.  The goal of the ECP is to provide compatibility with the 
drivers/apps that may have used Device I/O.

Device I/O is on list of deprecated protocols for UEFI 2.0 and later.
This module module layers Device I/O on top of PCI Root Bridge I/O (Segment 0)
 Use if:
   There are no EDK modules present that produces Device I/O
   EFI drivers included that consume Device I/O
   Platform required to support EFI drivers that consume Device I/O
   Platform required to support EFI applications that consume Device I/O

Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <IndustryStandard/Pci.h>
#include <Protocol/DeviceIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>


/**
  Perform reading memory mapped I/O space of device.

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The destination buffer to store results.

  @retval EFI_SUCCESS            The data was read from the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoMemRead (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_WIDTH             Width,
  IN     UINT64                   Address,
  IN     UINTN                    Count,
  IN OUT VOID                     *Buffer
  );


/**
  Perform writing memory mapped I/O space of device.

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The source buffer of data to be written.

  @retval EFI_SUCCESS            The data was written to the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoMemWrite (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_IO_WIDTH              Width,
  IN     UINT64                    Address,
  IN     UINTN                     Count,
  IN OUT VOID                      *Buffer
  );

/**
  Perform reading I/O space of device.

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The destination buffer to store results.

  @retval EFI_SUCCESS            The data was read from the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoIoRead (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_WIDTH             Width,
  IN     UINT64                   Address,
  IN     UINTN                    Count,
  IN OUT VOID                     *Buffer
  );

/**
  Perform writing I/O space of device.

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The source buffer of data to be written.

  @retval EFI_SUCCESS            The data was written to the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoIoWrite (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_IO_WIDTH              Width,
  IN     UINT64                    Address,
  IN     UINTN                     Count,
  IN OUT VOID                      *Buffer
  );

/**
  Perform reading PCI configuration space of device

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The destination buffer to store results.

  @retval EFI_SUCCESS            The data was read from the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoPciRead (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_WIDTH             Width,
  IN     UINT64                   Address,
  IN     UINTN                    Count,
  IN OUT VOID                     *Buffer
  );

/**
  Perform writing PCI configuration space of device.

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The source buffer of data to be written.

  @retval EFI_SUCCESS            The data was written to the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoPciWrite (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_IO_WIDTH              Width,
  IN     UINT64                    Address,
  IN     UINTN                     Count,
  IN OUT VOID                      *Buffer
  );

/**
  Provides an EFI Device Path for a PCI device with the given PCI configuration space address.

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  @param  Address                The PCI configuration space address of the device
                                 whose Device Path is going to be returned.
  @param  PciDevicePath          A pointer to the pointer for the EFI Device Path
                                 for PciAddress. Memory for the Device Path is
                                 allocated from the pool.

  @retval EFI_SUCCESS            The PciDevicePath returns a pointer to a valid EFI
                                 Device Path.
  @retval EFI_UNSUPPORTED        The PciAddress does not map to a valid EFI Device
                                 Path.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack
                                 of resources.

**/
EFI_STATUS
EFIAPI
DeviceIoPciDevicePath (
  IN     EFI_DEVICE_IO_PROTOCOL        *This,
  IN     UINT64                        Address,
  IN OUT EFI_DEVICE_PATH_PROTOCOL      **PciDevicePath
  );

/**
  Provides the device-specific addresses needed to access system memory.

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  @param  Operation              Indicates if the bus master is going to read or
                                 write to system memory.
  @param  HostAddress            The system memory address to map to the device.
  @param  NumberOfBytes          On input the number of bytes to map. On output the
                                 number of bytes that were mapped.
  @param  DeviceAddress          The resulting map address for the bus master
                                 device to use to access the hosts HostAddress.
  @param  Mapping                A resulting value to pass to Unmap().

  @retval EFI_SUCCESS            The range was mapped for the returned
                                 NumberOfBytes.
  @retval EFI_INVALID_PARAMETER  The Operation or HostAddress is undefined.
  @retval EFI_UNSUPPORTED        The HostAddress cannot be mapped as a common
                                 buffer.
  @retval EFI_DEVICE_ERROR       The system hardware could not map the requested
                                 address.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack
                                 of resources.

**/
EFI_STATUS
EFIAPI
DeviceIoMap (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_OPERATION_TYPE    Operation,
  IN     EFI_PHYSICAL_ADDRESS     *HostAddress,
  IN OUT UINTN                    *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS     *DeviceAddress,
  OUT    VOID                     **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  @param  Mapping                The mapping value returned from Map().

  @retval EFI_SUCCESS            The range was unmapped.
  @retval EFI_DEVICE_ERROR       The data was not committed to the target system
                                 memory.

**/
EFI_STATUS
EFIAPI
DeviceIoUnmap (
  IN EFI_DEVICE_IO_PROTOCOL   *This,
  IN VOID                     *Mapping
  );

/**
  Allocates pages that are suitable for an EFIBusMasterCommonBuffer mapping.

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  @param  Type                   The type allocation to perform.
  @param  MemoryType             The type of memory to allocate,
                                 EfiBootServicesData or EfiRuntimeServicesData.
  @param  Pages                  The number of pages to allocate.
  @param  PhysicalAddress        A pointer to store the base address of the
                                 allocated range.

  @retval EFI_SUCCESS            The requested memory pages were allocated.
  @retval EFI_OUT_OF_RESOURCES   The memory pages could not be allocated.
  @retval EFI_INVALID_PARAMETER  The requested memory type is invalid.
  @retval EFI_UNSUPPORTED        The requested PhysicalAddress is not supported on
                                 this platform.

**/
EFI_STATUS
EFIAPI
DeviceIoAllocateBuffer (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_ALLOCATE_TYPE         Type,
  IN     EFI_MEMORY_TYPE           MemoryType,
  IN     UINTN                     Pages,
  IN OUT EFI_PHYSICAL_ADDRESS      *PhysicalAddress
  );

/**
  Flushes any posted write data to the device.

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.

  @retval EFI_SUCCESS            The buffers were flushed.
  @retval EFI_DEVICE_ERROR       The buffers were not flushed due to a hardware
                                 error.

**/
EFI_STATUS
EFIAPI
DeviceIoFlush (
  IN EFI_DEVICE_IO_PROTOCOL  *This
  );

/**
  Frees pages that were allocated with AllocateBuffer().

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  @param  Pages                  The number of pages to free.
  @param  HostAddress            The base address of the range to free.

  @retval EFI_SUCCESS            The requested memory pages were freed.
  @retval EFI_NOT_FOUND          The requested memory pages were not allocated with
                                 AllocateBuffer().
  @retval EFI_INVALID_PARAMETER  HostAddress is not page aligned or Pages is
                                 invalid.

**/
EFI_STATUS
EFIAPI
DeviceIoFreeBuffer (
  IN EFI_DEVICE_IO_PROTOCOL   *This,
  IN UINTN                    Pages,
  IN EFI_PHYSICAL_ADDRESS     HostAddress
  );


#define DEVICE_IO_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('D', 'e', 'I', 'O')

typedef struct {
  UINTN                           Signature;
  EFI_DEVICE_IO_PROTOCOL          DeviceIo;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  UINT16                          PrimaryBus;
  UINT16                          SubordinateBus;
} DEVICE_IO_PRIVATE_DATA;

#define DEVICE_IO_PRIVATE_DATA_FROM_THIS(a) CR (a, DEVICE_IO_PRIVATE_DATA, DeviceIo, DEVICE_IO_PRIVATE_DATA_SIGNATURE)

#define MAX_COMMON_BUFFER                 0x00000000FFFFFFFF


EFI_EVENT  mPciRootBridgeIoRegistration;

//
// Device Io Volume Protocol template
//
DEVICE_IO_PRIVATE_DATA gDeviceIoPrivateDataTemplate = {
  DEVICE_IO_PRIVATE_DATA_SIGNATURE,
  {
    {
      DeviceIoMemRead,
      DeviceIoMemWrite
    },
    {
      DeviceIoIoRead,
      DeviceIoIoWrite  
    },
    {
      DeviceIoPciRead,
      DeviceIoPciWrite,  
    },
    DeviceIoMap,
    DeviceIoPciDevicePath,
    DeviceIoUnmap,
    DeviceIoAllocateBuffer,
    DeviceIoFlush,
    DeviceIoFreeBuffer
  },
  NULL, // PciRootBridgeIo
  NULL, // DevicePath
  0,    // PrimaryBus
  255   // SubordinateBus
};

/**
  This notification function is invoked when an instance of the
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL is produced. It installs another instance of the
  EFI_DEVICE_IO_PROTOCOL on the same handle.

  @param  Event                 The event that occured
  @param  Context               Context of event. Not used in this nofication function.

**/
VOID
EFIAPI
PciRootBridgeIoNotificationEvent (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                     Status;
  UINTN                          BufferSize;
  EFI_HANDLE                     Handle;
  DEVICE_IO_PRIVATE_DATA         *Private;
  EFI_DEVICE_IO_PROTOCOL         *DeviceIo;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;

  while (TRUE) {
    BufferSize = sizeof (Handle);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    &gEfiPciRootBridgeIoProtocolGuid,
                    mPciRootBridgeIoRegistration,
                    &BufferSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      //
      // Exit Path of While Loop....
      //
      break;
    }

    //
    // Skip this handle if the Device Io Protocol is already installed
    //
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiDeviceIoProtocolGuid,
                    (VOID **)&DeviceIo
                    );
    if (!EFI_ERROR (Status)) {
      continue;
    }

    //
    // Retrieve the Pci Root Bridge IO Protocol
    //
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiPciRootBridgeIoProtocolGuid,
                    (VOID **)&PciRootBridgeIo
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // We only install Device IO for PCI bus in Segment 0.
    // See the file description at @file for details.
    //
    if (PciRootBridgeIo->SegmentNumber != 0) {
      continue;
    }

    //
    // Allocate private data structure
    //
    Private = AllocateCopyPool (sizeof (DEVICE_IO_PRIVATE_DATA), &gDeviceIoPrivateDataTemplate);
    if (Private == NULL) {
      continue;
    }

    Private->PciRootBridgeIo = PciRootBridgeIo;

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **) &Private->DevicePath
                    );

    //
    // Install Device Io onto same handle
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Handle,
                    &gEfiDeviceIoProtocolGuid,
                    &Private->DeviceIo,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  The user Entry Point for DXE driver. The user code starts with this function
  as the real entry point for the image goes into a library that calls this 
  function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeDeviceIo (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EfiCreateProtocolNotifyEvent (
    &gEfiPciRootBridgeIoProtocolGuid,
    TPL_CALLBACK,
    PciRootBridgeIoNotificationEvent,
    NULL,
    &mPciRootBridgeIoRegistration
    );
  return EFI_SUCCESS;
}


/**
  Perform reading memory mapped I/O space of device.

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The destination buffer to store results.

  @retval EFI_SUCCESS            The data was read from the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoMemRead (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_WIDTH             Width,
  IN     UINT64                   Address,
  IN     UINTN                    Count,
  IN OUT VOID                     *Buffer
  )
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Width > MMIO_COPY_UINT64) {
    return EFI_INVALID_PARAMETER;
  }
  if (Width >= MMIO_COPY_UINT8) {
    Width = (EFI_IO_WIDTH) (Width - MMIO_COPY_UINT8);
    Status = Private->PciRootBridgeIo->CopyMem (
                                         Private->PciRootBridgeIo,
                                         (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                         (UINT64) (UINTN) Buffer,
                                         Address,
                                         Count
                                         );
  } else {
    Status = Private->PciRootBridgeIo->Mem.Read (
                                             Private->PciRootBridgeIo,
                                             (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                             Address,
                                             Count,
                                             Buffer
                                             );
  }

  return Status;
}




/**
  Perform writing memory mapped I/O space of device.

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The source buffer of data to be written.

  @retval EFI_SUCCESS            The data was written to the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoMemWrite (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_IO_WIDTH              Width,
  IN     UINT64                    Address,
  IN     UINTN                     Count,
  IN OUT VOID                      *Buffer
  )
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Width > MMIO_COPY_UINT64) {
    return EFI_INVALID_PARAMETER;
  }
  if (Width >= MMIO_COPY_UINT8) {
    Width = (EFI_IO_WIDTH) (Width - MMIO_COPY_UINT8);
    Status = Private->PciRootBridgeIo->CopyMem (
                                         Private->PciRootBridgeIo,
                                         (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                         Address,
                                         (UINT64) (UINTN) Buffer,
                                         Count
                                         );
  } else {
    Status = Private->PciRootBridgeIo->Mem.Write (
                                             Private->PciRootBridgeIo,
                                             (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                             Address,
                                             Count,
                                             Buffer
                                             );
  }

  return Status;
}


/**
  Perform reading I/O space of device.

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The destination buffer to store results.

  @retval EFI_SUCCESS            The data was read from the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoIoRead (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_WIDTH             Width,
  IN     UINT64                   Address,
  IN     UINTN                    Count,
  IN OUT VOID                     *Buffer
  )
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Width >= MMIO_COPY_UINT8) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Private->PciRootBridgeIo->Io.Read (
                                          Private->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          Address,
                                          Count,
                                          Buffer
                                          );

  return Status;
}


/**
  Perform writing I/O space of device.

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The source buffer of data to be written.

  @retval EFI_SUCCESS            The data was written to the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoIoWrite (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_IO_WIDTH              Width,
  IN     UINT64                    Address,
  IN     UINTN                     Count,
  IN OUT VOID                      *Buffer
  )
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Width >= MMIO_COPY_UINT8) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Private->PciRootBridgeIo->Io.Write (
                                          Private->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          Address,
                                          Count,
                                          Buffer
                                          );

  return Status;
}


/**
  Perform reading PCI configuration space of device

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The destination buffer to store results.

  @retval EFI_SUCCESS            The data was read from the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoPciRead (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_WIDTH             Width,
  IN     UINT64                   Address,
  IN     UINTN                    Count,
  IN OUT VOID                     *Buffer
  )
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if ((UINT32)Width >= MMIO_COPY_UINT8) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Private->PciRootBridgeIo->Pci.Read (
                                           Private->PciRootBridgeIo,
                                           (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                           Address,
                                           Count,
                                           Buffer
                                           );

  return Status;
}


/**
  Perform writing PCI configuration space of device.

  @param  This                   A pointer to EFI_DEVICE_IO protocol instance.
  @param  Width                  Width of I/O operations.
  @param  Address                The base address of I/O operations.
  @param  Count                  The number of I/O operations to perform.  Bytes
                                 moves is Width size * Count, starting at Address.
  @param  Buffer                 The source buffer of data to be written.

  @retval EFI_SUCCESS            The data was written to the device.
  @retval EFI_INVALID_PARAMETER  Width is invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to lack of
                                 resources.

**/
EFI_STATUS
EFIAPI
DeviceIoPciWrite (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_IO_WIDTH              Width,
  IN     UINT64                    Address,
  IN     UINTN                     Count,
  IN OUT VOID                      *Buffer
  )
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if ((UINT32)Width >= MMIO_COPY_UINT8) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Private->PciRootBridgeIo->Pci.Write (
                                           Private->PciRootBridgeIo,
                                           (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                           Address,
                                           Count,
                                           Buffer
                                           );

  return Status;
}


/**
  Append a PCI device path node to another device path.

  @param  Private                A pointer to DEVICE_IO_PRIVATE_DATA instance.
  @param  Bus                    PCI bus number of the device.
  @param  Device                 PCI device number of the device.
  @param  Function               PCI function number of the device.
  @param  DevicePath             Original device path which will be appended a PCI
                                 device path node.
  @param  BridgePrimaryBus       Primary bus number of the bridge.
  @param  BridgeSubordinateBus   Subordinate bus number of the bridge.

  @return Pointer to the appended PCI device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
AppendPciDevicePath (
  IN     DEVICE_IO_PRIVATE_DATA    *Private,
  IN     UINT8                     Bus,
  IN     UINT8                     Device,
  IN     UINT8                     Function,
  IN     EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN OUT UINT16                    *BridgePrimaryBus,
  IN OUT UINT16                    *BridgeSubordinateBus
  )
{
  UINT16                    ThisBus;
  UINT8                     ThisDevice;
  UINT8                     ThisFunc;
  UINT64                    Address;
  PCI_TYPE01                PciBridge;
  PCI_TYPE01                *PciPtr;
  EFI_DEVICE_PATH_PROTOCOL  *ReturnDevicePath;
  PCI_DEVICE_PATH           PciNode;

  PciPtr = &PciBridge;
  for (ThisBus = *BridgePrimaryBus; ThisBus <= *BridgeSubordinateBus; ThisBus++) {
    for (ThisDevice = 0; ThisDevice <= PCI_MAX_DEVICE; ThisDevice++) {
      for (ThisFunc = 0; ThisFunc <= PCI_MAX_FUNC; ThisFunc++) {
        Address = EFI_PCI_ADDRESS (ThisBus, ThisDevice, ThisFunc, 0);
        ZeroMem (PciPtr, sizeof (PCI_TYPE01));
        Private->DeviceIo.Pci.Read (
                                &Private->DeviceIo,
                                IO_UINT32,
                                Address,
                                1,
                                &(PciPtr->Hdr.VendorId)
                                );
        if ((PciPtr->Hdr.VendorId == 0xffff) && (ThisFunc == 0)) {
          break;
        }
        if (PciPtr->Hdr.VendorId == 0xffff) {
          continue;
        } else {
          Private->DeviceIo.Pci.Read (
                                  &Private->DeviceIo,
                                  IO_UINT32,
                                  Address,
                                  sizeof (PCI_TYPE01) / sizeof (UINT32),
                                  PciPtr
                                  );
          if (IS_PCI_BRIDGE (PciPtr)) {
            if (Bus >= PciPtr->Bridge.SecondaryBus && Bus <= PciPtr->Bridge.SubordinateBus) {

              PciNode.Header.Type     = HARDWARE_DEVICE_PATH;
              PciNode.Header.SubType  = HW_PCI_DP;
              SetDevicePathNodeLength (&PciNode.Header, sizeof (PciNode));

              PciNode.Device        = ThisDevice;
              PciNode.Function      = ThisFunc;
              ReturnDevicePath      = AppendDevicePathNode (DevicePath, &PciNode.Header);

              *BridgePrimaryBus     = PciPtr->Bridge.SecondaryBus;
              *BridgeSubordinateBus = PciPtr->Bridge.SubordinateBus;
              return ReturnDevicePath;
            }
          }
          if (ThisFunc == 0 && ((PciPtr->Hdr.HeaderType & HEADER_TYPE_MULTI_FUNCTION) != HEADER_TYPE_MULTI_FUNCTION)) {
            //
            // Skip sub functions, this is not a multi function device
            //
            ThisFunc = 8;
          }
        }
      }
    }
  }

  ZeroMem (&PciNode, sizeof (PciNode));
  PciNode.Header.Type     = HARDWARE_DEVICE_PATH;
  PciNode.Header.SubType  = HW_PCI_DP;
  SetDevicePathNodeLength (&PciNode.Header, sizeof (PciNode));
  PciNode.Device        = Device;
  PciNode.Function      = Function;

  ReturnDevicePath      = AppendDevicePathNode (DevicePath, &PciNode.Header);

  *BridgePrimaryBus     = 0xffff;
  *BridgeSubordinateBus = 0xffff;
  return ReturnDevicePath;
}


/**
  Provides an EFI Device Path for a PCI device with the given PCI configuration space address.

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  @param  Address                The PCI configuration space address of the device
                                 whose Device Path is going to be returned.
  @param  PciDevicePath          A pointer to the pointer for the EFI Device Path
                                 for PciAddress. Memory for the Device Path is
                                 allocated from the pool.

  @retval EFI_SUCCESS            The PciDevicePath returns a pointer to a valid EFI
                                 Device Path.
  @retval EFI_UNSUPPORTED        The PciAddress does not map to a valid EFI Device
                                 Path.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack
                                 of resources.

**/
EFI_STATUS
EFIAPI
DeviceIoPciDevicePath (
  IN     EFI_DEVICE_IO_PROTOCOL        *This,
  IN     UINT64                        Address,
  IN OUT EFI_DEVICE_PATH_PROTOCOL      **PciDevicePath
  )
{
  DEVICE_IO_PRIVATE_DATA  *Private;
  UINT16                  PrimaryBus;
  UINT16                  SubordinateBus;
  UINT8                   Bus;
  UINT8                   Device;
  UINT8                   Func;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  Bus     = (UINT8) (((UINT32) Address >> 24) & 0xff);
  Device  = (UINT8) (((UINT32) Address >> 16) & 0xff);
  Func    = (UINT8) (((UINT32) Address >> 8) & 0xff);

  if (Bus < Private->PrimaryBus || Bus > Private->SubordinateBus) {
    return EFI_UNSUPPORTED;
  }

  *PciDevicePath  = Private->DevicePath;
  PrimaryBus      = Private->PrimaryBus;
  SubordinateBus  = Private->SubordinateBus;
  do {
    *PciDevicePath = AppendPciDevicePath (
                       Private,
                       Bus,
                       Device,
                       Func,
                       *PciDevicePath,
                       &PrimaryBus,
                       &SubordinateBus
                       );
    if (*PciDevicePath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } while (PrimaryBus != 0xffff);

  return EFI_SUCCESS;
}


/**
  Provides the device-specific addresses needed to access system memory.

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  @param  Operation              Indicates if the bus master is going to read or
                                 write to system memory.
  @param  HostAddress            The system memory address to map to the device.
  @param  NumberOfBytes          On input the number of bytes to map. On output the
                                 number of bytes that were mapped.
  @param  DeviceAddress          The resulting map address for the bus master
                                 device to use to access the hosts HostAddress.
  @param  Mapping                A resulting value to pass to Unmap().

  @retval EFI_SUCCESS            The range was mapped for the returned
                                 NumberOfBytes.
  @retval EFI_INVALID_PARAMETER  The Operation or HostAddress is undefined.
  @retval EFI_UNSUPPORTED        The HostAddress cannot be mapped as a common
                                 buffer.
  @retval EFI_DEVICE_ERROR       The system hardware could not map the requested
                                 address.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack
                                 of resources.

**/
EFI_STATUS
EFIAPI
DeviceIoMap (
  IN     EFI_DEVICE_IO_PROTOCOL   *This,
  IN     EFI_IO_OPERATION_TYPE    Operation,
  IN     EFI_PHYSICAL_ADDRESS     *HostAddress,
  IN OUT UINTN                    *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS     *DeviceAddress,
  OUT    VOID                     **Mapping
  )
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if ((UINT32)Operation > EfiBusMasterCommonBuffer) {
    return EFI_INVALID_PARAMETER;
  }

  if (((UINTN) (*HostAddress) != (*HostAddress)) && Operation == EfiBusMasterCommonBuffer) {
    return EFI_UNSUPPORTED;
  }

  Status = Private->PciRootBridgeIo->Map (
                                       Private->PciRootBridgeIo,
                                       (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION) Operation,
                                       (VOID *) (UINTN) (*HostAddress),
                                       NumberOfBytes,
                                       DeviceAddress,
                                       Mapping
                                       );

  return Status;
}


/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  @param  Mapping                The mapping value returned from Map().

  @retval EFI_SUCCESS            The range was unmapped.
  @retval EFI_DEVICE_ERROR       The data was not committed to the target system
                                 memory.

**/
EFI_STATUS
EFIAPI
DeviceIoUnmap (
  IN EFI_DEVICE_IO_PROTOCOL   *This,
  IN VOID                     *Mapping
  )
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  Status = Private->PciRootBridgeIo->Unmap (
                                       Private->PciRootBridgeIo,
                                       Mapping
                                       );

  return Status;
}


/**
  Allocates pages that are suitable for an EFIBusMasterCommonBuffer mapping.

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  @param  Type                   The type allocation to perform.
  @param  MemoryType             The type of memory to allocate,
                                 EfiBootServicesData or EfiRuntimeServicesData.
  @param  Pages                  The number of pages to allocate.
  @param  PhysicalAddress        A pointer to store the base address of the
                                 allocated range.

  @retval EFI_SUCCESS            The requested memory pages were allocated.
  @retval EFI_OUT_OF_RESOURCES   The memory pages could not be allocated.
  @retval EFI_INVALID_PARAMETER  The requested memory type is invalid.
  @retval EFI_UNSUPPORTED        The requested PhysicalAddress is not supported on
                                 this platform.

**/
EFI_STATUS
EFIAPI
DeviceIoAllocateBuffer (
  IN     EFI_DEVICE_IO_PROTOCOL    *This,
  IN     EFI_ALLOCATE_TYPE         Type,
  IN     EFI_MEMORY_TYPE           MemoryType,
  IN     UINTN                     Pages,
  IN OUT EFI_PHYSICAL_ADDRESS      *PhysicalAddress
  )
{
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS    HostAddress;
  DEVICE_IO_PRIVATE_DATA  *Private;
  VOID                    *HostAddress2;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  HostAddress = *PhysicalAddress;

  if ((MemoryType != EfiBootServicesData) && (MemoryType != EfiRuntimeServicesData)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32)Type >= MaxAllocateType) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Type == AllocateAddress) && (HostAddress + EFI_PAGES_TO_SIZE (Pages) - 1 > MAX_COMMON_BUFFER)) {
    return EFI_UNSUPPORTED;
  }

  if ((AllocateAnyPages == Type) || (AllocateMaxAddress == Type && HostAddress > MAX_COMMON_BUFFER)) {
    Type        = AllocateMaxAddress;
    HostAddress = MAX_COMMON_BUFFER;
  }

  HostAddress2 = (VOID *) (UINTN) (HostAddress);
  Status = Private->PciRootBridgeIo->AllocateBuffer (
                                       Private->PciRootBridgeIo,
                                       Type,
                                       MemoryType,
                                       Pages,
                                       &HostAddress2,
                                       EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE |
                                       EFI_PCI_ATTRIBUTE_MEMORY_CACHED
                                       );
                                                    
  if (EFI_ERROR (Status)) {
    return Status;
  }


  *PhysicalAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress2;

  return EFI_SUCCESS;
}


/**
  Flushes any posted write data to the device.

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.

  @retval EFI_SUCCESS            The buffers were flushed.
  @retval EFI_DEVICE_ERROR       The buffers were not flushed due to a hardware
                                 error.

**/
EFI_STATUS
EFIAPI
DeviceIoFlush (
  IN EFI_DEVICE_IO_PROTOCOL  *This
  )
{
  EFI_STATUS              Status;
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  Status  = Private->PciRootBridgeIo->Flush (Private->PciRootBridgeIo);

  return Status;
}


/**
  Frees pages that were allocated with AllocateBuffer().

  @param  This                   A pointer to the EFI_DEVICE_IO_INTERFACE instance.
  @param  Pages                  The number of pages to free.
  @param  HostAddress            The base address of the range to free.

  @retval EFI_SUCCESS            The requested memory pages were freed.
  @retval EFI_NOT_FOUND          The requested memory pages were not allocated with
                                 AllocateBuffer().
  @retval EFI_INVALID_PARAMETER  HostAddress is not page aligned or Pages is
                                 invalid.

**/
EFI_STATUS
EFIAPI
DeviceIoFreeBuffer (
  IN EFI_DEVICE_IO_PROTOCOL   *This,
  IN UINTN                    Pages,
  IN EFI_PHYSICAL_ADDRESS     HostAddress
  )
{
  DEVICE_IO_PRIVATE_DATA  *Private;

  Private = DEVICE_IO_PRIVATE_DATA_FROM_THIS (This);

  if (((HostAddress & EFI_PAGE_MASK) != 0) || (Pages <= 0)) {
    return EFI_INVALID_PARAMETER;
  }

  return  Private->PciRootBridgeIo->FreeBuffer (
            Private->PciRootBridgeIo,
            Pages,
            (VOID *) (UINTN) HostAddress
            );

}

