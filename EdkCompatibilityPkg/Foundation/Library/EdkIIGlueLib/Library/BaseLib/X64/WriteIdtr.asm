; Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials                          
; are licensed and made available under the terms and conditions of the BSD License         
; which accompanies this distribution.  The full text of the license may be found at        
; http://opensource.org/licenses/bsd-license.php                                            
;                                                                                           
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
;
; Module Name:
;
;   WriteIdtr.Asm
;
; Abstract:
;
;   AsmWriteIdtr function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86WriteIdtr (
;   IN      CONST IA32_DESCRIPTOR     *Idtr
;   );
;------------------------------------------------------------------------------
InternalX86WriteIdtr  PROC
    lidt    fword ptr [rcx]
    ret
InternalX86WriteIdtr  ENDP

    END
