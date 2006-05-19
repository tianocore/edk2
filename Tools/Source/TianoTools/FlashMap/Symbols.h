/*++

Copyright (c)  2004 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


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
