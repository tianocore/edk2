  TITLE   Ia32math.asm: Generic math routines for EBC interpreter running on IA32 processor

;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation                                                         
; All rights reserved. This program and the accompanying materials                          
; are licensed and made available under the terms and conditions of the BSD License         
; which accompanies this distribution.  The full text of the license may be found at        
; http://opensource.org/licenses/bsd-license.php                                            
;                                                                                           
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
; 
; Module Name:
;
;   Ia32math.asm
; 
; Abstract:
; 
;   Generic math routines for EBC interpreter running on IA32 processor
;
;------------------------------------------------------------------------------

  .686P
  .XMM
  .MODEL SMALL
  .CODE

LeftShiftU64  PROTO C  Operand:  QWORD, CountIn: QWORD
RightShiftU64 PROTO C  Operand:  QWORD, CountIn: QWORD
ARightShift64 PROTO C  Operand:  QWORD, CountIn: QWORD
MulU64x64     PROTO C  Value1:   QWORD, Value2:  QWORD, ResultHigh: DWORD
MulS64x64     PROTO C  Value1:   QWORD, Value2:  QWORD, ResultHigh: DWORD
DivU64x64     PROTO C  Dividend: QWORD, Divisor: QWORD, Remainder:  DWORD, Error: DWORD
DivS64x64     PROTO C  Dividend: QWORD, Divisor: QWORD, Remainder:  DWORD, Error: DWORD

  
LeftShiftU64 PROC C Operand: QWORD, CountIn: QWORD

;------------------------------------------------------------------------------
; UINT64
; LeftShiftU64 (
;   IN UINT64   Operand,
;   IN UINT64   CountIn
;   )
;
; Routine Description:
;  
;   Left-shift a 64-bit value.
;
; Arguments:
;
;   Operand - the value to shift
;   Count   - shift count
;
; Returns:
;
;   Operand << Count
;------------------------------------------------------------------------------

  push   ecx
  ;
  ; if (CountIn > 63) return 0;
  ;
  cmp    dword ptr CountIn[4], 0
  jne    _LeftShiftU64_Overflow
  mov    ecx, dword ptr CountIn[0]
  cmp    ecx, 63
  jbe    _LeftShiftU64_Calc
  
_LeftShiftU64_Overflow:
  xor    eax, eax
  xor    edx, edx
  jmp    _LeftShiftU64_Done
  
_LeftShiftU64_Calc:
  mov    eax, dword ptr Operand[0]
  mov    edx, dword ptr Operand[4]
  
  shld   edx, eax, cl
  shl    eax, cl
  cmp    ecx, 32
  jc     short _LeftShiftU64_Done
  
  mov    edx, eax
  xor    eax, eax
  
_LeftShiftU64_Done:
  pop    ecx
  ret

LeftShiftU64 ENDP


RightShiftU64 PROC C Operand: QWORD, CountIn: QWORD

;------------------------------------------------------------------------------
; UINT64
; RightShiftU64 (
;   IN UINT64   Operand,
;   IN UINT64   CountIn
;   )
;
; Routine Description:
;  
;   Right-shift an unsigned 64-bit value.
;
; Arguments:
;
;   Operand - the value to shift
;   Count   - shift count
;
; Returns:
;
;   Operand >> Count
;------------------------------------------------------------------------------

  push   ecx
  ;
  ; if (CountIn > 63) return 0;
  ;
  cmp    dword ptr CountIn[4], 0
  jne    _RightShiftU64_Overflow
  mov    ecx, dword ptr CountIn[0]
  cmp    ecx, 63
  jbe    _RightShiftU64_Calc
  
_RightShiftU64_Overflow:
  xor    eax, eax
  xor    edx, edx
  jmp    _RightShiftU64_Done
  
_RightShiftU64_Calc:
  mov    eax, dword ptr Operand[0]
  mov    edx, dword ptr Operand[4]
  
  shrd   eax, edx, cl
  shr    edx, cl
  cmp    ecx, 32
  jc     short _RightShiftU64_Done
  
  mov    eax, edx
  xor    edx, edx
  
_RightShiftU64_Done:
  pop    ecx
  ret

RightShiftU64 ENDP


ARightShift64 PROC C Operand: QWORD, CountIn: QWORD

