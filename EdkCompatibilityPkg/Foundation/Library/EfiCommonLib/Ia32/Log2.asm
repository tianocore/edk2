;/*++
;
;Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
;This program and the accompanying materials                          
;are licensed and made available under the terms and conditions of the BSD License         
;which accompanies this distribution.  The full text of the license may be found at        
;http://opensource.org/licenses/bsd-license.php                                            
;                                                                                          
;THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;
;Module Name:
;
;  Log2.c
;
;Abstract:
;
;  64-bit integer logarithm function for IA-32
;
;--*/
;
;---------------------------------------------------------------------------
    .686
    .model  flat,C
    .code

;---------------------------------------------------------------------------

;UINT8
;Log2 (
;  IN UINT64   Operand
;  )
;/*++
;
;Routine Description:
; 
;  Calculates and floors logarithms based on 2
;
;Arguments:
;
;  Operand - value to calculate logarithm
;  
;Returns:
;
;  The largest integer that is less than or equal
;  to the logarithm of Operand based on 2 
;
;--*/
Log2 PROC
  mov    ecx, 64
  
  cmp    dword ptr [esp + 4], 0  ; (UINT32 *(&Operand))
  jne    _Log2_Wend 
  cmp    dword ptr [esp + 8], 0  ; (UINT32 *(&Operand)) + 1
  jne    _Log2_Wend 
  mov    cl, 0FFH
  jmp    _Log2_Done
  
_Log2_Wend:
  dec    ecx
  cmp    ecx, 32
  jae    _Log2_Higher
  bt     [esp + 4], ecx ; (UINT32 *(&Operand))
  jmp    _Log2_Bit
  
_Log2_Higher:
  mov    eax, ecx
  sub    eax, 32
  bt     [esp + 8], eax ; (UINT32 *(&Operand)) + 1
  
_Log2_Bit:
  jc     _Log2_Done
  jmp    _Log2_Wend
      
_Log2_Done:
  mov    al, cl

  ret

	Log2 ENDP
	END
