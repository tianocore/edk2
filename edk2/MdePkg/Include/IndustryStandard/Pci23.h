/** @file
  Support for PCI 2.3 standard.

  Copyright (c) 2006 - 2008, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _PCI23_H_
#define _PCI23_H_

#include <IndustryStandard/Pci22.h>

///
/// Definitions of PCI class bytes and manipulation macros.
///
#define PCI_IF_EHCI                   0x20

///
/// defined in PCI Express Spec.
///
#define PCI_EXP_MAX_CONFIG_OFFSET     0x1000

//
// PCI Capability List IDs and records
//
#define EFI_PCI_CAPABILITY_ID_PCIX    0x07

#pragma pack(1)
///
/// Capability EFI_PCI_CAPABILITY_ID_PCIX, defined in PCI-X Addendum to the PCI Local Bus Specification
///
typedef struct {
  EFI_PCI_CAPABILITY_HDR  Hdr;
  UINT16                  CommandReg;
  UINT32                  StatusReg;
} EFI_PCI_CAPABILITY_PCIX;

/// 
/// Capability EFI_PCI_CAPABILITY_PCIX_BRDG, defined in PCI-X Addendum to the PCI Local Bus Specification
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
