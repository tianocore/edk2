/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:

    PcatIsaAcpi.c
    
Abstract:

    EFI PCAT ISA ACPI Driver for a Generic PC Platform

Revision History

--*/

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

EFI_STATUS
EFIAPI
PcatIsaAcpiDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++
  
  Routine Description:
    the entry point of the PcatIsaAcpi driver
  
  Arguments:
  
  Returns:
    
--*/                
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

EFI_STATUS
EFIAPI
PcatIsaAcpiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

  ControllerDriver Protocol Method

Arguments:

Returns:

--*/
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;

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
            Pci.Hdr.VendorId == 0x8086 && 
            Pci.Hdr.DeviceId == 0x7110) {
          Status = EFI_SUCCESS;
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

EFI_STATUS
EFIAPI
PcatIsaAcpiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:
  Install EFI_ISA_ACPI_PROTOCOL

Arguments:

Returns:

--*/
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCAT_ISA_ACPI_DEV    *PcatIsaAcpiDev;
    
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

  Status = PciIo->Attributes (
                    PciIo, 
                    EfiPciIoAttributeOperationEnable, 
                    EFI_PCI_DEVICE_ENABLE | EFI_PCI_IO_ATTRIBUTE_ISA_IO | EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO, 
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
    if (PciIo) {
      PciIo->Attributes (
               PciIo, 
               EfiPciIoAttributeOperationDisable, 
               EFI_PCI_DEVICE_ENABLE | EFI_PCI_IO_ATTRIBUTE_ISA_IO | EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO,
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

EFI_STATUS
EFIAPI
PcatIsaAcpiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
/*++

  Routine Description:

  Arguments:

  Returns:

--*/
{
  EFI_STATUS             Status;
  EFI_ISA_ACPI_PROTOCOL  *IsaAcpi;
  PCAT_ISA_ACPI_DEV      *PcatIsaAcpiDev;
  
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

  PcatIsaAcpiDev->PciIo->Attributes (
                           PcatIsaAcpiDev->PciIo, 
                           EfiPciIoAttributeOperationDisable, 
                           EFI_PCI_DEVICE_ENABLE | EFI_PCI_IO_ATTRIBUTE_ISA_IO | EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO,
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
