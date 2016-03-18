#/** @file
#  
#    Low level IA32 specific EBC support routines.
#  
#  Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#  
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#  
#**/

ASM_GLOBAL ASM_PFX(CopyMem)
ASM_GLOBAL ASM_PFX(EbcInterpret)
ASM_GLOBAL ASM_PFX(ExecuteEbcImageEntryPoint)

ASM_GLOBAL ASM_PFX(EbcLLCALLEXNative)
ASM_PFX(EbcLLCALLEXNative):
         push   %ebp
         push   %ebx
         mov    %esp,%ebp
         mov    0xc(%esp),%ecx
         mov    0x14(%esp),%eax
         mov    0x10(%esp),%edx
         sub    %edx,%eax
         sub    %eax,%esp
         mov    %esp,%ebx
         push   %ecx
         push   %eax
         push   %edx
         push   %ebx
         call   ASM_PFX(CopyMem)
         pop    %eax
         pop    %eax
         pop    %eax
         pop    %ecx
         call   *%ecx
         mov    %ebp,%esp
         mov    %ebp,%esp
         pop    %ebx
         pop    %ebp
         ret

ASM_GLOBAL ASM_PFX(EbcLLEbcInterpret)
ASM_PFX(EbcLLEbcInterpret):
    # Construct new stack
    push %ebp
    mov  %esp, %ebp
    push %esi
    push %edi
    sub  $0x40, %esp
    push %eax
    mov  %ebp, %esi
    add  $0x8, %esi
    mov  %esp, %edi
    add  $0x4, %edi
    mov  $0x10, %ecx
    rep  movsd
    
    # call C-code
    call ASM_PFX(EbcInterpret)
    add  $0x44, %esp
    pop  %edi
    pop  %esi
    pop  %ebp
    ret

ASM_GLOBAL ASM_PFX(EbcLLExecuteEbcImageEntryPoint)
ASM_PFX(EbcLLExecuteEbcImageEntryPoint):
    # Construct new stack
    mov  %eax, -0xC(%esp)
    mov  0x4(%esp), %eax
    mov  %eax, -0x8(%esp)
    mov  0x8(%esp), %eax
    mov  %eax, -0x4(%esp)
    # call C-code
    sub  $0xC, %esp
    call ASM_PFX(ExecuteEbcImageEntryPoint)
    add  $0xC, %esp
    ret
