/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  PciLanInfo.h

Abstract:

--*/

#ifndef _PCI_LAN_INFO_GUID_H_
#define _PCI_LAN_INFO_GUID_H_

#pragma pack(1)

//
// structure used for Pci Lan variable
//
typedef struct {
  UINT8         PciBus;
  UINT8         PciDevice;
  UINT8         PciFunction;
} PCI_LAN_INFO;

#pragma pack()

#define EFI_PCI_LAN_INFO_GUID \
  {0xd9a1427, 0xe02a, 0x437d, 0x92, 0x6b, 0xaa, 0x52, 0x1f, 0xd7, 0x22, 0xba};

extern EFI_GUID gEfiPciLanInfoGuid;

#endif
