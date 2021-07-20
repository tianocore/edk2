; WARNING: do not edit!
; Generated from openssl/crypto/aes/asm/aesni-mb-x86_64.pl
;
; Copyright 2013-2020 The OpenSSL Project Authors. All Rights Reserved.
;
; Licensed under the OpenSSL license (the "License").  You may not use
; this file except in compliance with the License.  You can obtain a copy
; in the file LICENSE in the source distribution or at
; https://www.openssl.org/source/license.html

default rel
%define XMMWORD
%define YMMWORD
%define ZMMWORD
section .text code align=64


EXTERN  OPENSSL_ia32cap_P

global  aesni_multi_cbc_encrypt

ALIGN   32
aesni_multi_cbc_encrypt:
        mov     QWORD[8+rsp],rdi        ;WIN64 prologue
        mov     QWORD[16+rsp],rsi
        mov     rax,rsp
$L$SEH_begin_aesni_multi_cbc_encrypt:
        mov     rdi,rcx
        mov     rsi,rdx
        mov     rdx,r8



        mov     rax,rsp

        push    rbx

        push    rbp

        push    r12

        push    r13

        push    r14

        push    r15

        lea     rsp,[((-168))+rsp]
        movaps  XMMWORD[rsp],xmm6
        movaps  XMMWORD[16+rsp],xmm7
        movaps  XMMWORD[32+rsp],xmm8
        movaps  XMMWORD[48+rsp],xmm9
        movaps  XMMWORD[64+rsp],xmm10
        movaps  XMMWORD[80+rsp],xmm11
        movaps  XMMWORD[96+rsp],xmm12
        movaps  XMMWORD[(-104)+rax],xmm13
        movaps  XMMWORD[(-88)+rax],xmm14
        movaps  XMMWORD[(-72)+rax],xmm15






        sub     rsp,48
        and     rsp,-64
        mov     QWORD[16+rsp],rax


$L$enc4x_body:
        movdqu  xmm12,XMMWORD[rsi]
        lea     rsi,[120+rsi]
        lea     rdi,[80+rdi]

$L$enc4x_loop_grande:
        mov     DWORD[24+rsp],edx
        xor     edx,edx
        mov     ecx,DWORD[((-64))+rdi]
        mov     r8,QWORD[((-80))+rdi]
        cmp     ecx,edx
        mov     r12,QWORD[((-72))+rdi]
        cmovg   edx,ecx
        test    ecx,ecx
        movdqu  xmm2,XMMWORD[((-56))+rdi]
        mov     DWORD[32+rsp],ecx
        cmovle  r8,rsp
        mov     ecx,DWORD[((-24))+rdi]
        mov     r9,QWORD[((-40))+rdi]
        cmp     ecx,edx
        mov     r13,QWORD[((-32))+rdi]
        cmovg   edx,ecx
        test    ecx,ecx
        movdqu  xmm3,XMMWORD[((-16))+rdi]
        mov     DWORD[36+rsp],ecx
        cmovle  r9,rsp
        mov     ecx,DWORD[16+rdi]
        mov     r10,QWORD[rdi]
        cmp     ecx,edx
        mov     r14,QWORD[8+rdi]
        cmovg   edx,ecx
        test    ecx,ecx
        movdqu  xmm4,XMMWORD[24+rdi]
        mov     DWORD[40+rsp],ecx
        cmovle  r10,rsp
        mov     ecx,DWORD[56+rdi]
        mov     r11,QWORD[40+rdi]
        cmp     ecx,edx
        mov     r15,QWORD[48+rdi]
        cmovg   edx,ecx
        test    ecx,ecx
        movdqu  xmm5,XMMWORD[64+rdi]
        mov     DWORD[44+rsp],ecx
        cmovle  r11,rsp
        test    edx,edx
        jz      NEAR $L$enc4x_done

        movups  xmm1,XMMWORD[((16-120))+rsi]
        pxor    xmm2,xmm12
        movups  xmm0,XMMWORD[((32-120))+rsi]
        pxor    xmm3,xmm12
        mov     eax,DWORD[((240-120))+rsi]
        pxor    xmm4,xmm12
        movdqu  xmm6,XMMWORD[r8]
        pxor    xmm5,xmm12
        movdqu  xmm7,XMMWORD[r9]
        pxor    xmm2,xmm6
        movdqu  xmm8,XMMWORD[r10]
        pxor    xmm3,xmm7
        movdqu  xmm9,XMMWORD[r11]
        pxor    xmm4,xmm8
        pxor    xmm5,xmm9
        movdqa  xmm10,XMMWORD[32+rsp]
        xor     rbx,rbx
        jmp     NEAR $L$oop_enc4x

