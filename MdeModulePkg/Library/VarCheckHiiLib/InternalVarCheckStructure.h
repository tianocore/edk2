/** @file
  Internal structure for Var Check Hii.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VAR_CHECK_STRUCTURE_H_
#define _VAR_CHECK_STRUCTURE_H_

//
// Alignment for Hii Variable and Question header.
//
#define HEADER_ALIGNMENT  4
#define HEADER_ALIGN(Header)  (((UINTN) (Header) + HEADER_ALIGNMENT - 1) & (~(HEADER_ALIGNMENT - 1)))

#pragma pack (1)

#define VAR_CHECK_HII_REVISION  0x0001

typedef struct {
  UINT16            Revision;
  UINT16            HeaderLength;
  UINT32            Length; // Length include this header
  UINT8             OpCode;
  UINT8             Reserved;
  UINT16            Size;
  UINT32            Attributes;
  EFI_GUID          Guid;
//CHAR16              Name[];
} VAR_CHECK_HII_VARIABLE_HEADER;

typedef struct {
  UINT8             OpCode;
  UINT8             Length; // Length include this header
  UINT16            VarOffset;
  UINT8             StorageWidth;
} VAR_CHECK_HII_QUESTION_HEADER;

typedef struct {
  UINT8             OpCode;
  UINT8             Length; // Length include this header
  UINT16            VarOffset;
  UINT8             StorageWidth;
//UINTx               Data[]; // x = UINT8/UINT16/UINT32/UINT64;
} VAR_CHECK_HII_QUESTION_ONEOF;

typedef struct {
  UINT8             OpCode;
  UINT8             Length; // Length include this header
  UINT16            VarOffset;
  UINT8             StorageWidth;
} VAR_CHECK_HII_QUESTION_CHECKBOX;

typedef struct {
  UINT8             OpCode;
  UINT8             Length; // Length include this header
  UINT16            VarOffset;
  UINT8             StorageWidth;
//UINTx               Minimum; // x = UINT8/UINT16/UINT32/UINT64;
//UINTx               Maximum; // x = UINT8/UINT16/UINT32/UINT64;
} VAR_CHECK_HII_QUESTION_NUMERIC;

typedef struct {
  UINT8             OpCode;
  UINT8             Length; // Length include this header
  UINT16            VarOffset;
  UINT8             StorageWidth;
  UINT8             MaxContainers;
//UINTx               Data[]; // x = UINT8/UINT16/UINT32/UINT64;
} VAR_CHECK_HII_QUESTION_ORDEREDLIST;

#pragma pack ()

#endif
