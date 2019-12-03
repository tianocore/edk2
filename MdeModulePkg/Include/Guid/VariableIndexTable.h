/** @file
  The variable data structures are related to EDK II-specific implementation of UEFI variables.
  VariableFormat.h defines variable data headers and variable storage region headers.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VARIABLE_INDEX_TABLE_H__
#define __VARIABLE_INDEX_TABLE_H__

typedef struct {
  VARIABLE_HEADER *CurrPtr;
  VARIABLE_HEADER *EndPtr;
  VARIABLE_HEADER *StartPtr;
} VARIABLE_POINTER_TRACK;

#define VARIABLE_INDEX_TABLE_VOLUME 122

#define EFI_VARIABLE_INDEX_TABLE_GUID \
  { 0x8cfdb8c8, 0xd6b2, 0x40f3, { 0x8e, 0x97, 0x02, 0x30, 0x7c, 0xc9, 0x8b, 0x7c } }

extern EFI_GUID gEfiVariableIndexTableGuid;

///
/// Use this data structure to store variable-related info, which can decrease
/// the cost of access to NV.
///
typedef struct {
  UINT16          Length;
  UINT16          GoneThrough;
  VARIABLE_HEADER *EndPtr;
  VARIABLE_HEADER *StartPtr;
  ///
  /// This field is used to store the distance of two neighbouring VAR_ADDED type variables.
  /// The meaning of the field is implement-dependent.
  UINT16          Index[VARIABLE_INDEX_TABLE_VOLUME];
} VARIABLE_INDEX_TABLE;

#endif // __VARIABLE_INDEX_TABLE_H__
