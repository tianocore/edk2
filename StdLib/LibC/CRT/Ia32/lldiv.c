/** @file
  64-bit Math Worker Function.
  The 32-bit versions of C compiler generate calls to library routines
  to handle 64-bit math. These functions use non-standard calling conventions.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>


/*
 * Divides a 64-bit signed value with a 64-bit signed value and returns
 * a 64-bit signed result.
 */
__declspec(naked) void __cdecl _alldiv (void)
{
  //
  // Wrapper Implementation over EDKII DivS64x64Remainder() routine
  //    INT64
  //    EFIAPI
  //    DivS64x64Remainder (
  //      IN      UINT64     Dividend,
  //      IN      UINT64     Divisor,
  //      OUT     UINT64     *Remainder  OPTIONAL
  //      )
  //
  _asm {

    ;Entry:
    ;       Arguments are passed on the stack:
    ;               1st pushed: divisor (QWORD)
    ;               2nd pushed: dividend (QWORD)
    ;
    ;Exit:
    ;       EDX:EAX contains the quotient (dividend/divisor)
    ;       NOTE: this routine removes the parameters from the stack.
    ;
    ; Original local stack when calling _alldiv
    ;               -----------------
    ;               |               |
    ;               |---------------|
    ;               |               |
    ;               |--  Divisor  --|
    ;               |               |
    ;               |---------------|
    ;               |               |
    ;               |--  Dividend --|
    ;               |               |
    ;               |---------------|
    ;               |  ReturnAddr** |
    ;       ESP---->|---------------|
    ;

    ;
    ; Set up the local stack for NULL Reminder pointer
    ;
    xor  eax, eax
    push eax

    ;
    ; Set up the local stack for Divisor parameter
    ;
    mov  eax, [esp + 20]
    push eax
    mov  eax, [esp + 20]
    push eax

    ;
    ; Set up the local stack for Dividend parameter
    ;
    mov  eax, [esp + 20]
    push eax
    mov  eax, [esp + 20]
    push eax

    ;
    ; Call native DivS64x64Remainder of BaseLib
    ;
    call DivS64x64Remainder

    ;
    ; Adjust stack
    ;
    add  esp, 20

    ret  16
  }
}