;------------------------------------------------------------------------------
; INT64
; ARightShift64 (
;   IN INT64  Operand,
;   IN UINT64 CountIn
;   )
;
; Routine Description:
;  
;   Arithmatic shift a 64 bit signed value.
;
; Arguments:
;
;   Operand - the value to shift
;   Count   - shift count
;
; Returns:
;
;  Operand >> Count
;------------------------------------------------------------------------------

  push   ecx
  ;
  ; If they exceeded the max shift count, then return either 0 or all F's
  ; depending on the sign bit.
  ;
  cmp    dword ptr CountIn[4], 0
  jne    _ARightShiftU64_Overflow
  mov    ecx, dword ptr CountIn[0]
  cmp    ecx, 63
  jbe    _ARightShiftU64_Calc
  
_ARightShiftU64_Overflow:
  ;
  ; Check the sign bit of Operand
  ;
  bt     dword ptr Operand[4], 31
  jnc    _ARightShiftU64_Return_Zero
  ;
  ; return -1
  ;
  or     eax, 0FFFFFFFFh
  or     edx, 0FFFFFFFFh
  jmp    _ARightShiftU64_Done

_ARightShiftU64_Return_Zero:
  xor    eax, eax
  xor    edx, edx
  jmp    _ARightShiftU64_Done
  
_ARightShiftU64_Calc:
  mov    eax, dword ptr Operand[0]
  mov    edx, dword ptr Operand[4]

  shrd   eax, edx, cl
  sar    edx, cl
  cmp    ecx, 32
  jc     short _ARightShiftU64_Done

  ;
  ; if ecx >= 32, then eax = edx, and edx = sign bit
  ;
  mov    eax, edx
  sar    edx, 31

_ARightShiftU64_Done:
  pop    ecx
  ret

ARightShift64 ENDP


MulU64x64 PROC C  Value1: QWORD, Value2: QWORD, ResultHigh: DWORD

;------------------------------------------------------------------------------
; UINT64 
; MulU64x64 (
;   UINT64 Value1, 
;   UINT64 Value2, 
;   UINT64 *ResultHigh
;   )
;
; Routine Description:
; 
;   Multiply two unsigned 64-bit values.
;
; Arguments:
;
;   Value1      - first value to multiply
;   Value2      - value to multiply by Value1
;   ResultHigh  - result to flag overflows
;
; Returns:
; 
;   Value1 * Value2
;   The 128-bit result is the concatenation of *ResultHigh and the return value 
;------------------------------------------------------------------------------

  push   ebx
  push   ecx
  mov    ebx, ResultHigh           ; ebx points to the high 4 words of result
  ;
  ; The result consists of four double-words.
  ; Here we assume their names from low to high: dw0, dw1, dw2, dw3
  ;
  mov    eax, dword ptr Value1[0]
  mul    dword ptr Value2[0]
  push   eax                       ; eax contains final result of dw0, push it
  mov    ecx, edx                  ; ecx contains partial result of dw1
  
  mov    eax, dword ptr Value1[4]
  mul    dword ptr Value2[0]
  add    ecx, eax                  ; add eax to partial result of dw1
  adc    edx, 0    
  mov    dword ptr [ebx], edx      ; lower double-word of ResultHigh contains partial result of dw2
    
  mov    eax, dword ptr Value1[0]
  mul    dword ptr Value2[4]
  add    ecx, eax                  ; add eax to partial result of dw1
  push   ecx                      ; ecx contains final result of dw1, push it
  adc    edx, 0
  mov    ecx, edx                  ; ecx contains partial result of dw2, together with ResultHigh

  mov    eax, dword ptr Value1[4]
  mul    dword ptr Value2[4]
  add    ecx, eax                  ; add eax to partial result of dw2
  adc    edx, 0
  add    dword ptr [ebx], ecx      ; lower double-word of ResultHigh contains final result of dw2
  adc    edx, 0
  mov    dword ptr [ebx + 4], edx  ; high double-word of ResultHigh contains final result of dw3
    
  pop    edx                       ; edx contains the final result of dw1
  pop    eax                       ; edx contains the final result of dw0
  pop    ecx
  pop    ebx
  ret

MulU64x64 ENDP


MulS64x64  PROC C  Value1: QWORD, Value2: QWORD, ResultHigh: DWORD

