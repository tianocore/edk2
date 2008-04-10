/**@file
  GUIDs used to indicate the device is Pccard hotplug device
  
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __PCI_HOTPLUG_DEVICE_GUID_H_
#define __PCI_HOTPLUG_DEVICE_GUID_H_

#define EFI_PCI_HOTPLUG_DEVICE_GUID \
  { 0x0b280816, 0x52e7, 0x4e51, {0xaa, 0x57, 0x11, 0xbd, 0x41, 0xcb, 0xef, 0xc3 } }

extern EFI_GUID gEfiPciHotplugDeviceGuid;

#endif // __PCI_HOTPLUG_DEVICE_GUID_H_

