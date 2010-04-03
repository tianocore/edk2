/** @file
  Support for PCI 2.3 standard.

  Copyright (c) 2006 - 2010, Intel Corporation.  All rights reserved<BR>                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php.                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _PCI23_H_
#define _PCI23_H_

#include <IndustryStandard/Pci22.h>

///
/// PCI_CLASS_MASS_STORAGE, Base Class 01h.
///
///@{
#define PCI_CLASS_MASS_STORAGE_ATA       0x05
#define   PCI_IF_MASS_STORAGE_SINGLE_DMA   0x20
#define   PCI_IF_MASS_STORAGE_CHAINED_DMA  0x30
///@}

///
/// PCI_CLASS_SERIAL, Base Class 0Ch.
///
///@{
#define   PCI_IF_EHCI                      0x20
#define PCI_CLASS_SERIAL_IB              0x06
///@}

///
/// defined in PCI Express Spec.
///
#define PCI_EXP_MAX_CONFIG_OFFSET     0x1000

///
/// PCI Capability List IDs and records.
///
#define EFI_PCI_CAPABILITY_ID_PCIX    0x07

#pragma pack(1)
///
/// PCI-X Capabilities List, 
/// Section 7.2, PCI-X Addendum to the PCI Local Bus Specification, Revision 1.0b.
///
typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT16                  CommandReg;
  UINT32                  StatusReg;
} EFI_PCI_CAPABILITY_PCIX;

///
/// PCI-X Bridge Capabilities List, 
/// Section 8.6.2, PCI-X Addendum to the PCI Local Bus Specification, Revision 1.0b.
///
typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT16                  SecStatusReg;
  UINT32                  StatusReg;
  UINT32                  SplitTransCtrlRegUp;
  UINT32                  SplitTransCtrlRegDn;
} EFI_PCI_CAPABILITY_PCIX_BRDG;

#pragma pack()

#define PCI_CODE_TYPE_EFI_IMAGE       0x03

#endif
