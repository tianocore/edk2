#/** @file
#  
#    Low level IA32 specific EBC support routines.
#  
#  Copyright (c) 2007 - 2008, Intel Corporation. <BR>
#  All rights reserved. This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#  
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#  
#**/

.globl ASM_PFX(CopyMem)

.globl ASM_PFX(EbcLLCALLEXNative)
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

.globl ASM_PFX(EbcLLGetEbcEntryPoint)
ASM_PFX(EbcLLGetEbcEntryPoint):
        ret

.globl ASM_PFX(EbcLLGetStackPointer)
ASM_PFX(EbcLLGetStackPointer):
        mov    %esp,%eax
        add    $0x4,%eax
        ret

.globl ASM_PFX(EbcLLGetReturnValue)
ASM_PFX(EbcLLGetReturnValue):
        ret
