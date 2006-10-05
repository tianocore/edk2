/*++

Copyright (c)  2004-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Symbols.h

Abstract:

  Defines and prototypes for a class-like symbol table service.

--*/

#ifndef _SYMBOLS_H_
#define _SYMBOLS_H_

#ifdef __cplusplus
extern "C"
{
#endif

int
SymbolAdd (
  char    *Name,
  char    *Value,
  int     Mode
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Name  - GC_TODO: add argument description
  Value - GC_TODO: add argument description
  Mode  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
SymbolsFileStringsReplace (
  char    *InFileName,
  char    *OutFileName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  InFileName  - GC_TODO: add argument description
  OutFileName - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

void
SymbolsConstructor (
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

void
SymbolsDestructor (
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

#ifdef __cplusplus
}
#endif

#endif // #ifndef _SYMBOLS_H_