ALIGN   32
$L$oop_enc4x:
        add     rbx,16
        lea     rbp,[16+rsp]
        mov     ecx,1
        sub     rbp,rbx

DB      102,15,56,220,209
        prefetcht0      [31+rbx*1+r8]
        prefetcht0      [31+rbx*1+r9]
DB      102,15,56,220,217
        prefetcht0      [31+rbx*1+r10]
        prefetcht0      [31+rbx*1+r10]
DB      102,15,56,220,225
DB      102,15,56,220,233
        movups  xmm1,XMMWORD[((48-120))+rsi]
        cmp     ecx,DWORD[32+rsp]
DB      102,15,56,220,208
DB      102,15,56,220,216
DB      102,15,56,220,224
        cmovge  r8,rbp
        cmovg   r12,rbp
DB      102,15,56,220,232
        movups  xmm0,XMMWORD[((-56))+rsi]
        cmp     ecx,DWORD[36+rsp]
DB      102,15,56,220,209
DB      102,15,56,220,217
DB      102,15,56,220,225
        cmovge  r9,rbp
        cmovg   r13,rbp
DB      102,15,56,220,233
        movups  xmm1,XMMWORD[((-40))+rsi]
        cmp     ecx,DWORD[40+rsp]
DB      102,15,56,220,208
DB      102,15,56,220,216
DB      102,15,56,220,224
        cmovge  r10,rbp
        cmovg   r14,rbp
DB      102,15,56,220,232
        movups  xmm0,XMMWORD[((-24))+rsi]
        cmp     ecx,DWORD[44+rsp]
DB      102,15,56,220,209
DB      102,15,56,220,217
DB      102,15,56,220,225
        cmovge  r11,rbp
        cmovg   r15,rbp
DB      102,15,56,220,233
        movups  xmm1,XMMWORD[((-8))+rsi]
        movdqa  xmm11,xmm10
DB      102,15,56,220,208
        prefetcht0      [15+rbx*1+r12]
        prefetcht0      [15+rbx*1+r13]
DB      102,15,56,220,216
        prefetcht0      [15+rbx*1+r14]
        prefetcht0      [15+rbx*1+r15]
DB      102,15,56,220,224
DB      102,15,56,220,232
        movups  xmm0,XMMWORD[((128-120))+rsi]
        pxor    xmm12,xmm12

DB      102,15,56,220,209
        pcmpgtd xmm11,xmm12
        movdqu  xmm12,XMMWORD[((-120))+rsi]
DB      102,15,56,220,217
        paddd   xmm10,xmm11
        movdqa  XMMWORD[32+rsp],xmm10
DB      102,15,56,220,225
DB      102,15,56,220,233
        movups  xmm1,XMMWORD[((144-120))+rsi]

        cmp     eax,11

DB      102,15,56,220,208
DB      102,15,56,220,216
DB      102,15,56,220,224
DB      102,15,56,220,232
        movups  xmm0,XMMWORD[((160-120))+rsi]

        jb      NEAR $L$enc4x_tail

DB      102,15,56,220,209
DB      102,15,56,220,217
DB      102,15,56,220,225
DB      102,15,56,220,233
        movups  xmm1,XMMWORD[((176-120))+rsi]

DB      102,15,56,220,208
DB      102,15,56,220,216
DB      102,15,56,220,224
DB      102,15,56,220,232
        movups  xmm0,XMMWORD[((192-120))+rsi]

        je      NEAR $L$enc4x_tail

DB      102,15,56,220,209
DB      102,15,56,220,217
DB      102,15,56,220,225
DB      102,15,56,220,233
        movups  xmm1,XMMWORD[((208-120))+rsi]

DB      102,15,56,220,208
DB      102,15,56,220,216
DB      102,15,56,220,224
DB      102,15,56,220,232
        movups  xmm0,XMMWORD[((224-120))+rsi]
        jmp     NEAR $L$enc4x_tail

