/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:

    PcatIsaAcpi.h
    
Abstract:

    EFI PCAT ISA ACPI Driver for a Generic PC Platform

Revision History

--*/

#ifndef _PCAT_ISA_ACPI_H_
#define _PCAT_ISA_ACPI_H_

#include <PiDxe.h>

#include <IndustryStandard/Pci.h>

#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/IsaIo.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>


#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/IsaAcpi.h>
//
// PCAT ISA ACPI device private data structure
//
#define PCAT_ISA_ACPI_DEV_SIGNATURE  EFI_SIGNATURE_32('L','P','C','D')

typedef struct {
  UINTN                  Signature;
  EFI_HANDLE             Handle;    
  EFI_ISA_ACPI_PROTOCOL  IsaAcpi;
  EFI_PCI_IO_PROTOCOL    *PciIo;
} PCAT_ISA_ACPI_DEV;

#define PCAT_ISA_ACPI_DEV_FROM_THIS(a) _CR(a, PCAT_ISA_ACPI_DEV, IsaAcpi)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL gPcatIsaAcpiDriverBinding;

extern EFI_COMPONENT_NAME2_PROTOCOL gPcatIsaAcpiComponentName2;

extern EFI_COMPONENT_NAME_PROTOCOL  gPcatIsaAcpiComponentName;


//
// Prototypes for Driver model protocol interface
//
EFI_STATUS
EFIAPI
PcatIsaAcpiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
PcatIsaAcpiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
PcatIsaAcpiDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

//
// Prototypes for the ISA ACPI protocol interface
//
EFI_STATUS
EFIAPI
IsaDeviceEnumerate (
  IN  EFI_ISA_ACPI_PROTOCOL   *This,
  OUT EFI_ISA_ACPI_DEVICE_ID  **Device
  );

EFI_STATUS
EFIAPI
IsaDeviceSetPower (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device,
  IN BOOLEAN                 OnOff
  );
  
EFI_STATUS
EFIAPI
IsaGetCurrentResource (
  IN  EFI_ISA_ACPI_PROTOCOL       *This,
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  );
  
EFI_STATUS
EFIAPI
IsaGetPossibleResource (
  IN  EFI_ISA_ACPI_PROTOCOL       *This,
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,  
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  );
  
EFI_STATUS
EFIAPI
IsaSetResource (
  IN EFI_ISA_ACPI_PROTOCOL       *This,
  IN EFI_ISA_ACPI_DEVICE_ID      *Device,
  IN EFI_ISA_ACPI_RESOURCE_LIST  *ResourceList
  );
  
EFI_STATUS
EFIAPI
IsaEnableDevice (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device,
  IN BOOLEAN                 Enable
  );

EFI_STATUS
EFIAPI
IsaInitDevice (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device
  );
  
EFI_STATUS
EFIAPI
IsaInterfaceInit (
  IN EFI_ISA_ACPI_PROTOCOL  *This
  );  

#endif
