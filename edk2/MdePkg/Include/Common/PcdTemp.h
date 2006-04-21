/** @file

Copyright (c) 2006, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PCD_TEMP_H__
#define __PCD_TEMP_H__

#define PCD_INVALID_TOKEN     ((UINTN)(-1))

/*
 * The following structure will be removed soon after the real 
 * PCD service PEIM and DXE driver are implmented.
 */

///
/// I have rearranged the data so that the variable length items are at the end of the structure
/// and we have a reasonable change of interpreting the other stuff properly.
///
typedef struct {
  UINTN                 Token;

  //
  // HII Knowledge - non optimized for now
  //
  UINT8                 HiiData;          // If TRUE, use Variable Data
  UINT8                 SKUEnabled;       // If TRUE, there might be various SKU data entries for this Token
  UINT8                 MaxSKUCount;      // Up to 256 entries - limits the search space
  UINT8                 SKUId;            // ID of the SKU

  GUID                  VariableGuid;     // Variable GUID
  UINT32                DatumSize;
  UINT64                Datum;
  CHAR16                *VariableName;    // Null-terminated Variable Name (remember to calculate size)
                                          // We still need Offset information for the variable
                                          // So naturally we can use DatumSize as the Length Field
                                          // And we can overload the use of Datum for the Offset information
  VOID                  *ExtendedData;    // VOID* data of size DatumSize
} EMULATED_PCD_ENTRY;

typedef
VOID
(EFIAPI *PCD_TEMP_CALLBACK) (
  IN  CONST EFI_GUID   *CallBackGuid, OPTIONAL
  IN  UINTN            CallBackToken,
  IN  VOID             *TokenData,
  IN  UINTN            TokenDataSize
  );

///
/// Used by the PCD Database - never contained in an FFS file
///
typedef struct {
  UINTN                 Token;

  //
  // HII Knowledge - non optimized for now
  //
  UINT8                 HiiData;          // If TRUE, use Variable Data
  UINT8                 SKUEnabled;       // If TRUE, there might be various SKU data entries for this Token
  UINT8                 MaxSKUCount;      // Up to 256 entries - limits the search space
  UINT8                 SKUId;            // ID of the SKU

  GUID                  VariableGuid;     // Variable GUID
  UINT32                DatumSize;
  UINT64                Datum;
  CHAR16                *VariableName;    // Null-terminated Variable Name (remember to calculate size)
                                          // We still need Offset information for the variable
                                          // So naturally we can use DatumSize as the Length Field
                                          // And we can overload the use of Datum for the Offset information
  VOID                  *ExtendedData;    // VOID* data of size DatumSize

  PCD_TEMP_CALLBACK     *CallBackList;
  UINT32                CallBackEntries;
  UINT32                CallBackListSize;
} EMULATED_PCD_ENTRY_EX;                  // This exists to facilitate PCD Database implementation only

typedef struct {
  UINTN                   Count;
  EMULATED_PCD_ENTRY_EX   Entry[1];                  // This exists to facilitate PCD Database implementation only
} EMULATED_PCD_DATABASE_EX;

#endif
