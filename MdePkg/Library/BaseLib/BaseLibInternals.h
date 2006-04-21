/** @file
  Declaration of internal functions in BaseLib.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  BaseLibInternals.h

**/

#ifndef __BASE_LIB_INTERNALS__
#define __BASE_LIB_INTERNALS__

//
// Math functions
//

UINT64
EFIAPI
InternalMathLShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

UINT64
EFIAPI
InternalMathRShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

UINT64
EFIAPI
InternalMathARShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

UINT64
EFIAPI
InternalMathLRotU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

UINT64
EFIAPI
InternalMathRRotU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

UINT64
EFIAPI
InternalMathSwapBytes64 (
  IN      UINT64                    Operand
  );

UINT64
EFIAPI
InternalMathMultU64x32 (
  IN      UINT64                    Multiplicand,
  IN      UINT32                    Multiplier
  );

UINT64
EFIAPI
InternalMathMultU64x64 (
  IN      UINT64                    Multiplicand,
  IN      UINT64                    Multiplier
  );

UINT64
EFIAPI
InternalMathDivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  );

UINT32
EFIAPI
InternalMathModU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  );

UINT64
EFIAPI
InternalMathDivRemU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor,
  OUT     UINT32                    *Remainder
  );

UINT64
EFIAPI
InternalMathDivRemU64x64 (
  IN      UINT64                    Dividend,
  IN      UINT64                    Divisor,
  OUT     UINT64                    *Remainder
  );

INT64
EFIAPI
InternalMathDivRemS64x64 (
  IN      INT64                     Dividend,
  IN      INT64                     Divisor,
  OUT     INT64                     *Remainder
  );

//
// Ia32 and x64 specific functions
//

VOID
EFIAPI
InternalX86ReadGdtr (
  OUT     IA32_DESCRIPTOR           *Gdtr
  );

VOID
EFIAPI
InternalX86WriteGdtr (
  IN      CONST IA32_DESCRIPTOR     *Gdtr
  );

VOID
EFIAPI
InternalX86ReadIdtr (
  OUT     IA32_DESCRIPTOR           *Idtr
  );

VOID
EFIAPI
InternalX86WriteIdtr (
  IN      CONST IA32_DESCRIPTOR     *Idtr
  );

VOID
EFIAPI
InternalX86FxSave (
  OUT     IA32_FX_BUFFER            *Buffer
  );

VOID
EFIAPI
InternalX86FxRestore (
  IN      CONST IA32_FX_BUFFER      *Buffer
  );

VOID
EFIAPI
InternalX86EnablePaging32 (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack
  );

VOID
EFIAPI
InternalX86DisablePaging32 (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack
  );

VOID
EFIAPI
InternalX86EnablePaging64 (
  IN      UINT16                    Cs,
  IN      UINT64                    EntryPoint,
  IN      UINT64                    Context1,  OPTIONAL
  IN      UINT64                    Context2,  OPTIONAL
  IN      UINT64                    NewStack
  );

VOID
EFIAPI
InternalX86DisablePaging64 (
  IN      UINT16                    Cs,
  IN      UINT32                    EntryPoint,
  IN      UINT32                    Context1,  OPTIONAL
  IN      UINT32                    Context2,  OPTIONAL
  IN      UINT32                    NewStack
  );

#endif
