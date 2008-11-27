/** @file
  This file defines GUIDs used for variable list entries put in the EFI system table.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __VARIABLE_INFO_GUID_H__
#define __VARIABLE_INFO_GUID_H__

// {DDCF3616-3275-4164-98B6-FE85707FFE7D}
#define EFI_VARIABLE_INFO_GUID \
  { 0xddcf3616, 0x3275, 0x4164, { 0x98, 0xb6, 0xfe, 0x85, 0x70, 0x7f, 0xfe, 0x7d } }

extern EFI_GUID gEfiVariableInfoGuid;


typedef struct _VARIABLE_INFO_ENTRY  VARIABLE_INFO_ENTRY;

///
/// This list gets put in the EFI system table. It is produced by the Variable driver at
/// Boot Services time and records read and write access to a given variable
///
struct _VARIABLE_INFO_ENTRY {
  VARIABLE_INFO_ENTRY *Next;       /// Pointer to next entry
  EFI_GUID            VendorGuid;  /// Guid of Variable 
  CHAR16              *Name;       /// Name of Variable 
  UINT32              Attributes;  /// Attributes of variable defined in UEFI spec
  UINT32              ReadCount;   /// Times to read this variable
  UINT32              WriteCount;  /// Times to write this variable
  UINT32              DeleteCount; /// Times to delete this variable
  UINT32              CacheCount;  /// Times that cache hits this variable
  BOOLEAN             Volatile;    /// TRUE if volatile FALSE if non-volatile
};

#endif