ALIGN   32
$L$enc4x_tail:
DB      102,15,56,220,209
DB      102,15,56,220,217
DB      102,15,56,220,225
DB      102,15,56,220,233
        movdqu  xmm6,XMMWORD[rbx*1+r8]
        movdqu  xmm1,XMMWORD[((16-120))+rsi]

DB      102,15,56,221,208
        movdqu  xmm7,XMMWORD[rbx*1+r9]
        pxor    xmm6,xmm12
DB      102,15,56,221,216
        movdqu  xmm8,XMMWORD[rbx*1+r10]
        pxor    xmm7,xmm12
DB      102,15,56,221,224
        movdqu  xmm9,XMMWORD[rbx*1+r11]
        pxor    xmm8,xmm12
DB      102,15,56,221,232
        movdqu  xmm0,XMMWORD[((32-120))+rsi]
        pxor    xmm9,xmm12

        movups  XMMWORD[(-16)+rbx*1+r12],xmm2
        pxor    xmm2,xmm6
        movups  XMMWORD[(-16)+rbx*1+r13],xmm3
        pxor    xmm3,xmm7
        movups  XMMWORD[(-16)+rbx*1+r14],xmm4
        pxor    xmm4,xmm8
        movups  XMMWORD[(-16)+rbx*1+r15],xmm5
        pxor    xmm5,xmm9

        dec     edx
        jnz     NEAR $L$oop_enc4x

        mov     rax,QWORD[16+rsp]

        mov     edx,DWORD[24+rsp]










        lea     rdi,[160+rdi]
        dec     edx
        jnz     NEAR $L$enc4x_loop_grande

$L$enc4x_done:
        movaps  xmm6,XMMWORD[((-216))+rax]
        movaps  xmm7,XMMWORD[((-200))+rax]
        movaps  xmm8,XMMWORD[((-184))+rax]
        movaps  xmm9,XMMWORD[((-168))+rax]
        movaps  xmm10,XMMWORD[((-152))+rax]
        movaps  xmm11,XMMWORD[((-136))+rax]
        movaps  xmm12,XMMWORD[((-120))+rax]



        mov     r15,QWORD[((-48))+rax]

        mov     r14,QWORD[((-40))+rax]

        mov     r13,QWORD[((-32))+rax]

        mov     r12,QWORD[((-24))+rax]

        mov     rbp,QWORD[((-16))+rax]

        mov     rbx,QWORD[((-8))+rax]

        lea     rsp,[rax]

$L$enc4x_epilogue:
        mov     rdi,QWORD[8+rsp]        ;WIN64 epilogue
        mov     rsi,QWORD[16+rsp]
        DB      0F3h,0C3h               ;repret

$L$SEH_end_aesni_multi_cbc_encrypt:

global  aesni_multi_cbc_decrypt

ALIGN   32
aesni_multi_cbc_decrypt:
        mov     QWORD[8+rsp],rdi        ;WIN64 prologue
        mov     QWORD[16+rsp],rsi
        mov     rax,rsp
$L$SEH_begin_aesni_multi_cbc_decrypt:
        mov     rdi,rcx
        mov     rsi,rdx
        mov     rdx,r8



        mov     rax,rsp

        push    rbx

        push    rbp

        push    r12

        push    r13

        push    r14

        push    r15

        lea     rsp,[((-168))+rsp]
        movaps  XMMWORD[rsp],xmm6
        movaps  XMMWORD[16+rsp],xmm7
        movaps  XMMWORD[32+rsp],xmm8
        movaps  XMMWORD[48+rsp],xmm9
        movaps  XMMWORD[64+rsp],xmm10
        movaps  XMMWORD[80+rsp],xmm11
        movaps  XMMWORD[96+rsp],xmm12
        movaps  XMMWORD[(-104)+rax],xmm13
        movaps  XMMWORD[(-88)+rax],xmm14
        movaps  XMMWORD[(-72)+rax],xmm15






        sub     rsp,48
        and     rsp,-64
        mov     QWORD[16+rsp],rax


$L$dec4x_body:
        movdqu  xmm12,XMMWORD[rsi]
        lea     rsi,[120+rsi]
        lea     rdi,[80+rdi]

