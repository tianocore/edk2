/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IfrParse.h

Abstract:

  Prototypes and defines for the IFR parsing services.

--*/

#ifndef _IFR_PARSE_H_
#define _IFR_PARSE_H_

#define DEFAULT_HII_PACK_FILENAME_EXTENSION ".hpk"
//
// When we parse IFR, we'll keep the IFR in a linked list of
// these.
//
typedef struct _IFR_PARSE_ENTRY {
  struct _IFR_PARSE_ENTRY *Next;
  int                     Tag;  // for debugging
  EFI_IFR_OP_HEADER       *RawIfrHeader;
  //
  // GUIDs for variable storage
  //
  EFI_GUID                *VarStoreGuid1;
  char                    *VarStoreName1;
  EFI_GUID                *VarStoreGuid2;
  char                    *VarStoreName2;
} IFR_PARSE_ENTRY;

typedef struct _IFR_PARSE_CONTEXT {
  struct _IFR_PARSE_CONTEXT *Next;
  EFI_HII_IFR_PACK          *PackHeader;
  char                      *IfrBufferStart;
  char                      *CurrentPos;
  long                      IfrBufferLen;
  int                       Handle;
  IFR_PARSE_ENTRY           *Ifr;
  IFR_PARSE_ENTRY           *LastIfr;
  IFR_PARSE_ENTRY           *CurrentIfr;
  FILE                      *OutFptr;
  CHAR16                    *Language;
  EFI_GUID                  *FormsetGuid;
  EFI_GUID                  NullGuid;     // for use until we set the Guid field correctly
  EFI_GUID                  PackageGuid;  // from the PackageGuid in the HII data table
} IFR_PARSE_CONTEXT;

STATUS
IfrGetVarPack (
  int                   VarIndex,
  EFI_HII_VARIABLE_PACK **VarPack
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  VarIndex  - GC_TODO: add argument description
  VarPack   - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
IfrParsePack (
  int              Handle,
  EFI_HII_IFR_PACK *PackHeader,
  EFI_GUID         *PackageGuid
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Handle      - GC_TODO: add argument description
  PackHeader  - GC_TODO: add argument description
  PackageGuid - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
IfrParseCheck (
  char *Buffer,
  long BufferSize
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Buffer      - GC_TODO: add argument description
  BufferSize  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
IfrParseInit (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

STATUS
IfrParseEnd (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

STATUS
IfrParseDump (
  int    Handle,
  CHAR16 *Language,
  FILE   *OutFptr
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Handle    - GC_TODO: add argument description
  Language  - GC_TODO: add argument description
  OutFptr   - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
IfrSetDefaults (
  int MfgDefaults
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  MfgDefaults - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
IfrGetIfrPack (
  int              Handle,
  EFI_HII_IFR_PACK **PackHeader,
  EFI_GUID         *FormsetGuid
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Handle      - GC_TODO: add argument description
  PackHeader  - GC_TODO: add argument description
  FormsetGuid - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
IfrReferencesVarPack (
  int                   IfrHandle,
  EFI_HII_VARIABLE_PACK *VarPack
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IfrHandle - GC_TODO: add argument description
  VarPack   - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif // #ifndef _IFR_PARSE_H_
