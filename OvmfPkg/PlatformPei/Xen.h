/** @file
  Ovmf info structure passed by Xen

Copyright (c) 2013, Citrix Systems UK Ltd.<BR>

This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __XEN_H__
#define __XEN_H__

#include <PiPei.h>

// Physical address of OVMF info
#define OVMF_INFO_PHYSICAL_ADDRESS 0x00001000

// This structure must match the definition on Xen side
#pragma pack(1)
typedef struct {
  CHAR8 Signature[14]; // XenHVMOVMF\0
  UINT8 Length;        // Length of this structure
  UINT8 Checksum;      // Set such that the sum over bytes 0..length == 0
  //
  // Physical address of an array of TablesCount elements.
  //
  // Each element contains the physical address of a BIOS table.
  //
  EFI_PHYSICAL_ADDRESS Tables;
  UINT32 TablesCount;
  //
  // Physical address of the E820 table, contains E820EntriesCount entries.
  //
  EFI_PHYSICAL_ADDRESS E820;
  UINT32 E820EntriesCount;
} EFI_XEN_OVMF_INFO;
#pragma pack()

#endif /* __XEN_H__ */
