/** @file
  Support for the PCI Express 3.0 standard.

  This header file may not define all structures.  Please extend as required.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _PCIEXPRESS30_H_
#define _PCIEXPRESS30_H_

#include "PciExpress21.h"

#define PCI_EXPRESS_EXTENDED_CAPABILITY_SECONDARY_PCIE_ID    0x0019
#define PCI_EXPRESS_EXTENDED_CAPABILITY_SECONDARY_PCIE_VER1  0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER                Header;
  UINT32                                                  LinkControl3;
  UINT32                                                  LaneErrorStatus;
  UINT16                                                  EqualizationControl[2];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_SECONDARY_PCIE;

#endif
