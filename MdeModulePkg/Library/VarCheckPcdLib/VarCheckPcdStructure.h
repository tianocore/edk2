/** @file
  Internal structure for Var Check Pcd.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VAR_CHECK_STRUCTURE_H_
#define _VAR_CHECK_STRUCTURE_H_

//
// Alignment for PCD Variable and check data header.
//
#define HEADER_ALIGNMENT  4
#define HEADER_ALIGN(Header)  (((UINTN) (Header) + HEADER_ALIGNMENT - 1) & (~(HEADER_ALIGNMENT - 1)))

#pragma pack (1)

#define VAR_CHECK_PCD_REVISION  0x0001

typedef enum {
  VarCheckPcdVariableHeader,
  VarCheckPcdValidList,
  VarCheckPcdValidRange,
  VarCheckPcdCheckTypeMax,
} VAR_CHECK_PCD_CHECK_TYPE;

typedef struct {
  UINT16      Revision;
  UINT16      HeaderLength;
  UINT32      Length;       // Length include this header
  UINT8       Type;
  UINT8       Reserved[3];
  UINT32      Attributes;
  EFI_GUID    Guid;
  // CHAR16              Name[];
} VAR_CHECK_PCD_VARIABLE_HEADER;

typedef struct {
  UINT8     Type;
  UINT8     Length;         // Length include this header
  UINT16    VarOffset;
  UINT8     StorageWidth;
} VAR_CHECK_PCD_VALID_DATA_HEADER;

typedef struct {
  UINT8     Type;
  UINT8     Length;         // Length include this header
  UINT16    VarOffset;
  UINT8     StorageWidth;
  // UINTx               Data[]; // x = UINT8/UINT16/UINT32/UINT64;
} VAR_CHECK_PCD_VALID_LIST;

// typedef struct {
//  UINTx             Minimum; // x = UINT8/UINT16/UINT32/UINT64
//  UINTx             Maximum; // x = UINT8/UINT16/UINT32/UINT64
// } VAR_CHECK_PCD_VALID_RANGE_DATA;

typedef struct {
  UINT8     Type;
  UINT8     Length;         // Length include this header
  UINT16    VarOffset;
  UINT8     StorageWidth;
  //  VAR_CHECK_PCD_VALID_RANGE_DATA    ValidRange[];
} VAR_CHECK_PCD_VALID_RANGE;

#pragma pack ()

#endif
