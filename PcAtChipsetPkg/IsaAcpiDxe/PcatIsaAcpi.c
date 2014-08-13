/** @file
  EFI PCAT ISA ACPI Driver for a Generic PC Platform

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PcatIsaAcpi.h"

//
//  PcatIsaAcpi Driver Binding Protocol
//
EFI_DRIVER_BINDING_PROTOCOL gPcatIsaAcpiDriverBinding = {
  PcatIsaAcpiDriverBindingSupported,
  PcatIsaAcpiDriverBindingStart,
  PcatIsaAcpiDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  the entry point of the PcatIsaAcpi driver.

  @param ImageHandle     Handle for driver image
  @param SystemTable     Point to EFI_SYSTEM_TABLE

  @return Sucess or not for installing driver binding protocol
**/
EFI_STATUS
EFIAPI
PcatIsaAcpiDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle, 
           SystemTable, 
           &gPcatIsaAcpiDriverBinding,
           ImageHandle,
           &gPcatIsaAcpiComponentName,
           &gPcatIsaAcpiComponentName2
           );
}

/**
  ControllerDriver Protocol Method

  @param This                 Driver Binding protocol instance pointer.   
  @param Controller           Handle of device to test.
  @param RemainingDevicePath  Optional parameter use to pick a specific child
                              device to start.
  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
PcatIsaAcpiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;
  UINTN                SegmentNumber;
  UINTN                BusNumber;
  UINTN                DeviceNumber;
  UINTN                FunctionNumber;

  //
  // Get PciIo protocol instance
  //              
  Status = gBS->OpenProtocol (
                  Controller,  
                  &gEfiPciIoProtocolGuid, 
                  (VOID**)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = PciIo->Pci.Read (
                    PciIo,
                    EfiPciIoWidthUint32,
                    0,
                    sizeof(Pci) / sizeof(UINT32), 
                    &Pci);

  if (!EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    if ((Pci.Hdr.Command & 0x03) == 0x03) {
      if (Pci.Hdr.ClassCode[2] == PCI_CLASS_BRIDGE) {
        //
        // See if this is a standard PCI to ISA Bridge from the Base Code and Class Code
        //
        if (Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_ISA) {
          Status = EFI_SUCCESS;
        } 

        //
        // See if this is an Intel PCI to ISA bridge in Positive Decode Mode
        //
        if (Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_ISA_PDECODE && 
            Pci.Hdr.VendorId     == 0x8086                          ) {
          //
          // See if this is on Function #0 to avoid false positives on 
          // PCI_CLASS_BRIDGE_OTHER that has the same value as 
          // PCI_CLASS_BRIDGE_ISA_PDECODE
          //
          Status = PciIo->GetLocation (
                            PciIo, 
                            &SegmentNumber, 
                            &BusNumber, 
                            &DeviceNumber, 
                            &FunctionNumber
                            );
          if (!EFI_ERROR (Status) && FunctionNumber == 0) {
            Status = EFI_SUCCESS;
          } else {
            Status = EFI_UNSUPPORTED;
          }
        }
      } 
    }
  }

  gBS->CloseProtocol (
         Controller,       
         &gEfiPciIoProtocolGuid, 
         This->DriverBindingHandle,   
         Controller   
         );
  
  return Status;
}

/**
  Install EFI_ISA_ACPI_PROTOCOL.

  @param  This                 Driver Binding protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device
**/
EFI_STATUS
EFIAPI
PcatIsaAcpiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCAT_ISA_ACPI_DEV    *PcatIsaAcpiDev;
  UINT64               Supports;
  BOOLEAN              Enabled;

  Enabled = FALSE;
  Supports = 0;
  PcatIsaAcpiDev = NULL;
  //
  // Open the PCI I/O Protocol Interface
  //
  PciIo = NULL;
  Status = gBS->OpenProtocol (
                  Controller,       
                  &gEfiPciIoProtocolGuid, 
                  (VOID**)&PciIo,
                  This->DriverBindingHandle,   
                  Controller,   
                  EFI_OPEN_PROTOCOL_BY_DRIVER 
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Get supported PCI attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Supports &= (UINT64) (EFI_PCI_IO_ATTRIBUTE_ISA_IO | EFI_PCI_IO_ATTRIBUTE_ISA_IO_16);
  if (Supports == 0 || Supports == (EFI_PCI_IO_ATTRIBUTE_ISA_IO | EFI_PCI_IO_ATTRIBUTE_ISA_IO_16)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }  

  Enabled = TRUE;
  Status = PciIo->Attributes (
                    PciIo, 
                    EfiPciIoAttributeOperationEnable, 
                    EFI_PCI_DEVICE_ENABLE | Supports | EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO, 
                    NULL 
                    );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  //
  // Allocate memory for the PCAT ISA ACPI Device structure
  //
  PcatIsaAcpiDev = NULL;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof(PCAT_ISA_ACPI_DEV),
                  (VOID**)&PcatIsaAcpiDev
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Initialize the PCAT ISA ACPI Device structure
  //
  PcatIsaAcpiDev->Signature = PCAT_ISA_ACPI_DEV_SIGNATURE;
  PcatIsaAcpiDev->Handle    = Controller;
  PcatIsaAcpiDev->PciIo     = PciIo;

  //
  // Initialize PcatIsaAcpiDeviceList
  //
  InitializePcatIsaAcpiDeviceList ();
  
  //
  // IsaAcpi interface
  //
  (PcatIsaAcpiDev->IsaAcpi).DeviceEnumerate  = IsaDeviceEnumerate;
  (PcatIsaAcpiDev->IsaAcpi).SetPower         = IsaDeviceSetPower;
  (PcatIsaAcpiDev->IsaAcpi).GetCurResource   = IsaGetCurrentResource;
  (PcatIsaAcpiDev->IsaAcpi).GetPosResource   = IsaGetPossibleResource;
  (PcatIsaAcpiDev->IsaAcpi).SetResource      = IsaSetResource;
  (PcatIsaAcpiDev->IsaAcpi).EnableDevice     = IsaEnableDevice;
  (PcatIsaAcpiDev->IsaAcpi).InitDevice       = IsaInitDevice;
  (PcatIsaAcpiDev->IsaAcpi).InterfaceInit    = IsaInterfaceInit;
    
  //
  // Install the ISA ACPI Protocol interface
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiIsaAcpiProtocolGuid, &PcatIsaAcpiDev->IsaAcpi,
                  NULL
                  );

