/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    IncompatiblePciDeviceSupport.h
    
Abstract:

    EFI Incompatible PCI Device Support Protocol

Revision History

--*/

#ifndef _INCOMPATIBLE_PCI_DEVICE_SUPPORT_H_
#define _INCOMPATIBLE_PCI_DEVICE_SUPPORT_H_

#define EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL_GUID \
        {0xeb23f55a, 0x7863, 0x4ac2, {0x8d, 0x3d, 0x95, 0x65, 0x35, 0xde, 0x03, 0x75}}

EFI_FORWARD_DECLARATION (EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT);

  
typedef
EFI_STATUS
(EFIAPI *EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_CHECK_DEVICE) (
  IN EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT   *This,
  IN  UINTN                                         VendorId,
  IN  UINTN                                         DeviceId,
  IN  UINTN                                         Revision,
  IN  UINTN                                         SubVendorId,OPTIONAL
  IN  UINTN                                         SubDeviceId,OPTIONAL
  OUT VOID                                          *Configuration
); 


//
// Interface structure for the Incompatible PCI Device Support Protocol
//
typedef struct _EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT {
  EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_CHECK_DEVICE      CheckDevice;  
} EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL;

extern EFI_GUID gEfiIncompatiblePciDeviceSupportProtocolGuid;
  
#endif
