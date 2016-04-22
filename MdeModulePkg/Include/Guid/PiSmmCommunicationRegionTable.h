/** @file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PI_SMM_COMMUNICATION_REGION_TABLE_H_
#define _PI_SMM_COMMUNICATION_REGION_TABLE_H_

#define EDKII_PI_SMM_COMMUNICATION_REGION_TABLE_GUID {\
  0x4e28ca50, 0xd582, 0x44ac, {0xa1, 0x1f, 0xe3, 0xd5, 0x65, 0x26, 0xdb, 0x34} \
}

//
// This table to declare the generic SMM communication buffer location.
// If this table is present, it means the SMM communication buffer is restricted to
// EfiReservedMemoryType, EfiACPIMemoryNVS, or EfiRuntimeServicesData.
//
// This table is installed to UEFI configuration table by generic driver
// or platform driver, at early DXE phase.
// 
// The EFI_MEMORY_DESCRIPTOR entry must contain at least one entry.
// The entries must be normal memory region in EfiReservedMemoryType, EfiACPIMemoryNVS,
// or EfiRuntimeServicesData.
// If the Entry.Type is EfiConventionalMemory, it means this entry is free to use.
// If the Entry.Type is other, it means this entry is occupied.
//
// Any non-SMM component may put communication data there, then use 
// UEFI defined SMM Communication ACPI Table, or PI defined EFI_SMM_COMMUNICATION_PROTOCOL
// to communicate with SMI handler. The process is:
// 1) Find an entry whose type is EfiConventional.
// 2) Change type to be EfiReservedMemoryType before use.
// 3) Use it.
// 4) Restore type be EfiConventional.
//    The step 2) must be performed as an atomic transaction, if there might be conflict during runtime.
//    For example, on IA-32/x64 platforms, this can be done using the CMPXCHG CPU instruction.
//    If there is guarantee on no conflict during boot time, these steps can be skipped.
//    For example, DXE, UEFI driver and UEFI application runs in sequence.
//
// For example, FPDT driver can use this communication buffer to get SMM
// performance data in SMM. Profile driver can use this communication buffer
// to get SMM profile data in SMM.
//
typedef struct {
  UINT32                Version;
  UINT32                NumberOfEntries;
  UINT32                DescriptorSize;
  UINT32                Reserved;
//EFI_MEMORY_DESCRIPTOR Entry[1];
} EDKII_PI_SMM_COMMUNICATION_REGION_TABLE;

#define EDKII_PI_SMM_COMMUNICATION_REGION_TABLE_VERSION  0x00000001

extern EFI_GUID gEdkiiPiSmmCommunicationRegionTableGuid;

#endif
