;------------------------------------------------------------------------------
;*
;* Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
;* SPDX-License-Identifier: BSD-2-Clause-Patent
;*
;*
;------------------------------------------------------------------------------

DEFAULT REL
SECTION .text

global ASM_PFX(TestMmioWrite_C6C7_1)
ASM_PFX(TestMmioWrite_C6C7_1):
  mov   rax, 0xc0000000
  mov   BYTE [rax], 0
  ret

global ASM_PFX(TestMmioWrite_C6C7_2)
ASM_PFX(TestMmioWrite_C6C7_2):
  mov   rax, 0xc0000000
  mov   WORD [rax], 0
  ret

global ASM_PFX(TestMmioWrite_C6C7_4)
ASM_PFX(TestMmioWrite_C6C7_4):
  mov   rax, 0xc0000000
  mov   DWORD [rax], 0
  ret

global ASM_PFX(TestMmioWrite_C6C7_8)
ASM_PFX(TestMmioWrite_C6C7_8):
  mov   rax, 0xc0000000
  mov   QWORD [rax], 0
  ret

global ASM_PFX(TestMmioWrite_8889_1)
ASM_PFX(TestMmioWrite_8889_1):
  mov   rax, 0xc0000000
  mov   rbx, 0
  mov   BYTE [rax], bl
  ret

global ASM_PFX(TestMmioWrite_8889_2)
ASM_PFX(TestMmioWrite_8889_2):
  mov   rax, 0xc0000000
  mov   WORD [rax], bx
  ret

global ASM_PFX(TestMmioWrite_8889_4)
ASM_PFX(TestMmioWrite_8889_4):
  mov   rax, 0xc0000000
  mov   DWORD [rax], ebx
  ret

global ASM_PFX(TestMmioWrite_8889_8)
ASM_PFX(TestMmioWrite_8889_8):
  mov   rax, 0xc0000000
  mov   QWORD [rax], rbx
  ret

global ASM_PFX(TestMmioRead_8A8B_1)
ASM_PFX(TestMmioRead_8A8B_1):
  mov   rbx, 0xc0000000
  mov   al, byte [rbx]
  ret

global ASM_PFX(TestMmioRead_8A8B_2)
ASM_PFX(TestMmioRead_8A8B_2):
  mov   rbx, 0xc0000000
  mov   ax, word [rbx]
  ret

global ASM_PFX(TestMmioRead_8A8B_4)
ASM_PFX(TestMmioRead_8A8B_4):
  mov   rbx, 0xc0000000
  mov   eax, dword [rbx]
  ret

global ASM_PFX(TestMmioRead_8A8B_8)
ASM_PFX(TestMmioRead_8A8B_8):
  mov   rbx, 0xc0000000
  mov   rax, qword [rbx]
  ret

global ASM_PFX(TestMmioRead_MOVQ1)
ASM_PFX(TestMmioRead_MOVQ1):
  mov   rbx, 0xc0000000
  movq  xmm1, [rbx]
  ret

global ASM_PFX(TestMmioRead_MOVQ2)
ASM_PFX(TestMmioRead_MOVQ2):
  mov   rbx, 0xc0000000
  movq  xmm1, qword [rbx]
  ret

global ASM_PFX(TestMmioRead_B6B7_1)
ASM_PFX(TestMmioRead_B6B7_1):
  mov   rbx, 0xc0000000
  movzx ax, byte [rbx]
  ret

global ASM_PFX(TestMmioRead_B6B7_2)
ASM_PFX(TestMmioRead_B6B7_2):
  mov   rbx, 0xc0000000
  movzx eax, byte [rbx]
  ret

global ASM_PFX(TestMmioRead_B6B7_3)
ASM_PFX(TestMmioRead_B6B7_3):
  mov   rbx, 0xc0000000
  movzx rax, byte [rbx]
  ret

global ASM_PFX(TestMmioRead_B6B7_4)
ASM_PFX(TestMmioRead_B6B7_4):
  mov   rbx, 0xc0000000
  movzx eax, word [rbx]
  ret

global ASM_PFX(TestMmioRead_B6B7_5)
ASM_PFX(TestMmioRead_B6B7_5):
  mov   rbx, 0xc0000000
  movzx rax, word [rbx]
  ret


global ASM_PFX(TestMmioRead_BEBF_1)
ASM_PFX(TestMmioRead_BEBF_1):
  mov   rbx, 0xc0000000
  movsx ax, byte [rbx]
  ret

global ASM_PFX(TestMmioRead_BEBF_2)
ASM_PFX(TestMmioRead_BEBF_2):
  mov   rbx, 0xc0000000
  movsx eax, byte [rbx]
  ret

global ASM_PFX(TestMmioRead_BEBF_3)
ASM_PFX(TestMmioRead_BEBF_3):
  mov   rbx, 0xc0000000
  movsx rax, byte [rbx]
  ret

global ASM_PFX(TestMmioRead_BEBF_4)
ASM_PFX(TestMmioRead_BEBF_4):
  mov   rbx, 0xc0000000
  movsx eax, word [rbx]
  ret

global ASM_PFX(TestMmioRead_BEBF_5)
ASM_PFX(TestMmioRead_BEBF_5):
  mov   rbx, 0xc0000000
  movsx rax, word [rbx]
  ret

global ASM_PFX(TestMmioRead_BEBF_6)
ASM_PFX(TestMmioRead_BEBF_6):
  mov   rbx, 0xc0000000
  ; movsxd ax, word [rbx]
  ret

global ASM_PFX(TestMmioRead_BEBF_7)
ASM_PFX(TestMmioRead_BEBF_7):
  mov   rbx, 0xc0000000
  ; movsxd eax, dword [rbx]
  ret

global ASM_PFX(TestMmioRead_BEBF_8)
ASM_PFX(TestMmioRead_BEBF_8):
  mov   rbx, 0xc0000000
  movsxd rax, dword [rbx]
  ret
