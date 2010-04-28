/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    PciHotplugDevice.c
    
Abstract:


  GUIDs used to indicate the device is Pccard hotplug device

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION(PciHotPlugDevice)


EFI_GUID  gEfiPciHotplugDeviceGuid = EFI_PCI_HOTPLUG_DEVICE_GUID;

EFI_GUID_STRING(&gEfiPciHotplugDeviceGuid, "PCI Hotplug Device", "PCI Hotplug Device GUID in EFI System Table");