;------------------------------------------------------------------------------
; INT64
; MulS64x64 (
;   INT64 Value1,
;   INT64 Value2,
;   INT64 *ResultHigh
;   )
;
; Routine Description:
;  
;   Multiply two signed 64-bit values.
;
; Arguments:
;
;   Value1      - first value to multiply
;   Value2      - value to multiply by Value1
;   ResultHigh  - result to flag overflows
;
; Returns:
;
;   Value1 * Value2
;   The 128-bit result is the concatenation of *ResultHigh and the return value 
;------------------------------------------------------------------------------

  push   ebx
  push   ecx
  mov    ebx, ResultHigh           ; ebx points to the high 4 words of result
  xor    ecx, ecx                 ; the lowest bit of ecx flags the sign
    
  mov    edx, dword ptr Value1[4]
  bt     edx, 31
  jnc    short _MulS64x64_A_Positive
  ;
  ; a is negative
  ;
  mov    eax, dword ptr Value1[0]
  not    edx
  not    eax
  add    eax, 1
  adc    edx, 0
  mov    dword ptr Value1[0], eax
  mov    dword ptr Value1[4], edx
  btc    ecx, 0
    
_MulS64x64_A_Positive:
  mov    edx, dword ptr Value2[4]
  bt     edx, 31
  jnc    short _MulS64x64_B_Positive
  ;
  ; b is negative
  ;
  mov    eax, dword ptr Value2[0]
  not    edx
  not    eax
  add    eax, 1
  adc    edx, 0
  mov    dword ptr Value2[0], eax
  mov    dword ptr Value2[4], edx
  btc    ecx, 0
    
_MulS64x64_B_Positive:
  invoke MulU64x64, Value1, Value2, ResultHigh
  bt     ecx, 0
  jnc    short _MulS64x64_Done
  ;
  ;negate the result
  ;
  not    eax
  not    edx
  not    dword ptr [ebx]
  not    dword ptr [ebx + 4]
  add    eax, 1
  adc    edx, 0
  adc    dword ptr [ebx], 0
  adc    dword ptr [ebx + 4], 0
    
_MulS64x64_Done:
  pop    ecx
  pop    ebx
  ret

MulS64x64 ENDP


DivU64x64 PROC C  Dividend: QWORD, Divisor: QWORD, Remainder: DWORD, Error: DWORD, 

;------------------------------------------------------------------------------
; UINT64
; DivU64x64 (
;   IN  UINT64   Dividend,
;   IN  UINT64   Divisor,
;   OUT UINT64   *Remainder OPTIONAL,
;   OUT UINT32   *Error
;   )
;
; Routine Description:
; 
;   This routine allows a 64 bit value to be divided with a 64 bit value returns 
;   64bit result and the Remainder
;
; Arguments:
;
;   Dividend    - dividend
;   Divisor     - divisor
;   ResultHigh  - result to flag overflows
;   Error       - flag for error
;
; Returns:
; 
;   Dividend / Divisor
;   Remainder = Dividend mod Divisor
;------------------------------------------------------------------------------

  push   ecx
  
  mov    eax, Error
  mov    dword ptr [eax], 0 
  
  cmp    dword ptr Divisor[0], 0
  jne    _DivU64x64_Valid
  cmp    dword ptr Divisor[4], 0
  jne    _DivU64x64_Valid
  ;
  ; the divisor is zero
  ;
  mov    dword ptr [eax], 1 
  cmp    Remainder, 0
  je     _DivU64x64_Invalid_Return
  ;
  ; fill the remainder if the pointer is not null
  ;
  mov    eax, Remainder
  mov    dword ptr [eax], 0
  mov    dword ptr [eax + 4], 80000000h
  
_DivU64x64_Invalid_Return:
  xor    eax, eax
  mov    edx, 80000000h
  jmp    _DivU64x64_Done

_DivU64x64_Valid:
  ;
  ; let edx and eax contain the intermediate result of remainder
  ;
  xor    edx, edx
  xor    eax, eax
  mov    ecx, 64
  
_DivU64x64_Wend:
  ;
  ; shift dividend left one
  ;
  shl    dword ptr Dividend[0], 1
  rcl    dword ptr Dividend[4], 1    
  ;
  ; rotate intermediate result of remainder left one
  ;
  rcl    eax, 1
  rcl    edx, 1 
  
  cmp    edx, dword ptr Divisor[4]
  ja     _DivU64x64_Sub_Divisor
  jb     _DivU64x64_Cont
  cmp    eax, dword ptr Divisor[0]
  jb     _DivU64x64_Cont
  
