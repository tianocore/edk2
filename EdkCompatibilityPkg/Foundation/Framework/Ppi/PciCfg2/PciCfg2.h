/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 PciCfg2.h

Abstract:

  PciCfg2 PPI as defined in PI1.0 specification

  Used to access PCI configuration space in PEI

--*/

#ifndef _PEI_PCI_CFG2_H_
#define _PEI_PCI_CFG2_H_
#include "EfiPciCfg.h"

#define EFI_PEI_PCI_CFG2_PPI_GUID \
  { \
    0x57a449a, 0x1fdc, 0x4c06, {0xbf, 0xc9, 0xf5, 0x3f, 0x6a, 0x99, 0xbb, 0x92} \
  }

EFI_FORWARD_DECLARATION (EFI_PEI_PCI_CFG2_PPI);


typedef
EFI_STATUS
(EFIAPI *EFI_PEI_PCI_CFG_PPI_IO) (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN CONST EFI_PEI_PCI_CFG2_PPI     *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH      Width,
  IN UINT64                         Address,
  IN OUT VOID                       *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_PCI_CFG_PPI_RW) (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN CONST EFI_PEI_PCI_CFG2_PPI     *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH      Width,
  IN UINT64                         Address,
  IN VOID                           *SetBits,
  IN VOID                           *ClearBits
  );

struct _EFI_PEI_PCI_CFG2_PPI {
  EFI_PEI_PCI_CFG_PPI_IO  Read;
  EFI_PEI_PCI_CFG_PPI_IO  Write;
  EFI_PEI_PCI_CFG_PPI_RW  Modify;
  UINT16                  Segment;
};

extern EFI_GUID gPeiPciCfg2PpiGuid;

#endif
