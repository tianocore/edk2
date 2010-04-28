/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    EfiPci.h

Abstract:
    Support for EFI PCI specification.

Revision History

--*/

#ifndef _EFI_PCI_H_
#define _EFI_PCI_H_

//#include "pci22.h"
//#include "pci23.h"
//#include "pci30.h"

#pragma pack(1)

typedef struct {
  UINT8 Register;
  UINT8 Function;
  UINT8 Device;
  UINT8 Bus;
  UINT8 Reserved[4];
} DEFIO_PCI_ADDR;

#define EFI_ROOT_BRIDGE_LIST                            'eprb'
#define EFI_PCI_EXPANSION_ROM_HEADER_EFISIGNATURE       0x0EF1

typedef struct {
  UINT16  Signature;    // 0xaa55
  UINT16  InitializationSize;
  UINT32  EfiSignature; // 0x0EF1
  UINT16  EfiSubsystem;
  UINT16  EfiMachineType;
  UINT16  CompressionType;
  UINT8   Reserved[8];
  UINT16  EfiImageHeaderOffset;
  UINT16  PcirOffset;
} EFI_PCI_EXPANSION_ROM_HEADER;

typedef union {
  UINT8                           *Raw;
  PCI_EXPANSION_ROM_HEADER        *Generic;
  EFI_PCI_EXPANSION_ROM_HEADER    *Efi;
  EFI_LEGACY_EXPANSION_ROM_HEADER *PcAt;
} EFI_PCI_ROM_HEADER;

#pragma pack()

#endif
