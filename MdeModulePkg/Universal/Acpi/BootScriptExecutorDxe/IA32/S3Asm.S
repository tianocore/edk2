## @file
#
# Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are
# licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

#-----------------------------------------
#VOID
#AsmTransferControl (
#  IN   UINT32           S3WakingVector,
#  IN   UINT32           AcpiLowMemoryBase
#  );
#-----------------------------------------

ASM_GLOBAL ASM_PFX(AsmTransferControl)
ASM_PFX(AsmTransferControl):
    # S3WakingVector    :DWORD
    # AcpiLowMemoryBase :DWORD
    pushl %ebp
    movl  %esp,%ebp
    leal  LABLE, %eax
    pushl $0x28             # CS
    pushl %eax
    movl  8(%ebp),%ecx
    shrdl $20,%ecx,%ebx
    andl  $0xf,%ecx
    movw  %cx,%bx
    movl  %ebx, jmp_addr
    lret
LABLE: 
    .byte 0xb8,0x30,0       # mov ax, 30h as selector
    movw  %ax,%ds
    movw  %ax,%es
    movw  %ax,%fs
    movw  %ax,%gs
    movw  %ax,%ss
    movl  %cr0, %eax        # Get control register 0  
    .byte 0x66
    .byte 0x83,0xe0,0xfe    # and    eax, 0fffffffeh  ; Clear PE bit (bit #0)
    .byte 0xf,0x22,0xc0     # mov    cr0, eax         ; Activate real mode
    .byte 0xea              # jmp far @jmp_addr
jmp_addr: 
    .long    0

ASM_GLOBAL ASM_PFX(AsmTransferControl32)
ASM_PFX(AsmTransferControl32):
    jmp ASM_PFX(AsmTransferControl)

# dummy
ASM_GLOBAL ASM_PFX(AsmTransferControl16)
ASM_PFX(AsmTransferControl16):
ASM_GLOBAL ASM_PFX(AsmFixAddress16)
ASM_PFX(AsmFixAddress16):
    .long    0
ASM_GLOBAL ASM_PFX(AsmJmpAddr32)
ASM_PFX(AsmJmpAddr32):
    .long    0

