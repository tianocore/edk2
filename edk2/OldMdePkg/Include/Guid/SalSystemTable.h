/** @file
  GUIDs used for SAL system table entries in the EFI system table.

  SAL System Table contains Itanium-based processor centric information about
  the system.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SalSystemTable.h

  @par Revision Reference:
  GUIDs defined in UEFI 2.0 spec.

**/

#ifndef __SAL_SYSTEM_TABLE_GUID_H__
#define __SAL_SYSTEM_TABLE_GUID_H__

#define EFI_SAL_SYSTEM_TABLE_GUID \
  { \
    0xeb9d2d32, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

//
// GUID name defined in spec.
//
#define SAL_SYSTEM_TABLE_GUID EFI_SAL_SYSTEM_TABLE_GUID

extern EFI_GUID gEfiSalSystemTableGuid;

#endif
