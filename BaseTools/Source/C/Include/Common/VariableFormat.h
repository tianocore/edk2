/**@file
  Header file for EFI Variable Services.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

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