$L$dec4x_loop_grande:
        mov     DWORD[24+rsp],edx
        xor     edx,edx
        mov     ecx,DWORD[((-64))+rdi]
        mov     r8,QWORD[((-80))+rdi]
        cmp     ecx,edx
        mov     r12,QWORD[((-72))+rdi]
        cmovg   edx,ecx
        test    ecx,ecx
        movdqu  xmm6,XMMWORD[((-56))+rdi]
        mov     DWORD[32+rsp],ecx
        cmovle  r8,rsp
        mov     ecx,DWORD[((-24))+rdi]
        mov     r9,QWORD[((-40))+rdi]
        cmp     ecx,edx
        mov     r13,QWORD[((-32))+rdi]
        cmovg   edx,ecx
        test    ecx,ecx
        movdqu  xmm7,XMMWORD[((-16))+rdi]
        mov     DWORD[36+rsp],ecx
        cmovle  r9,rsp
        mov     ecx,DWORD[16+rdi]
        mov     r10,QWORD[rdi]
        cmp     ecx,edx
        mov     r14,QWORD[8+rdi]
        cmovg   edx,ecx
        test    ecx,ecx
        movdqu  xmm8,XMMWORD[24+rdi]
        mov     DWORD[40+rsp],ecx
        cmovle  r10,rsp
        mov     ecx,DWORD[56+rdi]
        mov     r11,QWORD[40+rdi]
        cmp     ecx,edx
        mov     r15,QWORD[48+rdi]
        cmovg   edx,ecx
        test    ecx,ecx
        movdqu  xmm9,XMMWORD[64+rdi]
        mov     DWORD[44+rsp],ecx
        cmovle  r11,rsp
        test    edx,edx
        jz      NEAR $L$dec4x_done

        movups  xmm1,XMMWORD[((16-120))+rsi]
        movups  xmm0,XMMWORD[((32-120))+rsi]
        mov     eax,DWORD[((240-120))+rsi]
        movdqu  xmm2,XMMWORD[r8]
        movdqu  xmm3,XMMWORD[r9]
        pxor    xmm2,xmm12
        movdqu  xmm4,XMMWORD[r10]
        pxor    xmm3,xmm12
        movdqu  xmm5,XMMWORD[r11]
        pxor    xmm4,xmm12
        pxor    xmm5,xmm12
        movdqa  xmm10,XMMWORD[32+rsp]
        xor     rbx,rbx
        jmp     NEAR $L$oop_dec4x

ALIGN   32
$L$oop_dec4x:
        add     rbx,16
        lea     rbp,[16+rsp]
        mov     ecx,1
        sub     rbp,rbx

DB      102,15,56,222,209
        prefetcht0      [31+rbx*1+r8]
        prefetcht0      [31+rbx*1+r9]
DB      102,15,56,222,217
        prefetcht0      [31+rbx*1+r10]
        prefetcht0      [31+rbx*1+r11]
DB      102,15,56,222,225
DB      102,15,56,222,233
        movups  xmm1,XMMWORD[((48-120))+rsi]
        cmp     ecx,DWORD[32+rsp]
DB      102,15,56,222,208
DB      102,15,56,222,216
DB      102,15,56,222,224
        cmovge  r8,rbp
        cmovg   r12,rbp
DB      102,15,56,222,232
        movups  xmm0,XMMWORD[((-56))+rsi]
        cmp     ecx,DWORD[36+rsp]
DB      102,15,56,222,209
DB      102,15,56,222,217
DB      102,15,56,222,225
        cmovge  r9,rbp
        cmovg   r13,rbp
DB      102,15,56,222,233
        movups  xmm1,XMMWORD[((-40))+rsi]
        cmp     ecx,DWORD[40+rsp]
DB      102,15,56,222,208
DB      102,15,56,222,216
DB      102,15,56,222,224
        cmovge  r10,rbp
        cmovg   r14,rbp
DB      102,15,56,222,232
        movups  xmm0,XMMWORD[((-24))+rsi]
        cmp     ecx,DWORD[44+rsp]
DB      102,15,56,222,209
DB      102,15,56,222,217
DB      102,15,56,222,225
        cmovge  r11,rbp
        cmovg   r15,rbp
DB      102,15,56,222,233
        movups  xmm1,XMMWORD[((-8))+rsi]
        movdqa  xmm11,xmm10
DB      102,15,56,222,208
        prefetcht0      [15+rbx*1+r12]
        prefetcht0      [15+rbx*1+r13]
DB      102,15,56,222,216
        prefetcht0      [15+rbx*1+r14]
        prefetcht0      [15+rbx*1+r15]
DB      102,15,56,222,224
DB      102,15,56,222,232
        movups  xmm0,XMMWORD[((128-120))+rsi]
        pxor    xmm12,xmm12

