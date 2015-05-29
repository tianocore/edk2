/** @file
  UfsHcDxe driver is used to provide platform-dependent info, mainly UFS host controller
  MMIO base, to upper layer UFS drivers.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UfsPciHcDxe.h"

//
// NVM Express Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gUfsHcDriverBinding = {
  UfsHcDriverBindingSupported,
  UfsHcDriverBindingStart,
  UfsHcDriverBindingStop,
  0x10,
  NULL,
  NULL
};

//
// Template for Ufs host controller private data.
//
UFS_HOST_CONTROLLER_PRIVATE_DATA gUfsHcTemplate = {
  UFS_HC_PRIVATE_DATA_SIGNATURE,  // Signature
  NULL,                           // Handle
  {                               // UfsHcProtocol
    UfsHcGetMmioBar,
    UfsHcAllocateBuffer,
    UfsHcFreeBuffer,
    UfsHcMap,
    UfsHcUnmap,
    UfsHcFlush,
    UfsHcMmioRead,
    UfsHcMmioWrite
  },
  NULL,                           // PciIo
  0,                              // BarIndex
  0                               // PciAttributes
};

/**
  Get the MMIO base of the UFS host controller.

  @param[in]   This             A pointer to the EFI_UFS_HOST_CONTROLLER_PROTOCOL instance.
  @param[out]  MmioBar          The MMIO base address of UFS host controller.

  @retval EFI_SUCCESS           The operation succeeds.
  @retval others                The operation fails.
**/
EFI_STATUS
EFIAPI
UfsHcGetMmioBar (
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL *This,
     OUT UINTN                              *MmioBar
  )
{
  UFS_HOST_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_STATUS                        Status;
  UINT8                             BarIndex;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *BarDesc;

  if ((This == NULL) || (MmioBar == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BarDesc  = NULL;
  Private  = UFS_HOST_CONTROLLER_PRIVATE_DATA_FROM_UFSHC (This);
  PciIo    = Private->PciIo;
  BarIndex = Private->BarIndex;

  Status = PciIo->GetBarAttributes (
                    PciIo,
                    BarIndex,
                    NULL,
                    (VOID**) &BarDesc
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *MmioBar = (UINTN)BarDesc->AddrRangeMin;

  FreePool (BarDesc);

  return Status;
}

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
EFI_STATUS
EFIAPI
UfsHcMap (
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL   *This,
  IN     EDKII_UFS_HOST_CONTROLLER_OPERATION  Operation,
  IN     VOID                                 *HostAddress,
  IN OUT UINTN                                *NumberOfBytes,
     OUT EFI_PHYSICAL_ADDRESS                 *DeviceAddress,
     OUT VOID                                 **Mapping
  )
{
  UFS_HOST_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_STATUS                        Status;

  if ((This == NULL) || (HostAddress == NULL) || (NumberOfBytes == NULL) || (DeviceAddress == NULL) || (Mapping == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = UFS_HOST_CONTROLLER_PRIVATE_DATA_FROM_UFSHC (This);
  PciIo   = Private->PciIo;

  Status  = PciIo->Map (PciIo, (EFI_PCI_IO_PROTOCOL_OPERATION)Operation, HostAddress, NumberOfBytes, DeviceAddress, Mapping);
  return Status;
}

/**                                                                 
  Completes the Map() operation and releases any corresponding resources.
            
  @param  This                  A pointer to the EFI_UFS_HOST_CONTROLLER_PROTOCOL instance.                                      
  @param  Mapping               The mapping value returned from Map().
                                  
  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
                                   
**/
EFI_STATUS
EFIAPI
UfsHcUnmap (
  IN  EDKII_UFS_HOST_CONTROLLER_PROTOCOL *This,
  IN  VOID                               *Mapping
  )
{
  UFS_HOST_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_STATUS                        Status;

  if ((This == NULL) || (Mapping == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = UFS_HOST_CONTROLLER_PRIVATE_DATA_FROM_UFSHC (This);
  PciIo   = Private->PciIo;

  Status  = PciIo->Unmap (PciIo, Mapping);
  return Status;
}

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
EFI_STATUS
EFIAPI
UfsHcAllocateBuffer (
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL *This,
  IN     EFI_ALLOCATE_TYPE                  Type,
  IN     EFI_MEMORY_TYPE                    MemoryType,
  IN     UINTN                              Pages,
     OUT VOID                               **HostAddress,
  IN     UINT64                             Attributes
  )
{
  UFS_HOST_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_STATUS                        Status;

  if ((This == NULL) || (HostAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = UFS_HOST_CONTROLLER_PRIVATE_DATA_FROM_UFSHC (This);
  PciIo   = Private->PciIo;

  Status  = PciIo->AllocateBuffer (PciIo, Type, MemoryType, Pages, HostAddress, Attributes);
  return Status;
}

/**                                                                 
  Frees memory that was allocated with AllocateBuffer().
            
  @param  This                  A pointer to the EFI_UFS_HOST_CONTROLLER_PROTOCOL instance.  
  @param  Pages                 The number of pages to free.                                
  @param  HostAddress           The base system memory address of the allocated range.                                    
                                  
  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().
                                     
**/
EFI_STATUS
EFIAPI
UfsHcFreeBuffer (
  IN  EDKII_UFS_HOST_CONTROLLER_PROTOCOL *This,
  IN  UINTN                              Pages,
  IN  VOID                               *HostAddress
  )
{
  UFS_HOST_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_STATUS                        Status;

  if ((This == NULL) || (HostAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = UFS_HOST_CONTROLLER_PRIVATE_DATA_FROM_UFSHC (This);
  PciIo   = Private->PciIo;

  Status  = PciIo->FreeBuffer (PciIo, Pages, HostAddress);
  return Status;
}

/**                                                                 
  Flushes all posted write transactions from the UFS bus to attached UFS device.
            
  @param  This                  A pointer to the EFI_UFS_HOST_CONTROLLER_PROTOCOL instance.  
                                  
  @retval EFI_SUCCESS           The posted write transactions were flushed from the UFS bus
                                to attached UFS device.                                      
  @retval EFI_DEVICE_ERROR      The posted write transactions were not flushed from the UFS
                                bus to attached UFS device due to a hardware error.                           
                                     
**/
EFI_STATUS
EFIAPI
UfsHcFlush (
  IN  EDKII_UFS_HOST_CONTROLLER_PROTOCOL *This
  )
{
  UFS_HOST_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_STATUS                        Status;

  Private = UFS_HOST_CONTROLLER_PRIVATE_DATA_FROM_UFSHC (This);
  PciIo   = Private->PciIo;

  Status  = PciIo->Flush (PciIo);
  return Status;
}

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
EFI_STATUS
EFIAPI
UfsHcMmioRead (
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL        *This,
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL_WIDTH  Width,
  IN     UINT64                                    Offset,
  IN     UINTN                                     Count,
  IN OUT VOID                                      *Buffer
  )
{
  UFS_HOST_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_STATUS                        Status;
  UINT8                             BarIndex;

  Private  = UFS_HOST_CONTROLLER_PRIVATE_DATA_FROM_UFSHC (This);
  PciIo    = Private->PciIo;
  BarIndex = Private->BarIndex;

  Status   = PciIo->Mem.Read (PciIo, (EFI_PCI_IO_PROTOCOL_WIDTH)Width, BarIndex, Offset, Count, Buffer);

  return Status;
}

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
EFI_STATUS
EFIAPI
UfsHcMmioWrite (
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL        *This,
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL_WIDTH  Width,
  IN     UINT64                                    Offset,
  IN     UINTN                                     Count,
  IN OUT VOID                                      *Buffer
  )
{
  UFS_HOST_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_STATUS                        Status;
  UINT8                             BarIndex;

  Private  = UFS_HOST_CONTROLLER_PRIVATE_DATA_FROM_UFSHC (This);
  PciIo    = Private->PciIo;
  BarIndex = Private->BarIndex;

  Status   = PciIo->Mem.Write (PciIo, (EFI_PCI_IO_PROTOCOL_WIDTH)Width, BarIndex, Offset, Count, Buffer);

  return Status;
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Since ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
UfsHcDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  BOOLEAN                   UfsHcFound;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                PciData;

  PciIo            = NULL;
  ParentDevicePath = NULL;
  UfsHcFound       = FALSE;

  //
  // UfsHcDxe is a device driver, and should ingore the
  // "RemainingDevicePath" according to EFI spec
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID *) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    //
    // EFI_ALREADY_STARTED is also an error
    //
    return Status;
  }
  //
  // Close the protocol because we don't use it here
  //
  gBS->CloseProtocol (
        Controller,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Now test the EfiPciIoProtocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Now further check the PCI header: Base class (offset 0x0B) and
  // Sub Class (offset 0x0A). This controller should be an UFS controller
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        0,
                        sizeof (PciData),
                        &PciData
                        );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiPciIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return EFI_UNSUPPORTED;
  }
  //
  // Since we already got the PciData, we can close protocol to avoid to carry it on for multiple exit points.
  //
  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Examine UFS Host Controller PCI Configuration table fields
  //
  if (PciData.Hdr.ClassCode[2] == PCI_CLASS_MASS_STORAGE) {
    if (PciData.Hdr.ClassCode[1] == 0x09 ) { //UFS Controller Subclass
      UfsHcFound = TRUE;
    }
  }

  if (!UfsHcFound) {
    return EFI_UNSUPPORTED;
  }

  return Status;
}


/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
UfsHcDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  UFS_HOST_CONTROLLER_PRIVATE_DATA  *Private;
  UINT64                            Supports;
  UINT8                             BarIndex;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *BarDesc;

  PciIo    = NULL;
  Private  = NULL;
  Supports = 0;
  BarDesc  = NULL;

  //
  // Now test and open the EfiPciIoProtocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  //
  // Status == 0 - A normal execution flow, SUCCESS and the program proceeds.
  // Status == ALREADY_STARTED - A non-zero Status code returned. It indicates
  //           that the protocol has been opened and should be treated as a
  //           normal condition and the program proceeds. The Protocol will not
  //           opened 'again' by this call.
  // Status != ALREADY_STARTED - Error status, terminate program execution
  //
  if (EFI_ERROR (Status)) {
    //
    // EFI_ALREADY_STARTED is also an error
    //
    return Status;
  }

  Private = AllocateCopyPool (sizeof (UFS_HOST_CONTROLLER_PRIVATE_DATA), &gUfsHcTemplate);
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Private->PciIo = PciIo;

  for (BarIndex = 0; BarIndex < PCI_MAX_BAR; BarIndex++) {
    Status = PciIo->GetBarAttributes (
                      PciIo,
                      BarIndex,
                      NULL,
                      (VOID**) &BarDesc
                      );
    if (Status == EFI_UNSUPPORTED) {
      continue;
    } else if (EFI_ERROR (Status)) {
      goto Done;
    }

    if (BarDesc->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
      Private->BarIndex = BarIndex;
      FreePool (BarDesc);
      break;
    }

    FreePool (BarDesc);
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationGet,
                    0,
                    &Private->PciAttributes
                    );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );

  if (!EFI_ERROR (Status)) {
    Supports &= (UINT64)EFI_PCI_DEVICE_ENABLE;
    Status    = PciIo->Attributes (
                         PciIo,
                         EfiPciIoAttributeOperationEnable,
                         Supports,
                         NULL
                         );
  } else {
    goto Done;
  }

  ///
  /// Install UFS_HOST_CONTROLLER protocol
  ///
  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEdkiiUfsHostControllerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID*)&(Private->UfsHc)
                  );

Done:
  if (EFI_ERROR (Status)) {
    if ((Private != NULL) && (Private->PciAttributes != 0)) {
      //
      // Restore original PCI attributes
      //
      Status = PciIo->Attributes (
                        PciIo,
                        EfiPciIoAttributeOperationSet,
                        Private->PciAttributes,
                        NULL
                        );
      ASSERT_EFI_ERROR (Status);
    }
    gBS->CloseProtocol (
          Controller,
          &gEfiPciIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    if (Private != NULL) {
      FreePool (Private);
    }
  }

  return Status;
}


/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
UfsHcDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                          Status;
  UFS_HOST_CONTROLLER_PRIVATE_DATA    *Private;
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHc;

  ///
  /// Get private data
  ///
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEdkiiUfsHostControllerProtocolGuid,
                  (VOID **) &UfsHc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Private = UFS_HOST_CONTROLLER_PRIVATE_DATA_FROM_UFSHC (UfsHc);

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEdkiiUfsHostControllerProtocolGuid,
                  &(Private->UfsHc)
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Restore original PCI attributes
    //
    Status = Private->PciIo->Attributes (
                               Private->PciIo,
                               EfiPciIoAttributeOperationSet,
                               Private->PciAttributes,
                               NULL
                               );
    ASSERT_EFI_ERROR (Status);

    //
    // Close protocols opened by UFS host controller driver
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    FreePool (Private);
  }

  return Status;
}

/**
  The entry point for UFS host controller driver, used to install this driver on the ImageHandle.

  @param[in]  ImageHandle   The firmware allocated handle for this driver image.
  @param[in]  SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS   Driver loaded.
  @retval other         Driver not loaded.

**/
EFI_STATUS
EFIAPI
UfsHcDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUfsHcDriverBinding,
             ImageHandle,
             &gUfsHcComponentName,
             &gUfsHcComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
