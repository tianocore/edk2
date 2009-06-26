/** @file
  The incompatible PCI device list template.

Copyright (c) 2006 - 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

**/

#ifndef _EFI_INCOMPATIBLE_PCI_DEVICE_LIST_H_
#define _EFI_INCOMPATIBLE_PCI_DEVICE_LIST_H_

#include <Library/PciIncompatibleDeviceSupportLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>


#define PCI_DEVICE_ID(VendorId, DeviceId, Revision, SubVendorId, SubDeviceId) \
    VendorId, DeviceId, Revision, SubVendorId, SubDeviceId

#define PCI_BAR_TYPE_IO   ACPI_ADDRESS_SPACE_TYPE_IO
#define PCI_BAR_TYPE_MEM  ACPI_ADDRESS_SPACE_TYPE_MEM

#define DEVICE_INF_TAG    0xFFF2
#define DEVICE_RES_TAG    0xFFF1
#define LIST_END_TAG      0x0000

//
// descriptor for access width of incompatible PCI device
//
typedef struct {
  UINT64                         AccessType;
  UINT64                         AccessWidth;
  EFI_PCI_REGISTER_ACCESS_DATA   PciRegisterAccessData;
} EFI_PCI_REGISTER_ACCESS_DESCRIPTOR;

//
// descriptor for register value of incompatible PCI device
//
typedef struct {
  UINT64                         AccessType;
  UINT64                         Offset;
  EFI_PCI_REGISTER_VALUE_DATA    PciRegisterValueData;
} EFI_PCI_REGISTER_VALUE_DESCRIPTOR;

#endif