DB      102,15,56,222,209
        pcmpgtd xmm11,xmm12
        movdqu  xmm12,XMMWORD[((-120))+rsi]
DB      102,15,56,222,217
        paddd   xmm10,xmm11
        movdqa  XMMWORD[32+rsp],xmm10
DB      102,15,56,222,225
DB      102,15,56,222,233
        movups  xmm1,XMMWORD[((144-120))+rsi]

        cmp     eax,11

DB      102,15,56,222,208
DB      102,15,56,222,216
DB      102,15,56,222,224
DB      102,15,56,222,232
        movups  xmm0,XMMWORD[((160-120))+rsi]

        jb      NEAR $L$dec4x_tail

DB      102,15,56,222,209
DB      102,15,56,222,217
DB      102,15,56,222,225
DB      102,15,56,222,233
        movups  xmm1,XMMWORD[((176-120))+rsi]

DB      102,15,56,222,208
DB      102,15,56,222,216
DB      102,15,56,222,224
DB      102,15,56,222,232
        movups  xmm0,XMMWORD[((192-120))+rsi]

        je      NEAR $L$dec4x_tail

DB      102,15,56,222,209
DB      102,15,56,222,217
DB      102,15,56,222,225
DB      102,15,56,222,233
        movups  xmm1,XMMWORD[((208-120))+rsi]

DB      102,15,56,222,208
DB      102,15,56,222,216
DB      102,15,56,222,224
DB      102,15,56,222,232
        movups  xmm0,XMMWORD[((224-120))+rsi]
        jmp     NEAR $L$dec4x_tail

ALIGN   32
$L$dec4x_tail:
DB      102,15,56,222,209
DB      102,15,56,222,217
DB      102,15,56,222,225
        pxor    xmm6,xmm0
        pxor    xmm7,xmm0
DB      102,15,56,222,233
        movdqu  xmm1,XMMWORD[((16-120))+rsi]
        pxor    xmm8,xmm0
        pxor    xmm9,xmm0
        movdqu  xmm0,XMMWORD[((32-120))+rsi]

DB      102,15,56,223,214
DB      102,15,56,223,223
        movdqu  xmm6,XMMWORD[((-16))+rbx*1+r8]
        movdqu  xmm7,XMMWORD[((-16))+rbx*1+r9]
DB      102,65,15,56,223,224
DB      102,65,15,56,223,233
        movdqu  xmm8,XMMWORD[((-16))+rbx*1+r10]
        movdqu  xmm9,XMMWORD[((-16))+rbx*1+r11]

        movups  XMMWORD[(-16)+rbx*1+r12],xmm2
        movdqu  xmm2,XMMWORD[rbx*1+r8]
        movups  XMMWORD[(-16)+rbx*1+r13],xmm3
        movdqu  xmm3,XMMWORD[rbx*1+r9]
        pxor    xmm2,xmm12
        movups  XMMWORD[(-16)+rbx*1+r14],xmm4
        movdqu  xmm4,XMMWORD[rbx*1+r10]
        pxor    xmm3,xmm12
        movups  XMMWORD[(-16)+rbx*1+r15],xmm5
        movdqu  xmm5,XMMWORD[rbx*1+r11]
        pxor    xmm4,xmm12
        pxor    xmm5,xmm12

        dec     edx
        jnz     NEAR $L$oop_dec4x

        mov     rax,QWORD[16+rsp]

        mov     edx,DWORD[24+rsp]

        lea     rdi,[160+rdi]
        dec     edx
        jnz     NEAR $L$dec4x_loop_grande

$L$dec4x_done:
        movaps  xmm6,XMMWORD[((-216))+rax]
        movaps  xmm7,XMMWORD[((-200))+rax]
        movaps  xmm8,XMMWORD[((-184))+rax]
        movaps  xmm9,XMMWORD[((-168))+rax]
        movaps  xmm10,XMMWORD[((-152))+rax]
        movaps  xmm11,XMMWORD[((-136))+rax]
        movaps  xmm12,XMMWORD[((-120))+rax]



        mov     r15,QWORD[((-48))+rax]

        mov     r14,QWORD[((-40))+rax]

        mov     r13,QWORD[((-32))+rax]

        mov     r12,QWORD[((-24))+rax]

        mov     rbp,QWORD[((-16))+rax]

        mov     rbx,QWORD[((-8))+rax]

        lea     rsp,[rax]