Done:
  if (EFI_ERROR (Status)) {
    if (PciIo != NULL && Enabled) {
      PciIo->Attributes (
               PciIo, 
               EfiPciIoAttributeOperationDisable, 
               EFI_PCI_DEVICE_ENABLE | Supports | EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO,
               NULL 
               );
    }
    gBS->CloseProtocol (
           Controller, 
           &gEfiPciIoProtocolGuid, 
           This->DriverBindingHandle, 
           Controller
           );
    if (PcatIsaAcpiDev != NULL) {
      gBS->FreePool (PcatIsaAcpiDev);
    }
    return Status;
  }
          
  return EFI_SUCCESS;
}


/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
PcatIsaAcpiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS             Status;
  EFI_ISA_ACPI_PROTOCOL  *IsaAcpi;
  PCAT_ISA_ACPI_DEV      *PcatIsaAcpiDev;
  UINT64                 Supports;
  
  //
  // Get the ISA ACPI Protocol Interface
  // 
  Status = gBS->OpenProtocol (
                  Controller, 
                  &gEfiIsaAcpiProtocolGuid, 
                  (VOID**)&IsaAcpi,
                  This->DriverBindingHandle,   
                  Controller,   
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the PCAT ISA ACPI Device structure from the ISA ACPI Protocol
  //
  PcatIsaAcpiDev = PCAT_ISA_ACPI_DEV_FROM_THIS (IsaAcpi);

  //
  // Get supported PCI attributes
  //
  Status = PcatIsaAcpiDev->PciIo->Attributes (
                                    PcatIsaAcpiDev->PciIo,
                                    EfiPciIoAttributeOperationSupported,
                                    0,
                                    &Supports
                                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Supports &= (UINT64) (EFI_PCI_IO_ATTRIBUTE_ISA_IO | EFI_PCI_IO_ATTRIBUTE_ISA_IO_16);

  PcatIsaAcpiDev->PciIo->Attributes (
                           PcatIsaAcpiDev->PciIo, 
                           EfiPciIoAttributeOperationDisable, 
                           EFI_PCI_DEVICE_ENABLE | Supports | EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO,
                           NULL 
                           );
 
  //
  // Uninstall protocol interface: EFI_ISA_ACPI_PROTOCOL
  //
  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiIsaAcpiProtocolGuid, &PcatIsaAcpiDev->IsaAcpi
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller, 
         &gEfiPciIoProtocolGuid, 
         This->DriverBindingHandle, 
         Controller
         );
  
  gBS->FreePool (PcatIsaAcpiDev);
  
  return EFI_SUCCESS;
}
