/** @file
  Internal structure for Var Check Hii.

Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VAR_CHECK_STRUCTURE_H_
#define _VAR_CHECK_STRUCTURE_H_

//
// Alignment for Hii Variable and Question header.
//
#define HEADER_ALIGNMENT  4
#define HEADER_ALIGN(Header)  (((UINTN) (Header) + HEADER_ALIGNMENT - 1) & (~(HEADER_ALIGNMENT - 1)))

#pragma pack (1)

#define VAR_CHECK_HII_REVISION  0x0002

typedef struct {
  UINT16      Revision;
  UINT16      HeaderLength;
  UINT32      Length;       // Length include this header
  UINT8       OpCode;
  UINT8       Reserved;
  UINT16      Size;
  UINT32      Attributes;
  EFI_GUID    Guid;
  // CHAR16              Name[];
} VAR_CHECK_HII_VARIABLE_HEADER;

typedef struct {
  UINT8      OpCode;
  UINT8      Length;        // Length include this header
  UINT16     VarOffset;
  UINT8      StorageWidth;
  BOOLEAN    BitFieldStore;        // Whether the Question is stored in bit field, if TRUE, the VarOffset/StorageWidth will be saved as bit level, otherwise in byte level.
} VAR_CHECK_HII_QUESTION_HEADER;

typedef struct {
  UINT8      OpCode;
  UINT8      Length;        // Length include this header
  UINT16     VarOffset;
  UINT8      StorageWidth;
  BOOLEAN    BitFieldStore;        // Whether the Question is stored in bit field, if TRUE, the VarOffset/StorageWidth will be saved as bit level, otherwise in byte level.
  // UINTx               Data[]; // x = UINT8/UINT16/UINT32/UINT64;
} VAR_CHECK_HII_QUESTION_ONEOF;

typedef struct {
  UINT8      OpCode;
  UINT8      Length;        // Length include this header
  UINT16     VarOffset;
  UINT8      StorageWidth;
  BOOLEAN    BitFieldStore;        // Whether the Question is stored in bit field, if TRUE, the VarOffset/StorageWidth will be saved as bit level, otherwise in byte level.
} VAR_CHECK_HII_QUESTION_CHECKBOX;

typedef struct {
  UINT8      OpCode;
  UINT8      Length;        // Length include this header
  UINT16     VarOffset;
  UINT8      StorageWidth;
  BOOLEAN    BitFieldStore;        // Whether the Question is stored in bit field, if TRUE, the VarOffset/StorageWidth will be saved as bit level, otherwise in byte level.
  // UINTx               Minimum; // x = UINT8/UINT16/UINT32/UINT64;
  // UINTx               Maximum; // x = UINT8/UINT16/UINT32/UINT64;
} VAR_CHECK_HII_QUESTION_NUMERIC;

typedef struct {
  UINT8      OpCode;
  UINT8      Length;        // Length include this header
  UINT16     VarOffset;
  UINT8      StorageWidth;
  BOOLEAN    BitFieldStore;        // Whether the Question is stored in bit field, if TRUE, the VarOffset/StorageWidth will be saved as bit level, otherwise in byte level.
  UINT8      MaxContainers;
  // UINTx               Data[]; // x = UINT8/UINT16/UINT32/UINT64;
} VAR_CHECK_HII_QUESTION_ORDEREDLIST;

#pragma pack ()

#endif
