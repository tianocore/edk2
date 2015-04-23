;; @file
;  Provide FSP helper function.
;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;;

    .586p
    .model  flat,C
    .code

FspInfoHeaderRelativeOff    PROC      NEAR    PRIVATE
   ;
   ; This value will be pached by the build script
   ;
   DD    012345678h
FspInfoHeaderRelativeOff    ENDP

GetFspBaseAddress           PROC      NEAR    PUBLIC
   mov   eax, GetFspBaseAddress
   sub   eax, dword ptr [FspInfoHeaderRelativeOff]
   add   eax, 01Ch
   mov   eax, dword ptr [eax]
   ret
GetFspBaseAddress           ENDP

     END