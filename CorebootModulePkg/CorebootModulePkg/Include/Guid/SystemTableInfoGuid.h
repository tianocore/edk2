/** @file
  This file defines the hob structure for system tables like ACPI, SMBIOS tables.
  
  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SYSTEM_TABLE_INFO_GUID_H__
#define __SYSTEM_TABLE_INFO_GUID_H__

///
/// System Table Information GUID
///
extern EFI_GUID gUefiSystemTableInfoGuid;

typedef struct {  
  UINT64             AcpiTableBase;
  UINT32             AcpiTableSize;  
  UINT64             SmbiosTableBase;    
  UINT32             SmbiosTableSize;  
} SYSTEM_TABLE_INFO;  
  
#endif
