/** @file
  This code supports the implementation of the Smbios protocol
  
Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _SMBIOS_DXE_H_
#define _SMBIOS_DXE_H_


#include <PiDxe.h>

#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>
#include <Guid/EventGroup.h>
#include <Guid/SmBios.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>

#define SMBIOS_INSTANCE_SIGNATURE SIGNATURE_32 ('S', 'B', 'i', 's')
typedef struct {
  UINT32                Signature;
  EFI_HANDLE            Handle;
  //
  // Produced protocol
  //
  EFI_SMBIOS_PROTOCOL   Smbios;
  //
  // Updates to record list must be locked.
  //
  EFI_LOCK              DataLock;
  //
  // List of EFI_SMBIOS_ENTRY structures.
  //
  LIST_ENTRY            DataListHead;
  //
  // List of allocated SMBIOS handle.
  //
  LIST_ENTRY            AllocatedHandleListHead;
} SMBIOS_INSTANCE;

#define SMBIOS_INSTANCE_FROM_THIS(this)  CR (this, SMBIOS_INSTANCE, Smbios, SMBIOS_INSTANCE_SIGNATURE)

//
// SMBIOS record Header
//
// An SMBIOS internal Record is an EFI_SMBIOS_RECORD_HEADER followed by (RecordSize - HeaderSize) bytes of
//  data. The format of the data is defined by the SMBIOS spec.
//
//
#define EFI_SMBIOS_RECORD_HEADER_VERSION  0x0100
typedef struct {
  UINT16      Version;
  UINT16      HeaderSize;
  UINTN       RecordSize;
  EFI_HANDLE  ProducerHandle;
  UINTN       NumberOfStrings;
} EFI_SMBIOS_RECORD_HEADER;


//
// Private data structure to contain the SMBIOS record. One record per
//  structure. SmbiosRecord is a copy of the data passed in and follows RecordHeader .
//
#define EFI_SMBIOS_ENTRY_SIGNATURE  SIGNATURE_32 ('S', 'r', 'e', 'c')
typedef struct {
  UINT32                    Signature;
  LIST_ENTRY                Link;
  EFI_SMBIOS_RECORD_HEADER  *RecordHeader;
  UINTN                     RecordSize;
  //
  // Indicate which table this record is added to.
  //
  BOOLEAN                   Smbios32BitTable;
  BOOLEAN                   Smbios64BitTable;
} EFI_SMBIOS_ENTRY;

#define SMBIOS_ENTRY_FROM_LINK(link)  CR (link, EFI_SMBIOS_ENTRY, Link, EFI_SMBIOS_ENTRY_SIGNATURE)

//
// Private data to contain the Smbios handle that already allocated.
//
#define SMBIOS_HANDLE_ENTRY_SIGNATURE  SIGNATURE_32 ('S', 'h', 'r', 'd')

typedef struct {
  UINT32               Signature;
  LIST_ENTRY           Link;
  //
  // Filter driver will register what record guid filter should be used.
  //
  EFI_SMBIOS_HANDLE    SmbiosHandle;

} SMBIOS_HANDLE_ENTRY;

#define SMBIOS_HANDLE_ENTRY_FROM_LINK(link)  CR (link, SMBIOS_HANDLE_ENTRY, Link, SMBIOS_HANDLE_ENTRY_SIGNATURE)

typedef struct {
  EFI_SMBIOS_TABLE_HEADER  Header;
  UINT8                    Tailing[2];
} EFI_SMBIOS_TABLE_END_STRUCTURE;

/**
  Create Smbios Table and installs the Smbios Table to the System Table.
  
  @param  Smbios32BitTable    The flag to update 32-bit table.
  @param  Smbios64BitTable    The flag to update 64-bit table.
  
**/
VOID
EFIAPI
SmbiosTableConstruction (
  BOOLEAN     Smbios32BitTable,
  BOOLEAN     Smbios64BitTable
  );

#endif
