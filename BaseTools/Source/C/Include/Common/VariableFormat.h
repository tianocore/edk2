/**@file
  Header file for EFI Variable Services.

  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at:
    http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  File Name: VariableFormat.h

**/

#ifndef __VARIABLE_FORMAT_H__
#define __VARIABLE_FORMAT_H__

#define VARIABLE_DATA                     0x55AA

//
// Variable Store Header flags
//
#define VARIABLE_STORE_FORMATTED          0x5a
#define VARIABLE_STORE_HEALTHY            0xfe

#pragma pack(1)

typedef struct {
  EFI_GUID  Signature;
  UINT32    Size;
  UINT8     Format;
  UINT8     State;
  UINT16    Reserved;
  UINT32    Reserved1;
} VARIABLE_STORE_HEADER;

typedef struct {
  UINT16      StartId;
  UINT8       State;
  UINT8       Reserved;
  UINT32      Attributes;
  UINT32      NameSize;
  UINT32      DataSize;
  EFI_GUID    VendorGuid;
} VARIABLE_HEADER;

#pragma pack()

#endif // _EFI_VARIABLE_H_