_DivU64x64_Sub_Divisor:
  ;
  ; If intermediate result of remainder is larger than
  ; or equal to divisor, then set the lowest bit of dividend,
  ; and subtract divisor from intermediate remainder
  ;
  bts    dword ptr Dividend[0], 0
  sub    eax, dword ptr Divisor[0]
  sbb    edx, dword ptr Divisor[4]
  
_DivU64x64_Cont:
  loop   _DivU64x64_Wend
  
  cmp    Remainder, 0
  je     _DivU64x64_Assign
  mov    ecx, Remainder
  mov    dword ptr [ecx], eax
  mov    dword ptr [ecx + 4], edx
 
_DivU64x64_Assign:
  mov    eax, dword ptr Dividend[0]
  mov    edx, dword ptr Dividend[4]
  
_DivU64x64_Done:
  pop    ecx
  ret
  
DivU64x64 ENDP

DivS64x64 PROC C  Dividend: QWORD, Divisor: QWORD, Remainder: DWORD, Error: DWORD, 

;------------------------------------------------------------------------------
; INT64
; DivU64x64 (
;   IN  INT64   Dividend,
;   IN  INT64   Divisor,
;   OUT UINT64   *Remainder OPTIONAL,
;   OUT UINT32   *Error
;   )
;
; Routine Description:
; 
;   This routine allows a 64 bit signed value to be divided with a 64 bit 
;   signed value returns 64bit result and the Remainder.
;
; Arguments:
;
;   Dividend    - dividend
;   Divisor     - divisor
;   ResultHigh  - result to flag overflows
;   Error       - flag for error
;
; Returns:
; 
;   Dividend / Divisor
;   Remainder = Dividend mod Divisor
;------------------------------------------------------------------------------

  push   ecx
  
  mov    eax, Error
  mov    dword ptr [eax], 0 
  
  cmp    dword ptr Divisor[0], 0
  jne    _DivS64x64_Valid
  cmp    dword ptr Divisor[4], 0
  jne    _DivS64x64_Valid
  ;
  ; the divisor is zero
  ;
  mov    dword ptr [eax], 1 
  cmp    Remainder, 0
  je     _DivS64x64_Invalid_Return
  ;
  ; fill the remainder if the pointer is not null
  ;
  mov    eax, Remainder
  mov    dword ptr [eax], 0
  mov    dword ptr [eax + 4], 80000000h
  
_DivS64x64_Invalid_Return:
  xor    eax, eax
  mov    edx, 80000000h
  jmp    _DivS64x64_Done

_DivS64x64_Valid:
  ;
  ; The lowest bit of ecx flags the sign of quotient,
  ; The seconde lowest bit flags the sign of remainder
  ;
  xor    ecx, ecx                 
  
  mov    edx, dword ptr Dividend[4]
  bt     edx, 31
  jnc    short _DivS64x64_Dividend_Positive
  ;
  ; dividend is negative
  ;
  mov    eax, dword ptr Dividend[0]
  not    edx
  not    eax
  add    eax, 1
  adc    edx, 0
  mov    dword ptr Dividend[0], eax
  mov    dword ptr Dividend[4], edx
  ;
  ; set both the flags for signs of quotient and remainder
  ;
  btc    ecx, 0
  btc    ecx, 1
    
_DivS64x64_Dividend_Positive:
  mov    edx, dword ptr Divisor[4]
  bt     edx, 31
  jnc    short _DivS64x64_Divisor_Positive
  ;
  ; divisor is negative
  ;
  mov    eax, dword ptr Divisor[0]
  not    edx
  not    eax
  add    eax, 1
  adc    edx, 0
  mov    dword ptr Divisor[0], eax
  mov    dword ptr Divisor[4], edx
  ;
  ; just complement the flag for sign of quotient
  ;
  btc    ecx, 0
    
_DivS64x64_Divisor_Positive:
  invoke DivU64x64, Dividend, Divisor, Remainder, Error
  bt     ecx, 0
  jnc    short _DivS64x64_Remainder
  ;
  ; negate the quotient
  ;
  not    eax
  not    edx
  add    eax, 1
  adc    edx, 0
    
_DivS64x64_Remainder:
  bt     ecx, 1
  jnc    short _DivS64x64_Done
  ;
  ; negate the remainder
  ;
  mov    ecx, remainder
  not    dword ptr [ecx]
  not    dword ptr [ecx + 4]
  add    dword ptr [ecx], 1
  adc    dword ptr [ecx + 4], 0
  
_DivS64x64_Done:
  pop    ecx
  ret

DivS64x64 ENDP

END