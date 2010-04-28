/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 PciCfg.h

Abstract:

  PciCfg PPI as defined in PEI CIS specification

  Used to access PCI configuration space in PEI

--*/

#ifndef _PEI_PCI_CFG_H_
#define _PEI_PCI_CFG_H_
#include "EfiPciCfg.h"

#define PEI_PCI_CFG_PPI_GUID \
  { \
    0xe1f2eba0, 0xf7b9, 0x4a26, {0x86, 0x20, 0x13, 0x12, 0x21, 0x64, 0x2a, 0x90} \
  }

EFI_FORWARD_DECLARATION (PEI_PCI_CFG_PPI);


typedef
EFI_STATUS
(EFIAPI *PEI_PCI_CFG_PPI_IO) (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_PCI_CFG_PPI          * This,
  IN PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                   Address,
  IN OUT VOID                 *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *PEI_PCI_CFG_PPI_RW) (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_PCI_CFG_PPI          * This,
  IN PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                   Address,
  IN UINTN                    SetBits,
  IN UINTN                    ClearBits
  );

struct _PEI_PCI_CFG_PPI {
  PEI_PCI_CFG_PPI_IO  Read;
  PEI_PCI_CFG_PPI_IO  Write;
  PEI_PCI_CFG_PPI_RW  Modify;
};

extern EFI_GUID gPeiPciCfgPpiInServiceTableGuid;

#endif
