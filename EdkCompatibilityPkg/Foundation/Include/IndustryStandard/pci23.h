/*++

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    pci23.h

Abstract:
    Support for PCI 2.3 standard.

Revision History

--*/

#ifndef _PCI23_H
#define _PCI23_H

#include "pci22.h"

//
// PCI_CLASS_MASS_STORAGE
//
#define PCI_CLASS_MASS_STORAGE_ATA    0x05

//
// PCI_CLASS_SERIAL
//
#define PCI_CLASS_SERIAL_IB           0x06

#define PCI_EXP_MAX_CONFIG_OFFSET     0x1000
#define EFI_PCI_CAPABILITY_ID_PCIEXP  0x10

#endif
