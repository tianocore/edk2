/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  StringParse.h

Abstract:

  Prototypes and defines for the string pack parsing services.

--*/

#ifndef _STRING_PARSE_H_
#define _STRING_PARSE_H_

STATUS
StringGetPack (
  int                 Handle,           // matches handle passed in with StringParsePack()
  EFI_HII_STRING_PACK **StringPack,     // returned pointer to string pack
  int                 *StringPackSize,  // sizeof buffer pointed to by StringPack
  int                 *NumStringPacks,  // in the array pointed to by StringPack
  EFI_GUID            *FormsetGuid,
  EFI_GUID            *PackageGuid
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Handle          - GC_TODO: add argument description
  StringPack      - GC_TODO: add argument description
  StringPackSize  - GC_TODO: add argument description
  NumStringPacks  - GC_TODO: add argument description
  FormsetGuid     - GC_TODO: add argument description
  PackageGuid     - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
StringParsePack (
  int                   Handle,
  EFI_HII_STRING_PACK   *StringPack,
  EFI_GUID              *FormsetGuid,
  EFI_GUID              *PackageGuid
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Handle      - GC_TODO: add argument description
  StringPack  - GC_TODO: add argument description
  FormsetGuid - GC_TODO: add argument description
  PackageGuid - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
StringInit (
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
StringEnd (
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

#endif // #ifndef _STRING_PARSE_H_