$L$dec4x_epilogue:
        mov     rdi,QWORD[8+rsp]        ;WIN64 epilogue
        mov     rsi,QWORD[16+rsp]
        DB      0F3h,0C3h               ;repret

$L$SEH_end_aesni_multi_cbc_decrypt:
EXTERN  __imp_RtlVirtualUnwind

ALIGN   16
se_handler:
        push    rsi
        push    rdi
        push    rbx
        push    rbp
        push    r12
        push    r13
        push    r14
        push    r15
        pushfq
        sub     rsp,64

        mov     rax,QWORD[120+r8]
        mov     rbx,QWORD[248+r8]

        mov     rsi,QWORD[8+r9]
        mov     r11,QWORD[56+r9]

        mov     r10d,DWORD[r11]
        lea     r10,[r10*1+rsi]
        cmp     rbx,r10
        jb      NEAR $L$in_prologue

        mov     rax,QWORD[152+r8]

        mov     r10d,DWORD[4+r11]
        lea     r10,[r10*1+rsi]
        cmp     rbx,r10
        jae     NEAR $L$in_prologue

        mov     rax,QWORD[16+rax]

        mov     rbx,QWORD[((-8))+rax]
        mov     rbp,QWORD[((-16))+rax]
        mov     r12,QWORD[((-24))+rax]
        mov     r13,QWORD[((-32))+rax]
        mov     r14,QWORD[((-40))+rax]
        mov     r15,QWORD[((-48))+rax]
        mov     QWORD[144+r8],rbx
        mov     QWORD[160+r8],rbp
        mov     QWORD[216+r8],r12
        mov     QWORD[224+r8],r13
        mov     QWORD[232+r8],r14
        mov     QWORD[240+r8],r15

        lea     rsi,[((-56-160))+rax]
        lea     rdi,[512+r8]
        mov     ecx,20
        DD      0xa548f3fc

$L$in_prologue:
        mov     rdi,QWORD[8+rax]
        mov     rsi,QWORD[16+rax]
        mov     QWORD[152+r8],rax
        mov     QWORD[168+r8],rsi
        mov     QWORD[176+r8],rdi

        mov     rdi,QWORD[40+r9]
        mov     rsi,r8
        mov     ecx,154
        DD      0xa548f3fc

        mov     rsi,r9
        xor     rcx,rcx
        mov     rdx,QWORD[8+rsi]
        mov     r8,QWORD[rsi]
        mov     r9,QWORD[16+rsi]
        mov     r10,QWORD[40+rsi]
        lea     r11,[56+rsi]
        lea     r12,[24+rsi]
        mov     QWORD[32+rsp],r10
        mov     QWORD[40+rsp],r11
        mov     QWORD[48+rsp],r12
        mov     QWORD[56+rsp],rcx
        call    QWORD[__imp_RtlVirtualUnwind]

        mov     eax,1
        add     rsp,64
        popfq
        pop     r15
        pop     r14
        pop     r13
        pop     r12
        pop     rbp
        pop     rbx
        pop     rdi
        pop     rsi
        DB      0F3h,0C3h               ;repret


section .pdata rdata align=4
ALIGN   4
        DD      $L$SEH_begin_aesni_multi_cbc_encrypt wrt ..imagebase
        DD      $L$SEH_end_aesni_multi_cbc_encrypt wrt ..imagebase
        DD      $L$SEH_info_aesni_multi_cbc_encrypt wrt ..imagebase
        DD      $L$SEH_begin_aesni_multi_cbc_decrypt wrt ..imagebase
        DD      $L$SEH_end_aesni_multi_cbc_decrypt wrt ..imagebase
        DD      $L$SEH_info_aesni_multi_cbc_decrypt wrt ..imagebase
section .xdata rdata align=8
ALIGN   8
$L$SEH_info_aesni_multi_cbc_encrypt:
DB      9,0,0,0
        DD      se_handler wrt ..imagebase
        DD      $L$enc4x_body wrt ..imagebase,$L$enc4x_epilogue wrt ..imagebase
$L$SEH_info_aesni_multi_cbc_decrypt:
DB      9,0,0,0
        DD      se_handler wrt ..imagebase
        DD      $L$dec4x_body wrt ..imagebase,$L$dec4x_epilogue wrt ..imagebase
