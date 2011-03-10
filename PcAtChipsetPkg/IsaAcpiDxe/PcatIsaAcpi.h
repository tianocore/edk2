/** @file
  EFI PCAT ISA ACPI Driver for a Generic PC Platform

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

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
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>

#include <Protocol/IsaAcpi.h>
//
// PCAT ISA ACPI device private data structure
//
#define PCAT_ISA_ACPI_DEV_SIGNATURE  SIGNATURE_32('L','P','C','D')

typedef struct {
  UINTN                  Signature;
  EFI_HANDLE             Handle;    
  EFI_ISA_ACPI_PROTOCOL  IsaAcpi;
  EFI_PCI_IO_PROTOCOL    *PciIo;
} PCAT_ISA_ACPI_DEV;

#define PCAT_ISA_ACPI_DEV_FROM_THIS(a) BASE_CR(a, PCAT_ISA_ACPI_DEV, IsaAcpi)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL gPcatIsaAcpiDriverBinding;

extern EFI_COMPONENT_NAME2_PROTOCOL gPcatIsaAcpiComponentName2;

extern EFI_COMPONENT_NAME_PROTOCOL  gPcatIsaAcpiComponentName;


//
// Prototypes for Driver model protocol interface
//
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
  );

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
  );

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
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

//
// Prototypes for the ISA ACPI protocol interface
//
/**
  Enumerate the ISA devices on the ISA bus


  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 

  @retval EFI_NOT_FOUND Can not found the next Isa device.
  @retval EFI_SUCESS    Success retrieve the next Isa device for enumration.

**/
EFI_STATUS
EFIAPI
IsaDeviceEnumerate (
  IN  EFI_ISA_ACPI_PROTOCOL   *This,
  OUT EFI_ISA_ACPI_DEVICE_ID  **Device
  );

/**
  Set ISA device power


  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 
  @param OnOff           TRUE for setting isa device power on,
                         FALSE for setting isa device power off

  @return EFI_SUCCESS    Sucess to change power status for isa device.
**/
EFI_STATUS
EFIAPI
IsaDeviceSetPower (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device,
  IN BOOLEAN                 OnOff
  );
  
/**
  Get current resource for the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 
  @param ResourceList    On return, point to resources instances for given isa device

  @retval EFI_NOT_FOUND Can not found the resource instance for given isa device
  @retval EFI_SUCCESS   Success to get resource instance for given isa device.
**/
EFI_STATUS
EFIAPI
IsaGetCurrentResource (
  IN  EFI_ISA_ACPI_PROTOCOL       *This,
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  );
  
/**
  Get possible resource for the specific ISA device.
  
  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 
  @param ResourceList    On return, point to resources instances for given isa device

  @retval EFI_SUCCESS   Success to get resource instance for given isa device.
**/
EFI_STATUS
EFIAPI
IsaGetPossibleResource (
  IN  EFI_ISA_ACPI_PROTOCOL       *This,
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,  
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  );
  
/**
  Set resource for the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 
  @param ResourceList    Point to resources instances for given isa device

  @return EFI_SUCESS  Success to set resource.

**/
EFI_STATUS
EFIAPI
IsaSetResource (
  IN EFI_ISA_ACPI_PROTOCOL       *This,
  IN EFI_ISA_ACPI_DEVICE_ID      *Device,
  IN EFI_ISA_ACPI_RESOURCE_LIST  *ResourceList
  );
  
/**
  Enable/Disable the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 
  @param Enable          Enable/Disable

  @return EFI_SUCESS  Success to enable/disable.

**/
EFI_STATUS
EFIAPI
IsaEnableDevice (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device,
  IN BOOLEAN                 Enable
  );

/**
  Initialize the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance 

  @return EFI_SUCESS  Success to initialize.

**/
EFI_STATUS
EFIAPI
IsaInitDevice (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device
  );
  
/**
  Initialize the ISA interface.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL

  @return EFI_SUCESS  Success to initialize ISA interface.

**/
EFI_STATUS
EFIAPI
IsaInterfaceInit (
  IN EFI_ISA_ACPI_PROTOCOL  *This
  );  

/**
  Initialize the ISA device list.
**/
VOID
InitializePcatIsaAcpiDeviceList (
  VOID
  );

#endif
