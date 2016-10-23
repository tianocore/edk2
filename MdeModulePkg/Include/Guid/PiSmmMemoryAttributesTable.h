/** @file
  Define the GUID of the EDKII PI SMM memory attribute table, which
  is published by PI SMM Core.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PI_SMM_MEMORY_ATTRIBUTES_TABLE_H_
#define _PI_SMM_MEMORY_ATTRIBUTES_TABLE_H_

#define EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE_GUID {\
  0x6b9fd3f7, 0x16df, 0x45e8, {0xbd, 0x39, 0xb9, 0x4a, 0x66, 0x54, 0x1a, 0x5d} \
}

//
// The PI SMM memory attribute table contains the SMM memory map for SMM image.
//
// This table is installed to SMST as SMM configuration table.
//
// This table is published at gEfiSmmEndOfDxeProtocolGuid notification, because
// there should be no more SMM driver loaded after that. The EfiRuntimeServicesCode
// region should not be changed any more.
//
// This table is published, if and only if all SMM PE/COFF have aligned section
// as specified in UEFI specification Section 2.3. For example, IA32/X64 alignment is 4KiB.
//
// If this table is published, the EfiRuntimeServicesCode contains code only
// and it is EFI_MEMORY_RO; the EfiRuntimeServicesData contains data only
// and it is EFI_MEMORY_XP.
//
typedef struct {
  UINT32                Version;
  UINT32                NumberOfEntries;
  UINT32                DescriptorSize;
  UINT32                Reserved;
//EFI_MEMORY_DESCRIPTOR Entry[1];
} EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE;

#define EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE_VERSION  0x00000001

extern EFI_GUID gEdkiiPiSmmMemoryAttributesTableGuid;

#endif
