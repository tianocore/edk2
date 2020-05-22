/*++ @file

Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010,Apple Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Gop.h"
#include <IndustryStandard/Acpi.h>

STATIC VOID
BhyveGetGraphicsMode (
  EFI_PCI_IO_PROTOCOL *PciIo,
  UINT16              *Width,
  UINT16              *Height,
  UINT16              *Depth
  );


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
  Because ControllerHandle may have been previously started by the same driver, if a protocol is
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
EmuGopDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;
  UINT16                    Width, Height, Depth;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // See if this is a PCI Framebuffer Controller by looking at the Command register and
  // Class Code Register
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        PCI_BAR_IDX0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = EFI_UNSUPPORTED;
  if (Pci.Hdr.VendorId == 0xFB5D && Pci.Hdr.DeviceId == 0x40FB) {
    DEBUG((DEBUG_INFO, "BHYVE framebuffer device detected\n"));
    Status = EFI_SUCCESS;

    BhyveGetGraphicsMode(PciIo, &Width, &Height, &Depth);
    PcdSet32S (PcdVideoHorizontalResolution, Width);
    PcdSet32S (PcdVideoVerticalResolution, Height);
  }

Done:
  //
  // Close the PCI I/O Protocol
  //
  gBS->CloseProtocol (
        Handle,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );

  return Status;
}


/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
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
EmuGopDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
  BHYVE_FBUF_MEMREGS      Memregs;
  GOP_PRIVATE_DATA        *Private;
  EFI_STATUS              Status;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *MmioDesc;

  //
  // Allocate Private context data for SGO inteface.
  //
  Private = NULL;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (GOP_PRIVATE_DATA),
                  (VOID **)&Private
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  // Set up context record
  //
  Private->Signature           = GOP_PRIVATE_DATA_SIGNATURE;
  Private->Handle              = Handle;
  Private->ControllerNameTable = NULL;

  //
  // Open PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &Private->PciIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Check if fbuf mmio BAR is present
  //
  MmioDesc = NULL;
  Status = Private->PciIo->GetBarAttributes (
                      Private->PciIo,
                      PCI_BAR_IDX0,
                      NULL,
                      (VOID**) &MmioDesc
                      );
  if (EFI_ERROR (Status) ||
      MmioDesc->ResType != ACPI_ADDRESS_SPACE_TYPE_MEM) {
    DEBUG ((DEBUG_INFO, "BHYVE GOP: No mmio bar\n"));
  } else {
    DEBUG ((DEBUG_INFO, "BHYVE GOP: Using mmio bar @ 0x%lx\n",
            MmioDesc->AddrRangeMin));
    BhyveGetMemregs(Private, &Memregs);
    Private->FbSize = Memregs.FbSize;
  }
  if (MmioDesc != NULL) {
    FreePool (MmioDesc);
  }
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Check if fbuf frame-buffer BAR is present
  //
  MmioDesc = NULL;
  Status = Private->PciIo->GetBarAttributes (
                      Private->PciIo,
                      PCI_BAR_IDX1,
                      NULL,
                      (VOID**) &MmioDesc
                      );
  if (EFI_ERROR (Status) ||
      MmioDesc->ResType != ACPI_ADDRESS_SPACE_TYPE_MEM) {
    DEBUG ((DEBUG_INFO, "BHYVE GOP: No frame-buffer bar\n"));
  } else {
    DEBUG ((DEBUG_INFO, "BHYVE GOP: Using frame-buffer bar @ 0x%lx\n",
            MmioDesc->AddrRangeMin));
    Private->FbAddr = MmioDesc->AddrRangeMin;
    // XXX assert BAR is >= size
  }

  if (MmioDesc != NULL) {
    FreePool (MmioDesc);
  }
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  DEBUG ((DEBUG_INFO, "BHYVE GOP: Framebuf addr 0x%lx, size %x\n",
         Private->FbAddr, Private->FbSize));

  Status = EmuGopConstructor (Private);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Publish the Gop interface to the world
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Handle,
                  &gEfiGraphicsOutputProtocolGuid,    &Private->GraphicsOutput,
                  NULL
                  );

  DEBUG((DEBUG_INFO, "BHYVE framebuffer device started\n"));

  //
  // Install int10 handler
  //
#ifndef CSM_ENABLE
  InstallVbeShim (L"Framebuffer", Private->FbAddr);
#endif

Done:
  if (EFI_ERROR (Status)) {
    if (Private != NULL) {
      //
      // On Error Free back private data
      //
      if (Private->ControllerNameTable != NULL) {
        FreeUnicodeStringTable (Private->ControllerNameTable);
      }

      gBS->FreePool (Private);
    }
  }

  return Status;
}



/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
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
EmuGopDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
  EFI_STATUS                   Status;
  GOP_PRIVATE_DATA             *Private;

  DEBUG((DEBUG_INFO, "BHYVE framebuffer device stopping\n"));

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    //
    // If the GOP interface does not exist the driver is not started
    //
    return EFI_NOT_STARTED;
  }

  //
  // Get our private context information
  //
  Private = GOP_PRIVATE_DATA_FROM_THIS (GraphicsOutput);

  //
  // Remove the SGO interface from the system
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Private->Handle,
                  &gEfiGraphicsOutputProtocolGuid,    &Private->GraphicsOutput,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Shutdown the hardware
    //
    Status = EmuGopDestructor (Private);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    gBS->CloseProtocol (
          Handle,
          &gEfiPciIoProtocolGuid,
          This->DriverBindingHandle,
          Private->Handle
          );

    //
    // Free our instance data
    //
    FreeUnicodeStringTable (Private->ControllerNameTable);

    gBS->FreePool (Private);

  }

  return Status;
}


///
/// This protocol provides the services required to determine if a driver supports a given controller.
/// If a controller is supported, then it also provides routines to start and stop the controller.
///
EFI_DRIVER_BINDING_PROTOCOL gEmuGopDriverBinding = {
  EmuGopDriverBindingSupported,
  EmuGopDriverBindingStart,
  EmuGopDriverBindingStop,
  0xa,
  NULL,
  NULL
};



/**
  The user Entry Point for module EmuGop. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeEmuGop (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gEmuGopDriverBinding,
             ImageHandle,
             &gEmuGopComponentName,
             &gEmuGopComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

STATIC VOID
BhyveGetGraphicsMode (
  EFI_PCI_IO_PROTOCOL *PciIo,
  UINT16              *Width,
  UINT16              *Height,
  UINT16              *Depth
  )
{
  BHYVE_FBUF_MEMREGS BhyveRegs;
  UINT64       Offset;
  EFI_STATUS   Status;


  Offset = (UINT64)&BhyveRegs.Width - (UINT64)&BhyveRegs;

  Status = PciIo->Mem.Read (
      PciIo,
      EfiPciIoWidthUint16,
      PCI_BAR_IDX0,
      Offset,
      3,
      &BhyveRegs.Width
      );

  *Width  = BhyveRegs.Width;
  *Height = BhyveRegs.Height;
  *Depth  = BhyveRegs.Depth;

  DEBUG ((DEBUG_INFO, "BHYVE Get Graphics Mode: w %d, h %d\n", *Width, *Height));

  ASSERT_EFI_ERROR (Status);
}

VOID
BhyveSetGraphicsMode (
  GOP_PRIVATE_DATA  *Private,
  UINT16             Width,
  UINT16             Height,
  UINT16             Depth
  )
{
  BHYVE_FBUF_MEMREGS BhyveRegs;
  UINT64       Offset;
  EFI_STATUS   Status;

  DEBUG ((DEBUG_INFO, "BHYVE Set Graphics Mode: w %d, h %d\n", Width, Height));

  BhyveRegs.Width  = Width;
  BhyveRegs.Height = Height;
  BhyveRegs.Depth  = Depth;
  Offset = (UINT64)&BhyveRegs.Width - (UINT64)&BhyveRegs;

  Status = Private->PciIo->Mem.Write (
      Private->PciIo,
      EfiPciIoWidthUint16,
      PCI_BAR_IDX0,
      Offset,
      3,
      &BhyveRegs.Width
      );
  ASSERT_EFI_ERROR (Status);
}

VOID
BhyveGetMemregs (
  GOP_PRIVATE_DATA  *Private,
  BHYVE_FBUF_MEMREGS *Memregs
  )
{
  EFI_STATUS   Status;

  Status = Private->PciIo->Mem.Read (
      Private->PciIo,
      EfiPciIoWidthUint32,
      PCI_BAR_IDX0,
      0,
      3,
      Memregs
      );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "BHYVE Get Memregs, size %d width %d height %d\n",
         Memregs->FbSize, Memregs->Width, Memregs->Height));
}
