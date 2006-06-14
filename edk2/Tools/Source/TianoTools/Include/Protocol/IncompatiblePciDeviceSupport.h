/** @file
  This file declares EFI Incompatible PCI Device Support Protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  IncompatiblePciDeviceSupport.h

  @par Revision Reference:
  This protocol is defined in Framework of EFI PCI Platform Support Specification.
  Version0.9

**/

#ifndef _INCOMPATIBLE_PCI_DEVICE_SUPPORT_H_
#define _INCOMPATIBLE_PCI_DEVICE_SUPPORT_H_

#define EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL_GUID \
        {0xeb23f55a, 0x7863, 0x4ac2, {0x8d, 0x3d, 0x95, 0x65, 0x35, 0xde, 0x03, 0x75} }

typedef struct _EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL;

/**
  Returns a list of ACPI resource descriptors that detail the special 
  resource configuration requirements for an incompatible PCI device.

  @param  This Pointer to the EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL instance. 
  
  @param  VendorID A unique ID to identify the manufacturer of the PCI device.
  
  @param  DeviceID A unique ID to identify the particular PCI device.
  
  @param  RevisionID A PCI device-specific revision identifier.
  
  @param  SubsystemVendorId Specifies the subsystem vendor ID. 
  
  @param  SubsystemDeviceId Specifies the subsystem device ID.
  
  @param  Configuration A list of ACPI resource descriptors that detail 
  the configuration requirement. 

  @retval EFI_SUCCESS The function always returns EFI_SUCCESS.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_CHECK_DEVICE) (
  IN EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL   *This,
  IN  UINTN                                         VendorId,
  IN  UINTN                                         DeviceId,
  IN  UINTN                                         Revision,
  IN  UINTN                                         SubVendorId,OPTIONAL
  IN  UINTN                                         SubDeviceId,OPTIONAL
  OUT VOID                                          **Configuration
); 


//
// Interface structure for the Incompatible PCI Device Support Protocol
//
/**
  @par Protocol Description:
  This protocol can find some incompatible PCI devices and report their 
  special resource requirements to the PCI bus driver.

  @param CheckDevice
  Returns a list of ACPI resource descriptors that detail any special 
  resource configuration requirements if the specified device is a recognized 
  incompatible PCI device. 

**/
struct _EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL {
  EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_CHECK_DEVICE      CheckDevice;  
};

extern EFI_GUID gEfiIncompatiblePciDeviceSupportProtocolGuid;
  
#endif
